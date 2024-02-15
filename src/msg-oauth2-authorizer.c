#include "config.h"

#include <glib.h>

#define GETTEXT_PACKAGE "MOEP"

#include <glib/gi18n-lib.h>

#include "msg-authorizer.h"
#include "msg-oauth2-authorizer.h"
#include "msg-error.h"
#include "msg-service.h"

static void authorizer_init (MsgAuthorizerInterface *iface);

struct _MsgOAuth2AuthorizerPrivate {
  SoupSession *session;

  char *client_id;
  char *redirect_uri;

  GMutex mutex;

  char *access_token;
  char *refresh_token;
};

enum {
  PROP_CLIENT_ID = 1,
  PROP_REDIRECT_URI,
  PROP_REFRESH_TOKEN,
};

G_DEFINE_TYPE_WITH_CODE (MsgOAuth2Authorizer, msg_oauth2_authorizer,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (MsgOAuth2Authorizer)
                         G_IMPLEMENT_INTERFACE (MSG_TYPE_AUTHORIZER,
                                                authorizer_init));

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  MsgOAuth2Authorizer *self = MSG_OAUTH2_AUTHORIZER (object);
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);


  switch (property_id) {
    case PROP_CLIENT_ID:
      g_value_set_string (value, priv->client_id);
      break;
    case PROP_REDIRECT_URI:
      g_value_set_string (value, priv->redirect_uri);
      break;
    case PROP_REFRESH_TOKEN:
      g_mutex_lock (&priv->mutex);
      g_value_set_string (value, priv->refresh_token);
      g_mutex_unlock (&priv->mutex);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
msg_oauth2_authorizer_set_refresh_token (MsgOAuth2Authorizer *self,
                                         const char          *refresh_token)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_return_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self));

  g_mutex_lock (&priv->mutex);

  if (g_strcmp0 (priv->refresh_token, refresh_token) == 0) {
    g_mutex_unlock (&priv->mutex);
    return;
  }

  g_clear_pointer (&priv->access_token, g_free);
  g_clear_pointer (&priv->refresh_token, g_free);
  priv->refresh_token = g_strdup (refresh_token);

  g_mutex_unlock (&priv->mutex);

  g_object_notify (G_OBJECT (self), "refresh-token");
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  MsgOAuth2Authorizer *self = MSG_OAUTH2_AUTHORIZER (object);
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  switch (property_id) {
    case PROP_CLIENT_ID:
      priv->client_id = g_value_dup_string (value);
      break;
    case PROP_REDIRECT_URI:
      priv->redirect_uri = g_value_dup_string (value);
      break;
    case PROP_REFRESH_TOKEN:
      msg_oauth2_authorizer_set_refresh_token (self,
                                               g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dispose (GObject *object)
{
  MsgOAuth2Authorizer *self = MSG_OAUTH2_AUTHORIZER (object);
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_clear_object (&priv->session);

  G_OBJECT_CLASS (msg_oauth2_authorizer_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  MsgOAuth2Authorizer *self = MSG_OAUTH2_AUTHORIZER (object);
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_free (priv->client_id);
  g_free (priv->redirect_uri);

  g_mutex_lock (&priv->mutex);
  g_free (priv->access_token);
  g_mutex_unlock (&priv->mutex);

  g_free (priv->refresh_token);

  g_mutex_clear (&priv->mutex);

  G_OBJECT_CLASS (msg_oauth2_authorizer_parent_class)->finalize (object);
}


static void
msg_oauth2_authorizer_class_init (MsgOAuth2AuthorizerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->dispose = dispose;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_CLIENT_ID,
                                   g_param_spec_string ("client-id",
                                                        "Client ID",
                                                        "A client ID for your application.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_REDIRECT_URI,
                                   g_param_spec_string ("redirect-uri",
                                                        "Redirect URI",
                                                        "Redirect URI to send the response from the authorisation request to.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_REFRESH_TOKEN,
                                   g_param_spec_string ("refresh-token",
                                                        "Refresh Token",
                                                        "The server provided refresh token.",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
msg_oauth2_authorizer_init (MsgOAuth2Authorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_mutex_init (&priv->mutex);
  priv->session = soup_session_new ();
}

static void
sign_message_locked (MsgOAuth2Authorizer *self,
                     SoupMessage         *message,
                     const char          *access_token)
{
  GUri *message_uri;
  g_autofree char *auth_header = NULL;

  g_return_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self));
  g_return_if_fail (SOUP_IS_MESSAGE (message));
  g_return_if_fail (access_token != NULL && *access_token != '\0');

  message_uri = soup_message_get_uri (message);

  if (strcmp (g_uri_get_scheme (message_uri), "https")) {
    g_warning ("Not authorizing a non-HTTPS message with the user’s OAuth 2.0 access token as the connection isn’t secure.");
    return;
  }

  /* Add the authorisation header. */
  auth_header = g_strdup_printf ("Bearer %s", access_token);
  soup_message_headers_append (soup_message_get_request_headers (message), "Authorization", auth_header);
}

static void
process_request (MsgAuthorizer *self,
                 SoupMessage   *message)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (MSG_OAUTH2_AUTHORIZER (self));

  g_mutex_lock (&priv->mutex);

  g_assert ((priv->access_token == NULL) || (priv->refresh_token != NULL));

  if (priv->access_token != NULL) {
    sign_message_locked (MSG_OAUTH2_AUTHORIZER (self), message, priv->access_token);
  }

  g_mutex_unlock (&priv->mutex);
}

static void
parse_grant_response (MsgOAuth2Authorizer  *self,
                      GBytes               *bytes,
                      GError              **error)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);
  JsonParser *parser = NULL;
  JsonNode *root_node;
  JsonObject *root_object;
  const char *access_token = NULL, *refresh_token = NULL;
  GError *child_error = NULL;
  const char *content = NULL;
  gsize len;

  content = g_bytes_get_data (bytes, &len);

  /* Parse the successful response */
  parser = json_parser_new ();

  json_parser_load_from_data (parser, content, len, &child_error);

  if (child_error != NULL) {
    g_clear_error (&child_error);
    g_set_error_literal (&child_error, MSG_ERROR,
                         MSG_ERROR_PROTOCOL_ERROR,
                         _("The server returned a malformed response."));

    goto done;
  }

  /* Extract the access token, TTL and refresh token */
  root_node = json_parser_get_root (parser);

  if (JSON_NODE_HOLDS_OBJECT (root_node) == FALSE) {
    g_set_error_literal (&child_error, MSG_ERROR,
                         MSG_ERROR_PROTOCOL_ERROR,
                         _("The server returned a malformed response."));
    goto done;
  }

  root_object = json_node_get_object (root_node);

  if (json_object_has_member (root_object, "access_token")) {
    access_token = json_object_get_string_member (root_object,
                                                  "access_token");
  }
  if (json_object_has_member (root_object, "refresh_token")) {
    refresh_token = json_object_get_string_member (root_object,
                                                   "refresh_token");
  }

  /* Always require an access token. */
  if (access_token == NULL || *access_token == '\0') {
    g_set_error_literal (&child_error, MSG_ERROR,
                         MSG_ERROR_PROTOCOL_ERROR,
                         _("The server returned a malformed response."));

    access_token = NULL;
    refresh_token = NULL;

    goto done;
  }

  /* Only require a refresh token if this is the first authentication.
   */
  if ((refresh_token == NULL || *refresh_token == '\0') &&
      priv->refresh_token == NULL) {
    g_set_error_literal (&child_error, MSG_ERROR,
                         MSG_ERROR_PROTOCOL_ERROR,
                         _("The server returned a malformed response."));

    access_token = NULL;
    refresh_token = NULL;

    goto done;
  }

done:
  /* Postconditions. */
  g_assert ((refresh_token == NULL) || (access_token != NULL));
  g_assert ((child_error != NULL) == (access_token == NULL));

  /* Update state. */
  g_mutex_lock (&priv->mutex);

  g_free (priv->access_token);
  priv->access_token = g_strdup (access_token);

  if (refresh_token != NULL) {
    g_free (priv->refresh_token);
    priv->refresh_token = g_strdup (refresh_token);
  }

  g_mutex_unlock (&priv->mutex);

  if (child_error != NULL) {
    g_propagate_error (error, child_error);
  }

  g_object_unref (parser);
}


static void
parse_grant_error (GBytes  *bytes,
                   GError **error)
{
  JsonParser *parser = NULL;
  JsonNode *root_node;
  JsonObject *root_object;
  const char *error_code = NULL;
  GError *child_error = NULL;
  const char *content;
  gsize len;

  content = g_bytes_get_data (bytes, &len);

  /* Parse the error response */
  parser = json_parser_new ();

  if (content == NULL) {
    g_clear_error (&child_error);
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));

    goto done;
  }

  json_parser_load_from_data (parser, content, len, &child_error);

  if (child_error != NULL) {
    g_clear_error (&child_error);
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));

    goto done;
  }

  /* Extract the error code. */
  root_node = json_parser_get_root (parser);

  if (JSON_NODE_HOLDS_OBJECT (root_node) == FALSE) {
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));
    goto done;
  }

  root_object = json_node_get_object (root_node);

  if (json_object_has_member (root_object, "error")) {
    error_code = json_object_get_string_member (root_object,
                                                "error");
  }

  /* Always require an error_code. */
  if (error_code == NULL || *error_code == '\0') {
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));

    error_code = NULL;

    goto done;
  }

  /* Parse the error code. */
  if (strcmp (error_code, "invalid_grant") == 0) {
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("Access was denied by the user or server."));
  } else {
    /* Unknown error code. */
    g_set_error_literal (&child_error, MSG_ERROR, MSG_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));
  }

