#include "src/msg-authorizer.h"
#include "src/drive/msg-drive-service.h"
#include "src/drive/msg-drive-item-file.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

#define CLIENT_ID "c6ab1078-80d3-40ba-a49b-375dd013251f"
#define REDIRECT_URI ""

static MsgService *service = NULL;
static MsgDriveItem *root = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
    MsgDriveItem *item;
} TempItemData;

static MsgDriveItem *
_setup_temp_item (MsgDriveItem *item,
                  MsgService   *service,
                  GFile        *item_file)
{
  MsgDriveItem *new_item = NULL;
  GOutputStream *output_stream = NULL;
  GFileInputStream *file_stream = NULL;
  g_autoptr (GError) error = NULL;

  new_item = msg_drive_service_add_item_to_folder (MSG_DRIVE_SERVICE (service), root, item, NULL, &error);
  g_assert_no_error (error);

  output_stream = msg_drive_service_update (MSG_DRIVE_SERVICE (service), new_item, NULL, &error);
  g_assert_no_error (error);

  file_stream = g_file_read (item_file, NULL, &error);
  g_assert_no_error (error);

  g_output_stream_splice (G_OUTPUT_STREAM (output_stream),
                          G_INPUT_STREAM (file_stream),
                          G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                          NULL,
                          &error);
  g_assert_no_error (error);

  msg_drive_service_update_finish (MSG_DRIVE_SERVICE (service), new_item, output_stream, NULL, &error);
  g_assert_no_error (error);

  return new_item;
}

static void
setup_temp_item_spreadsheet (TempItemData *data,
                             const void   *user_data)
{
  g_autoptr (MsgDriveItem) item = NULL;
  g_autofree char *item_file_path = NULL;
  g_autoptr (GFile) item_file = NULL;
  MsgService *service = (MsgService *) user_data;
  g_autoptr (GError) error = NULL;
  GList *drives;

  msg_test_mock_server_start_trace (mock_server, "setup-temp-item-spreadsheet");

  drives = msg_drive_service_get_drives (MSG_DRIVE_SERVICE (service), NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (drives);

  root = msg_drive_service_get_root (MSG_DRIVE_SERVICE (service), drives->data, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (root);

  item = MSG_DRIVE_ITEM (msg_drive_item_file_new ());
  msg_drive_item_set_name (item, "Temporary Item (Spreadsheet).ods");

  item_file_path = g_test_build_filename (G_TEST_DIST, "data", "test.ods", NULL);
  g_debug ("%s\n", item_file_path);
  item_file = g_file_new_for_path (item_file_path);

  data->item = _setup_temp_item (item, MSG_SERVICE (service), item_file);

  uhm_server_end_trace (mock_server);
}

static void
teardown_temp_item (TempItemData *data,
                    const void   *user_data)
{
  MsgService *service = (MsgService *)user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "teardown-temp-item");

  if (data->item) {
    /* delete_item (data->item, MSG_SERVCE (service)); */
    msg_drive_service_delete (MSG_DRIVE_SERVICE (service), data->item, NULL, &error);
    g_clear_object (&data->item);
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
    if (!authorisation_code)
      return;
  } else {
    /* Hard-coded default to match the trace file. */
    authorisation_code = g_strdup ("4/GeYb_3HkYh4vyephp-lbvzQs1GAb.YtXAxmx-uJ0eoiIBeO6P2m9iH6kvkQI");
  }

  g_assert (msg_oauth2_authorizer_request_authorization (MSG_OAUTH2_AUTHORIZER (msg_test_get_authorizer ()), authorisation_code, NULL, NULL) == TRUE);

  msg_oauth2_authorizer_test_save_credentials (msg_test_get_authorizer ());

  uhm_server_end_trace (mock_server);
}

static void
test_delete_item (TempItemData *data,
                  const void   *user_data)
{
  MsgService *service = (MsgService *)user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "delete-item");

  msg_drive_service_delete (MSG_DRIVE_SERVICE (service), data->item, NULL, &error);
  g_assert_no_error (error);

  g_clear_object (&data->item);

  uhm_server_end_trace (mock_server);
}

/* static void */
/* test_drive (void) */
/* { */
/*   GList *drives; */
/*   g_autoptr (GError) error = NULL; */

/*   msg_test_mock_server_start_trace (mock_server, "test-drive"); */

/*   service = MSG_SERVICE (msg_drive_service_new (msg_test_get_authorizer ())); */

/*   drives = msg_drive_service_get_drives (MSG_DRIVE_SERVICE (service), NULL, &error); */
/*   g_print ("Drives: %p\n", drives); */
/*   g_print ("Error: %p\n", error); */
/*   uhm_server_end_trace (mock_server); */
/* } */

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
  path = g_test_build_filename (G_TEST_DIST, "traces/drive", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);
  g_object_unref (trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_drive_service_new (authorizer));

  /* Always test authentication first, so that authorizer is set up */
  /* g_test_add_func ("/drive/authentication", test_authentication); */

  /* g_test_add ("/drive/delete/item", */
  /*                  TempItemData, */
  /*                  service, */
  /*                  setup_temp_item_spreadsheet, */
  /*                  test_delete_item, */
  /*                  teardown_temp_item); */

  retval = g_test_run ();

  return retval;
}

