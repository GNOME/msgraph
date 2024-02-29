/* Copyright 2022-2023 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "msg-service.h"

#include "msg-error.h"
#include "msg-private.h"

typedef struct _MsgServicePrivate MsgServicePrivate;
struct _MsgServicePrivate {
  MsgAuthorizer *authorizer;
  SoupSession *session;
};

G_DEFINE_TYPE_WITH_PRIVATE (MsgService, msg_service, G_TYPE_OBJECT);

#define MSG_SERVICE_GET_PRIVATE(o)  ((MsgServicePrivate *)msg_service_get_instance_private ((o)))

enum {
  PROP_0,
  PROP_AUTHORIZER,
  PROP_COUNT
};

static GParamSpec *properties[PROP_COUNT] = { NULL, };

gboolean
msg_service_refresh_authorization (MsgService    *self,
                                   GCancellable  *cancellable,
                                   GError       **error)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);

  if (!priv->authorizer || !MSG_IS_AUTHORIZER (priv->authorizer)) {
    g_set_error (error,
                 MSG_ERROR,
                 MSG_ERROR_FAILED,
                 "Authorizer is NULL or is not an MSG_AUTHORIZER instance");
    return FALSE;
  }

  if (!msg_authorizer_refresh_authorization (priv->authorizer, cancellable, error))
    return FALSE;

  return TRUE;
}

guint
msg_service_get_https_port (void)
{
  const char *port_string;

  /* Allow changing the HTTPS port just for testing. */
  port_string = g_getenv ("MSG_HTTPS_PORT");
  if (port_string != NULL) {
    const char *end;

    guint64 port = g_ascii_strtoull (port_string, (gchar **)&end, 10);

    if (port != 0 && *end == '\0') {
      g_debug ("Overriding message port to %" G_GUINT64_FORMAT ".", port);
      return port;
    }
  }

  /* Return the default. */
  return 443;
}

gboolean
msg_service_accept_certificate_cb (__attribute__ ((unused)) SoupMessage         *msg,
                                   __attribute__ ((unused)) GTlsCertificate     *tls_cert,
                                   __attribute__ ((unused)) GTlsCertificateFlags tls_errors,
                                   gpointer                                      session)
{
  return !!g_object_get_data (session, "msg-lax-ssl");
}


SoupMessage *
msg_service_new_message_from_uri (MsgService *self,
                                  const char *method,
                                  GUri       *uri)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);
  SoupMessage *ret;

  ret = soup_message_new_from_uri (method, uri);

  g_signal_connect (ret, "accept-certificate", G_CALLBACK (msg_service_accept_certificate_cb), priv->session);

  return ret;
}


/**
 * msg_service_build_message:
 * @method: transfer method
 * @uri: uri to access
 * @etag: an optional etag
 * @etag_if_match: use etag if
 *
 * Construct and checks a #SoupMessage for transfer
 *
 * Returns: (transfer full): a #SoupMessage or NULL on error.
 */
SoupMessage *
msg_service_build_message (MsgService *self,
                           const char *method,
                           const char *uri,
                           const char *etag,
                           gboolean    etag_if_match)
{
  SoupMessage *message;
  g_autoptr (GUri) _uri = NULL;
  g_autoptr (GUri) _uri_parsed = NULL;

  _uri_parsed = g_uri_parse (uri, SOUP_HTTP_URI_FLAGS, NULL);
  _uri = soup_uri_copy (_uri_parsed, SOUP_URI_PORT, msg_service_get_https_port (), SOUP_URI_NONE);
  g_assert_cmpstr (g_uri_get_scheme (_uri), ==, "https");

  message = msg_service_new_message_from_uri (self, method, _uri);
  if (etag)
    soup_message_headers_append (soup_message_get_request_headers (message), (etag_if_match == TRUE) ? "If-Match" : "If-None-Match", etag);

  return message;
}

/**
 * msg_service_send:
 * @self: a msg service
 * @message: a #SoupMessage
 * @cancellable: a #GCancellable
 * @error: a #Gerror
 *
 * Adds authorizer information to `message` and send it.
 *
 * Returns: (transfer full): a #GInputStream
 */
GInputStream *
msg_service_send (MsgService    *self,
                  SoupMessage   *message,
                  GCancellable  *cancellable,
                  GError       **error)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);

  msg_authorizer_process_request (priv->authorizer, message);

  return soup_session_send (priv->session, message, cancellable, error);
}


/**
 * msg_service_send_and_read:
 * @self: a msg service
 * @message: a #SoupMessage
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Adds authorizer information to `message` and send it.
 *
 * Returns: http status code of the response
 */
GBytes *
msg_service_send_and_read (MsgService    *self,
                           SoupMessage   *message,
                           GCancellable  *cancellable,
                           GError       **error)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);

  msg_authorizer_process_request (priv->authorizer, message);

  return soup_session_send_and_read (priv->session, message, cancellable, error);
}

static void
msg_service_set_authorizer (MsgService    *self,
                            MsgAuthorizer *authorizer)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);

  g_return_if_fail (MSG_IS_SERVICE (self));
  g_return_if_fail (authorizer == NULL || MSG_IS_AUTHORIZER (authorizer));

  g_clear_object (&priv->authorizer);
  priv->authorizer = authorizer;
  if (priv->authorizer)
    g_object_ref (priv->authorizer);

  g_object_notify (G_OBJECT (self), "authorizer");
}