done:
  /* Postconditions. */
  g_assert (child_error != NULL);

  if (child_error != NULL) {
    g_propagate_error (error, child_error);
  }

  g_object_unref (parser);
}


static void
authorizer_message_starting (SoupMessage *msg,
                             gpointer     body)
{
  soup_message_set_request_body_from_bytes (msg, "application/x-www-form-urlencoded", body);
}

static void
authorizer_message_finished (__attribute__ ((unused)) SoupMessage *msg,
                             gpointer                              body)
{
  g_bytes_unref (body);
}

static SoupMessage *
build_authorization_message (MsgAuthorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv;
  SoupMessage *message = NULL;
  GUri *_uri = NULL;
  gchar *request_body;
  GBytes *body;

  priv = msg_oauth2_authorizer_get_instance_private (MSG_OAUTH2_AUTHORIZER (self));

  /* Prepare the request */
  request_body = soup_form_encode ("client_id", priv->client_id,
                                   "refresh_token", priv->refresh_token,
                                   "grant_type", "refresh_token",
                                   NULL);

  /* Build the message */
  _uri = g_uri_build (SOUP_HTTP_URI_FLAGS,
                      "https", NULL, "login.microsoftonline.com",
                      msg_service_get_https_port (),
                      "/common/oauth2/v2.0/token", NULL, NULL);
  message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
  g_uri_unref (_uri);

  g_signal_connect (message, "accept-certificate",
                    G_CALLBACK (msg_service_accept_certificate_cb),
                    priv->session);

  body = g_bytes_new_take (request_body, strlen (request_body));

  g_signal_connect (message, "starting", G_CALLBACK (authorizer_message_starting), body);
  g_signal_connect (message, "finished", G_CALLBACK (authorizer_message_finished), body);

  return message;
}

