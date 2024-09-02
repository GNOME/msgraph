#include "src/msg-authorizer.h"
#include "src/drive/msg-drive-service.h"
#include "src/drive/msg-drive-item-file.h"
#include "src/msg-service.h"
#include "src/msg-input-stream.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

static MsgService *service = NULL;
static MsgDriveItem *root = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
  GMainLoop *loop;
  GCancellable *cancellable;
  MsgDriveItem *test_dir;
  MsgDriveItem *item;
} TempItemData;

static MsgDriveItem *
_setup_temp_item (MsgDriveItem *item,
                  MsgService   *service,
                  TempItemData *data,
                  GFile        *item_file)
{
  g_autoptr (MsgDriveItem) test_dir = NULL;
  g_autoptr (MsgDriveItem) new_item = NULL;
  MsgDriveItem *ret = NULL;
  GOutputStream *output_stream = NULL;
  GFileInputStream *file_stream = NULL;
  g_autoptr (GError) error = NULL;

  data->test_dir = msg_drive_service_create_folder (MSG_DRIVE_SERVICE (service), root, "test", data->cancellable, &error);
  g_assert_no_error (error);

  new_item = msg_drive_service_add_item_to_folder (MSG_DRIVE_SERVICE (service), data->test_dir, item, data->cancellable, &error);
  g_assert_no_error (error);

  output_stream = msg_drive_service_update (MSG_DRIVE_SERVICE (service), new_item, data->cancellable, &error);
  g_assert_no_error (error);

  file_stream = g_file_read (item_file, data->cancellable, &error);
  g_assert_no_error (error);

  g_output_stream_splice (G_OUTPUT_STREAM (output_stream),
                          G_INPUT_STREAM (file_stream),
                          G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                          data->cancellable,
                          &error);
  g_assert_no_error (error);

  ret = msg_drive_service_update_finish (MSG_DRIVE_SERVICE (service), new_item, output_stream, data->cancellable, &error);
  g_assert_no_error (error);

  return ret;
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
  g_autolist (MsgDrive) drives = NULL;

  data->cancellable = g_cancellable_new ();

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
  item_file = g_file_new_for_path (item_file_path);

  data->item = _setup_temp_item (item, MSG_SERVICE (service), data, item_file);

  uhm_server_end_trace (mock_server);
}

static void
teardown_temp_item (TempItemData *data,
                    const void   *user_data)
{
  MsgService *service = (MsgService *)user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "teardown-temp-item");

  g_cancellable_cancel (data->cancellable);
  g_clear_object (&data->cancellable);

  if (data->item) {
    /* delete_item (data->item, MSG_SERVCE (service)); */
    msg_drive_service_delete (MSG_DRIVE_SERVICE (service), data->item, NULL, &error);
    g_clear_object (&data->item);
  }

  if (data->test_dir) {
    msg_drive_service_delete (MSG_DRIVE_SERVICE (service), data->test_dir, NULL, &error);
    g_clear_object (&data->test_dir);
  }

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

static void
test_item_properties (TempItemData                        *data,
                      __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *id = NULL;
  g_autofree char *parent_id = NULL;

  /* Shared */
  g_assert (!msg_drive_item_is_shared (data->item));

  /* ID */
  id = g_strdup (msg_drive_item_get_id (data->item));
  msg_drive_item_set_id (data->item, "id");
  g_assert_cmpstr (msg_drive_item_get_id (data->item), ==, "id");
  msg_drive_item_set_id (data->item, id);

  /* Parent ID */
  parent_id = g_strdup (msg_drive_item_get_parent_id (data->item));
  msg_drive_item_set_parent_id (data->item, "parent_id");
  g_assert_cmpstr (msg_drive_item_get_parent_id (data->item), ==, "parent_id");
  msg_drive_item_set_parent_id (data->item, parent_id);

  /* Size */
  g_assert (msg_drive_item_get_size (data->item) == 7564);
  g_assert_cmpstr (msg_drive_item_get_user (MSG_DRIVE_ITEM (data->item)), ==, "Jan Brummer");
  g_assert_nonnull (msg_drive_item_get_etag (MSG_DRIVE_ITEM (data->item)));
  g_assert (msg_drive_item_get_modified (MSG_DRIVE_ITEM (data->item)) != 0);
  g_assert (msg_drive_item_get_created (MSG_DRIVE_ITEM (data->item)) != 0);

  g_clear_object (&data->item);
}

static void
test_item_file_properties (TempItemData                        *data,
                           __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GError) error = NULL;

  g_assert_cmpstr (msg_drive_item_file_get_mime_type (MSG_DRIVE_ITEM_FILE (data->item)), ==, "application/vnd.oasis.opendocument.spreadsheet");
  g_assert_cmpstr (msg_drive_item_file_get_thumbnail_uri (MSG_DRIVE_ITEM_FILE (data->item)), ==, NULL);

  g_clear_object (&data->item);
}

static void
test_item_file_rename (TempItemData                        *data,
                       __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "rename-item");

  data->item = msg_drive_service_rename (MSG_DRIVE_SERVICE (service), data->item, "renamed", NULL, &error);
  g_assert (data->item);
  g_assert_cmpstr (msg_drive_item_get_name (data->item), ==, "renamed");

  g_clear_object (&data->item);

  uhm_server_end_trace (mock_server);
}

