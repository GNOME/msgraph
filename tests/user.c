#include "src/msg-authorizer.h"
#include "src/user/msg-user-service.h"
#include "src/user/msg-user.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

static MsgService *service = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
    MsgUser *user;
} TempUserData;

static void
test_finalize (void)
{
  g_autoptr (MsgUser) user = NULL;

  user = msg_user_new ();
  g_clear_object (&user);
}

static void
test_get_user (void)
{
  g_autoptr (MsgUser) user = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-user");

  user = msg_user_service_get_user (MSG_USER_SERVICE (service), NULL, NULL, &error);
  g_assert (user);

  g_assert_nonnull (msg_user_get_mail (user));
  g_clear_object (&user);

  user = msg_user_service_get_user (MSG_USER_SERVICE (service), "unknown", NULL, &error);
  g_assert (!user);

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
    uhm_resolver_add_A (resolver, "graph.microsoft.com", ip_address);
    uhm_resolver_add_A (resolver, "login.microsoftonline.com", ip_address);
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
  path = g_test_build_filename (G_TEST_DIST, "traces/user", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_user_service_new (authorizer));
  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add_func ("/user/finalize", test_finalize);
  g_test_add_func ("/user/get/user", test_get_user);

  retval = g_test_run ();

  return retval;
}