static gboolean
refresh_authorization (MsgAuthorizer  *self,
                       GCancellable   *cancellable,
                       GError        **error)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (MSG_OAUTH2_AUTHORIZER (self));
  g_autoptr (SoupMessage) message = NULL;
  GError *local_error = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), FALSE);

  if (priv->refresh_token == NULL) {
    return FALSE;
  }

  message = build_authorization_message (self);
  response = soup_session_send_and_read (priv->session, message, cancellable, &local_error);
  if (local_error) {
    parse_grant_error (response, &local_error);
    return FALSE;
  }

  /* Parse and handle the response */
  parse_grant_response (MSG_OAUTH2_AUTHORIZER (self), response, &local_error);
  if (local_error) {
    g_propagate_error (error, local_error);
    return FALSE;
  }

  return TRUE;
}

static void
authorizer_init (MsgAuthorizerInterface *iface)
{
  iface->process_request = process_request;
  iface->refresh_authorization = refresh_authorization;
}

MsgOAuth2Authorizer *
msg_oauth2_authorizer_new (const char *client_id,
                           const char *redirect_uri)
{
  MsgOAuth2Authorizer *authorizer = NULL;

  g_return_val_if_fail (client_id != NULL && *client_id != '\0', NULL);
  g_return_val_if_fail (redirect_uri != NULL && *redirect_uri != '\0', NULL);

  authorizer = MSG_OAUTH2_AUTHORIZER (g_object_new (MSG_TYPE_OAUTH2_AUTHORIZER,
                                                    "client-id", client_id,
                                                    "redirect-uri", redirect_uri,
                                                    NULL));
  return authorizer;
}