static void
test_item_file_copy (TempItemData                        *data,
                     __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GError) error = NULL;
  gboolean ret;

  msg_test_mock_server_start_trace (mock_server, "copy-item");

  ret = msg_drive_service_copy_file (MSG_DRIVE_SERVICE (service), data->item, root, NULL, &error);
  g_assert (ret == TRUE);

  g_clear_object (&data->item);

  uhm_server_end_trace (mock_server);
}

/* static void */
/* test_item_file_move (TempItemData                        *data, */
/*                      __attribute__ ((unused)) const void *user_data) */
/* { */
/*   g_autoptr (GError) error = NULL; */
/*   g_autoptr (MsgDriveItem) item = NULL; */

/*   msg_test_mock_server_start_trace (mock_server, "move-item"); */

/*   item = msg_drive_service_move_file (MSG_DRIVE_SERVICE (service), data->item, root, NULL, &error); */
/*   g_assert (item); */

/*   g_clear_object (&data->item); */

/*   uhm_server_end_trace (mock_server); */
/* } */

static void
on_splice_done (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
  GOutputStream *stream = G_OUTPUT_STREAM (source_object);
  TempItemData *data = user_data;
  g_autoptr (GError) error = NULL;

  g_assert_true (g_main_loop_get_context (data->loop) == g_main_context_get_thread_default ());

  gssize size = g_output_stream_splice_finish (stream, res, &error);
  /* We are receiving a gzip compressed file, so we cannot compare sizes directly... */
  g_assert (size != 0);
  /* g_assert (msg_drive_item_get_size (data->item) == (guint64) size); */

  g_main_loop_quit (data->loop);
}

static void
download_thread (GTask                         *task,
                 __attribute__ ((unused)) void *source,
                 void                          *user_data,
                 GCancellable                  *cancellable)
{
  TempItemData *data = user_data;
  g_autoptr (GError) error = NULL;
  g_autoptr (GInputStream) input = NULL;
  g_autoptr (GOutputStream) output = NULL;
  GMainContext *context;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  data->loop = g_main_loop_new (context, FALSE);

  msg_test_mock_server_start_trace (mock_server, "download-item");

  input = msg_drive_service_download_item (MSG_DRIVE_SERVICE (service), data->item, cancellable, &error);
  g_assert (input);

  output = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  g_output_stream_splice_async (output, input, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, cancellable, on_splice_done, data);

  g_main_loop_run (data->loop);
  g_main_loop_unref (data->loop);

  uhm_server_end_trace (mock_server);

  g_task_return_boolean (task, TRUE);
  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);
}

static void
test_item_file_download (TempItemData                        *data,
                         __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GTask) task = NULL;

  task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_task_data (task, data, NULL);
  g_task_run_in_thread_sync (task, download_thread);
}

static void
download_io_thread (GTask                         *task,
                    __attribute__ ((unused)) void *source,
                    void                          *user_data,
                    GCancellable                  *cancellable)
{
  TempItemData *data = user_data;
  g_autoptr (GError) error = NULL;
  g_autoptr (GInputStream) input = NULL;
  g_autoptr (GOutputStream) output = NULL;
  GMainContext *context;
  char buffer[16];

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);

  data->loop = g_main_loop_new (context, FALSE);

  msg_test_mock_server_start_trace (mock_server, "download-io-item");

  input = msg_drive_service_download_item (MSG_DRIVE_SERVICE (service), data->item, cancellable, &error);
  g_assert (input);

  g_assert (g_seekable_can_seek (G_SEEKABLE (input)));
  g_assert (g_seekable_tell (G_SEEKABLE (input)) == 0);
  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_CUR, NULL, &error);
  g_assert_no_error (error);

  g_input_stream_read (input, buffer, sizeof (buffer), cancellable, &error);
  g_assert_no_error (error);

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_END, NULL, &error);
  g_assert_no_error (error);

  g_assert (g_seekable_can_truncate (G_SEEKABLE (input)) == FALSE);
  g_assert (g_seekable_truncate (G_SEEKABLE (input), 0, NULL, NULL) == FALSE);

  g_assert (msg_input_stream_get_message (input));

  uhm_server_end_trace (mock_server);

  g_task_return_boolean (task, TRUE);
  g_main_context_pop_thread_default (context);
  g_main_context_unref (context);
}

static void
test_item_file_download_io (TempItemData                        *data,
                            __attribute__ ((unused)) const void *user_data)
{
  g_autoptr (GTask) task = NULL;

  task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_task_data (task, data, NULL);
  g_task_run_in_thread_sync (task, download_io_thread);
}