static void
msg_service_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MsgService *self = MSG_SERVICE (object);

  switch (property_id) {
    case PROP_AUTHORIZER:
      msg_service_set_authorizer (self, MSG_AUTHORIZER (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
msg_service_get_property (GObject                         *object,
                          guint                            property_id,
                          __attribute__ ((unused)) GValue *value,
                          GParamSpec                      *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
soup_log_printer (__attribute__ ((unused)) SoupLogger        *logger,
                  __attribute__ ((unused)) SoupLoggerLogLevel level,
                  char                                        direction,
                  const char                                 *data,
                  __attribute__ ((unused)) gpointer           user_data)
{
  g_debug ("%c %s", direction, data);
}

MsgLogLevel
msg_servie_get_log_level (void)
{
  static int level = -1;

  if (level < 0) {
    const char *env = g_getenv ("MSG_DEBUG");

    if (env != NULL) {
      level = atoi (env);
    }

    level = MIN (MAX (level, 0), MSG_LOG_FULL_UNREDACTED);
  }

  return level;
}

static void
msg_service_init (MsgService *self)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);

  /* priv->session = soup_session_new_with_options ("timeout", 0, NULL); */
  priv->session = soup_session_new ();

  /* Iff MSG_LAX_SSL_CERTIFICATES=1, relax SSL certificate validation to allow using invalid/unsigned certificates for testing. */
  if (g_strcmp0 (g_getenv ("MSG_LAX_SSL_CERTIFICATES"), "1") == 0) {
    g_object_set_data (G_OBJECT (priv->session), "msg-lax-ssl", (gpointer)TRUE);
  }

  if (msg_servie_get_log_level () > MSG_LOG_MESSAGES) {
    g_autoptr (SoupLogger) logger = NULL;
    SoupLoggerLogLevel level;

    switch (msg_servie_get_log_level ()) {
      case MSG_LOG_FULL_UNREDACTED:
      case MSG_LOG_FULL:
        level = SOUP_LOGGER_LOG_BODY;
        break;
      case MSG_LOG_HEADERS:
        level = SOUP_LOGGER_LOG_HEADERS;
        break;
      case MSG_LOG_MESSAGES:
      case MSG_LOG_NONE:
      default:
        g_assert_not_reached ();
    }

    logger = soup_logger_new (level);
    soup_logger_set_printer (logger, (SoupLoggerPrinter)soup_log_printer, NULL, NULL);
    soup_session_add_feature (priv->session, SOUP_SESSION_FEATURE (logger));
  }
}

static void
msg_service_class_init (MsgServiceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = msg_service_set_property;
  object_class->get_property = msg_service_get_property;

  properties [PROP_AUTHORIZER] = g_param_spec_object ("authorizer",
                                                      "Authorizer",
                                                      "The authorizer for this service",
                                                      MSG_TYPE_AUTHORIZER,
                                                      G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_COUNT, properties);
}

MsgService *
msg_service_new (MsgAuthorizer *authorizer)
{
  return g_object_new (MSG_TYPE_SERVICE, "authorizer", authorizer, NULL);
}

/**
 * msg_service_parse_response:
 * @bytes: input bytes containing response buffer
 * @object: a pointer to the returning root object
 * @error: a #GError
 *
 * Parse response data and check for errors. In case
 * no errors are found, return json root object.
 *
 * Returns: (transfer full): a #JsonParser
 */
JsonParser *
msg_service_parse_response (GBytes      *bytes,
                            JsonObject **object,
                            GError     **error)
{
  JsonParser *parser = NULL;
  g_autoptr (GError) local_error = NULL;
  JsonObject *root_object = NULL;
  JsonNode *root = NULL;
  const char *content;
  gsize len;

  content = g_bytes_get_data (bytes, &len);

  parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, content, len, &local_error)) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  root = json_parser_get_root (parser);
  if (!JSON_NODE_HOLDS_OBJECT (root)) {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "Invalid response, didn't get JSON object");
    return NULL;
  }

  root_object = json_node_get_object (root);
  /* g_print (json_to_string (root, TRUE)); */

  if (json_object_has_member (root_object, "error")) {
    JsonObject *error_object = json_object_get_object_member (root_object, "error");
    const char *message = json_object_get_string_member (error_object, "message");

    g_set_error_literal (error,
                         MSG_ERROR,
                         MSG_ERROR_FAILED,
                         message);
    return NULL;
  }

  if (object)
    *object = root_object;

  return parser;
}

/**
 * msg_service_get_session:
 * @self: a #MsgService
 *
 * Get related soup session
 *
 * Returns: (transfer none): a #SoupSession
 */
SoupSession *
msg_service_get_session (MsgService *self)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);
  return priv->session;
}

/**
 * msg_service_get_authorizer:
 * @self: a #MsgService
 *
 * Get related authorizer.
 *
 * Returns: (transfer none): a #MsgAuthorizer
 */
MsgAuthorizer *
msg_service_get_authorizer (MsgService *self)
{
  MsgServicePrivate *priv = MSG_SERVICE_GET_PRIVATE (self);
  return priv->authorizer;
}