char *
msg_oauth2_authorizer_build_authentication_uri (MsgOAuth2Authorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);
  GString *uri = NULL;

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), NULL);

  g_mutex_lock (&priv->mutex);

  uri = g_string_new ("https://login.microsoftonline.com/common/oauth2/v2.0/authorize"
                      "?response_type=code"
                      "&client_id=");
  g_string_append_uri_escaped (uri, priv->client_id, NULL, TRUE);
  g_string_append (uri, "&redirect_uri=");
  g_string_append_uri_escaped (uri, priv->redirect_uri, NULL, TRUE);
  g_string_append (uri, "&response_mode=query");
  /* g_string_append_uri_escaped (uri, "goa-oauth2://localhost/e5a936a7-1145-4ffb-b7e4-5968a9231ade", NULL, TRUE); */

  g_string_append (uri, "&scope=");

  /* FIXME */
  g_string_append_uri_escaped (uri, "files.readwrite offline_access", NULL, TRUE);

  g_mutex_unlock (&priv->mutex);

  return g_string_free (uri, FALSE);
}

gboolean
msg_oauth2_authorizer_request_authorization (MsgOAuth2Authorizer  *self,
                                             const char           *authorization_code,
                                             GCancellable         *cancellable,
                                             GError              **error)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);
  g_autoptr (SoupMessage) message = NULL;
  GUri *_uri = NULL;
  char *request_body = NULL;
  GError *child_error = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autofree char *scope = g_strdup ("files.readwrite offline_access");
  GBytes *body = NULL;

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), FALSE);
  g_return_val_if_fail (authorization_code != NULL &&
                        *authorization_code != '\0', FALSE);
  g_return_val_if_fail (cancellable == NULL ||
                        G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);


  request_body = soup_form_encode ("client_id", priv->client_id,
                                   "code", authorization_code,
                                   "redirect_uri", priv->redirect_uri,
                                   "grant_type", "authorization_code",
                                   "scope", scope,
                                   NULL);

  /* Build the message */
  _uri = g_uri_build (SOUP_HTTP_URI_FLAGS, "https", NULL, "login.microsoftonline.com", msg_service_get_https_port (), "/common/oauth2/v2.0/token", NULL, NULL);
  message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
  g_uri_unref (_uri);

  g_signal_connect (message, "accept-certificate",
                    G_CALLBACK (msg_service_accept_certificate_cb),
                    priv->session);

  body = g_bytes_new_take (request_body, strlen (request_body));

  g_signal_connect (message, "starting", G_CALLBACK (authorizer_message_starting), body);
  g_signal_connect (message, "finished", G_CALLBACK (authorizer_message_finished), body);

  request_body = NULL;

  response = soup_session_send_and_read (priv->session, message, cancellable, &child_error);
  if (child_error) {
    parse_grant_error (response, error);
    return FALSE;
  }

  /* Parse and handle the response */
  parse_grant_response (self, response, &child_error);

  if (child_error != NULL) {
    g_propagate_error (error, child_error);
    return FALSE;
  }

  return TRUE;
}