static void
test_drive (void)
{
  g_autolist (MsgDrive) drives = NULL;
  g_autoptr (GError) error = NULL;
  MsgDrive *drive;

  msg_test_mock_server_start_trace (mock_server, "test-drive");
  drives = msg_drive_service_get_drives (MSG_DRIVE_SERVICE (service), NULL, &error);
  g_assert_nonnull (drives);

  drive = MSG_DRIVE (drives->data);
  g_assert (msg_drive_get_drive_type (drive) == MSG_DRIVE_TYPE_PERSONAL);
  g_assert_cmpstr (msg_drive_get_name (drive), ==, NULL);
  g_assert (msg_drive_get_total (drive) != 0);
  g_assert (msg_drive_get_used (drive) != 0);
  g_assert (msg_drive_get_remaining (drive) != 0);
  g_assert (!msg_drive_get_created (drive));
  g_assert (!msg_drive_get_modified (drive));

  /* Check finailize */
  drive = msg_drive_new ();
  g_clear_object (&drive);

  uhm_server_end_trace (mock_server);
}

static void
test_shared_with_me (void)
{
  g_autolist (MsgDriveItem) shared_items = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "test-shared-with-me");
  shared_items = msg_drive_service_get_shared_with_me (MSG_DRIVE_SERVICE (service), NULL, &error);
  g_assert_nonnull (shared_items);

  uhm_server_end_trace (mock_server);
}

static void
test_download_url (void)
{
  g_autoptr (GInputStream) stream = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GOutputStream) output = NULL;

  msg_test_mock_server_start_trace (mock_server, "download-url");

  stream = msg_drive_service_download_url (MSG_DRIVE_SERVICE (service), "https://graph.microsoft.com/v1.0/", NULL, &error);
  g_assert_nonnull (stream);

  output = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  g_output_stream_splice (G_OUTPUT_STREAM (output), stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL);

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
    /* uhm_resolver_add_A (resolver, "login.microsoftonline.com", ip_address); */
    uhm_resolver_add_A (resolver, "graph.microsoft.com", ip_address);
    uhm_resolver_add_A (resolver, "npwwvq.am.files.1drv.com", ip_address);
    uhm_resolver_add_A (resolver, "iqfaiw.am.files.1drv.com", ip_address);
  }
}

void
test_create_folder (void)
{
  g_autoptr (GError) error = NULL;
  MsgDriveItem *folder = NULL;
  g_autolist (MsgDrive) drives = NULL;
  g_autoptr (MsgDriveItem) root = NULL;

  msg_test_mock_server_start_trace (mock_server, "create-folder");

  drives = msg_drive_service_get_drives (MSG_DRIVE_SERVICE (service), NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (drives);

  root = msg_drive_service_get_root (MSG_DRIVE_SERVICE (service), drives->data, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (root);

  folder = msg_drive_service_create_folder (MSG_DRIVE_SERVICE (service), root, "Folder", NULL, &error);
  g_assert (folder);

  g_assert_cmpstr (msg_drive_item_get_name (folder), ==, "Folder");

  g_assert (msg_drive_service_delete (MSG_DRIVE_SERVICE (service), folder, NULL, &error));

  g_clear_object (&folder);

  uhm_server_end_trace (mock_server);
}

void
test_list_folder (void)
{
  g_autoptr (GError) error = NULL;
  g_autolist (MsgDriveItem) children = NULL;
  g_autolist (MsgDrive) drives = NULL;
  g_autoptr (MsgDriveItem) root = NULL;

  msg_test_mock_server_start_trace (mock_server, "list-folder");

  drives = msg_drive_service_get_drives (MSG_DRIVE_SERVICE (service), NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (drives);

  root = msg_drive_service_get_root (MSG_DRIVE_SERVICE (service), drives->data, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (root);

  children = msg_drive_service_list_children (MSG_DRIVE_SERVICE (service), root, NULL, &error);
  g_assert (children);

  uhm_server_end_trace (mock_server);
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

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_drive_service_new (authorizer));
  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add_func ("/drive/properties", test_drive);
  g_test_add_func ("/drive/shared_with_me", test_shared_with_me);

  g_test_add ("/drive/item/properties",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_properties,
                   teardown_temp_item);

  g_test_add ("/drive/item/file/properties",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_file_properties,
                   teardown_temp_item);

  g_test_add ("/drive/item/delete/file",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_delete_item,
                   teardown_temp_item);

  g_test_add_func ("/drive/create/folder", test_create_folder);
  g_test_add_func ("/drive/list/folder", test_list_folder);

  g_test_add ("/drive/item/file/download/io",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_file_download_io,
                   teardown_temp_item);

  g_test_add ("/drive/item/file/download",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_file_download,
                   teardown_temp_item);

  g_test_add_func ("/drive/download", test_download_url);

  g_test_add ("/drive/item/file/rename",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_file_rename,
                   teardown_temp_item);

  g_test_add ("/drive/item/file/copy",
                   TempItemData,
                   service,
                   setup_temp_item_spreadsheet,
                   test_item_file_copy,
                   teardown_temp_item);

  /* g_test_add ("/drive/item/file/move", */
  /*                  TempItemData, */
  /*                  service, */
  /*                  setup_temp_item_spreadsheet, */
  /*                  test_item_file_move, */
  /*                  teardown_temp_item); */

  retval = g_test_run ();

  g_clear_object (&service);
  g_clear_object (&root);
  g_clear_object (&mock_server);

  return retval;
}

