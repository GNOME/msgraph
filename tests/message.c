#include "src/msg-authorizer.h"
#include "src/message/msg-message-service.h"
#include "src/message/msg-message.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

#define CLIENT_ID "c6ab1078-80d3-40ba-a49b-375dd013251f"
#define REDIRECT_URI ""

static MsgService *service = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
    MsgMessage *message;
} TempMessageData;

static void
setup_temp_message (TempMessageData *data,
                    const void      *user_data)
{
  g_autoptr (MsgMessage) message = NULL;
  g_autoptr (MsgMessage) new_message = NULL;
  MsgService *service = (MsgService *) user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "setup-temp-message");

  message = MSG_MESSAGE (msg_message_new ());
  msg_message_set_subject (message, "Hello World", NULL, &error);
  g_assert_no_error (error);
  msg_message_set_body (message, "Test :)", NULL, &error);
  g_assert_no_error (error);

  data->message = msg_message_service_create_draft (MSG_MESSAGE_SERVICE (service), message, NULL, &error);
  g_assert_no_error (error);

  uhm_server_end_trace (mock_server);
}

static void
teardown_temp_message (TempMessageData *data,
                       const void      *user_data)
{
  MsgService *service = (MsgService *)user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "teardown-temp-message");

  if (data->message) {
    msg_message_service_delete (MSG_MESSAGE_SERVICE (service), data->message, NULL, &error);
    g_clear_object (&data->message);
  }

  uhm_server_end_trace (mock_server);
}

void
test_authentication (void)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *authentication_uri = NULL;
  g_autofree char *authorisation_code = NULL;

  if (msg_service_refresh_authorization (MSG_SERVICE (service), NULL, &error))
    return;

  msg_test_mock_server_start_trace (mock_server, "setup-oauth2-authorizer-data-authenticated");

  /* Get an authentication URI. */
  authentication_uri = msg_oauth2_authorizer_build_authentication_uri (MSG_OAUTH2_AUTHORIZER (msg_test_get_authorizer ()));
  g_assert (authentication_uri != NULL);

  if (uhm_server_get_enable_online (mock_server)) {
    authorisation_code = msg_test_query_user_for_verifier (authentication_uri);
    g_assert (authorisation_code != NULL);
  } else {
    /* Hard-coded default to match the trace file. */
    authorisation_code = g_strdup ("4/GeYb_3HkYh4vyephp-lbvzQs1GAb.YtXAxmx-uJ0eoiIBeO6P2m9iH6kvkQI");
  }

  g_assert (msg_oauth2_authorizer_request_authorization (MSG_OAUTH2_AUTHORIZER (msg_test_get_authorizer ()), authorisation_code, NULL, NULL) == TRUE);

  msg_oauth2_authorizer_test_save_credentials (msg_test_get_authorizer ());

  uhm_server_end_trace (mock_server);
}

static void
test_get_messages (__attribute__ ((unused)) TempMessageData *data,
                   const void                               *user_data)
{
  MsgService *service = (MsgService *)user_data;
  MsgMessage *message = NULL;
  g_autoptr (GError) error = NULL;
  GList *list;

  msg_test_mock_server_start_trace (mock_server, "get-messages");

  /* List should only contain temporary message */
  list = msg_message_service_get_messages (MSG_MESSAGE_SERVICE (service), NULL, &error);
  g_assert_no_error (error);

  g_assert (g_list_length (list) == 1);

  message = MSG_MESSAGE (list->data);
  g_assert_cmpstr (msg_message_get_subject (message), ==, "Hello World");
  /* g_assert_cmpstr (msg_message_get_body_preview (message), ==, "Test :)"); */
  g_assert_cmpstr (msg_message_get_id (message), ==, msg_message_get_id (data->message));

  uhm_server_end_trace (mock_server);
}

static void
mock_server_notify_resolver_cb (GObject                             *object,
                                __attribute__ ((unused)) GParamSpec *pspec,
                                __attribute__ ((unused)) gpointer    user_data)
{
  UhmServer *server;
  UhmResolver *resolver;

  server = UHM_SERVER (object);

  resolver = uhm_server_get_resolver (server);
  if (resolver != NULL) {
    const char *ip_address = uhm_server_get_address (server);

    g_print ("Add resolver to %s\n", ip_address);
    uhm_resolver_add_A (resolver, "login.microsoftonline.com", ip_address);
    uhm_resolver_add_A (resolver, "graph.microsoft.com", ip_address);

  }
}

int
main (int    argc,
      char **argv)
{
  g_autoptr (GFile) trace_directory = NULL;
  g_autofree char *path = NULL;
  MsgAuthorizer *authorizer = NULL;
  g_autoptr (GError) error = NULL;
  int retval;

  msg_test_init (argc, argv);

  mock_server = msg_test_get_mock_server ();
  g_signal_connect (G_OBJECT (mock_server), "notify::resolver", (GCallback) mock_server_notify_resolver_cb, NULL);
  path = g_test_build_filename (G_TEST_DIST, "traces/message", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_message_service_new (authorizer));

  /* Always test authentication first, so that authorizer is set up */
  g_test_add_func ("/message/authentication", test_authentication);

  g_test_add ("/message/get/messages",
                   TempMessageData,
                   service,
                   setup_temp_message,
                   test_get_messages,
                   teardown_temp_message);

  retval = g_test_run ();

  return retval;
}