static void
request_authorization_thread (GTask        *task,
                              gpointer      source_object,
                              gpointer      task_data,
                              GCancellable *cancellable)
{
  MsgOAuth2Authorizer *authorizer = MSG_OAUTH2_AUTHORIZER (source_object);
  g_autoptr (GError) error = NULL;
  const char *authorization_code = task_data;

  if (!msg_oauth2_authorizer_request_authorization (authorizer,
                                                    authorization_code,
                                                    cancellable,
                                                    &error))
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_boolean (task, TRUE);
}

void
msg_oauth2_authorizer_request_authorization_async (MsgOAuth2Authorizer *self,
                                                   const char          *authorization_code,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data)
{
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self));
  g_return_if_fail (authorization_code != NULL && *authorization_code != '\0');
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, msg_oauth2_authorizer_request_authorization_async);
  g_task_set_task_data (task, g_strdup (authorization_code), (GDestroyNotify)g_free);
  g_task_run_in_thread (task, request_authorization_thread);
}

gboolean
msg_oauth2_authorizer_request_authorization_finish (MsgOAuth2Authorizer  *self,
                                                    GAsyncResult         *async_result,
                                                    GError              **error)
{
  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
  g_return_val_if_fail (g_async_result_is_tagged (async_result, msg_oauth2_authorizer_request_authorization_async), FALSE);

  return g_task_propagate_boolean (G_TASK (async_result), error);
}

const char *
msg_oauth2_authorizer_get_client_id (MsgOAuth2Authorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), NULL);
  return priv->client_id;
}

const char *
msg_oauth2_authorizer_get_redirect_uri (MsgOAuth2Authorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), NULL);
  return priv->redirect_uri;
}

char *
msg_oauth2_authorizer_dup_refresh_token (MsgOAuth2Authorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (self);
  char *refresh_token;

  g_return_val_if_fail (MSG_IS_OAUTH2_AUTHORIZER (self), NULL);

  g_mutex_lock (&priv->mutex);
  refresh_token = g_strdup (priv->refresh_token);
  g_mutex_unlock (&priv->mutex);

  return refresh_token;
}

void
msg_oauth2_authorizer_test_save_credentials (MsgAuthorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (MSG_OAUTH2_AUTHORIZER (self));
  g_autoptr (GKeyFile) key_file = g_key_file_new ();
  g_autoptr (GError) error = NULL;

  /* g_key_file_set_string (key_file, "General", "ClientID", priv->client_id); */
  g_key_file_set_string (key_file, "General", "RefreshToken", priv->refresh_token);

  if (!g_key_file_save_to_file (key_file, "msgraph.ini", &error)) {
    g_warning ("Error saving key file: %s", error->message);
  }
}

gboolean
msg_oauth2_authorizer_test_load_credentials (MsgAuthorizer *self)
{
  MsgOAuth2AuthorizerPrivate *priv = msg_oauth2_authorizer_get_instance_private (MSG_OAUTH2_AUTHORIZER (self));
  g_autoptr (GKeyFile) key_file = g_key_file_new ();
  g_autoptr (GError) error = NULL;

  if (!g_key_file_load_from_file (key_file, "msgraph.ini", G_KEY_FILE_NONE, &error)) {
    if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
      g_warning ("Error loading key file: %s", error->message);
    return FALSE;
  }

  /* priv->client_id = g_key_file_get_string (key_file, "General", "ClientId", &error); */
  priv->refresh_token = g_key_file_get_string (key_file, "General", "RefreshToken", &error);

  return TRUE;
}
