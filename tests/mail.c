#include "src/msg-authorizer.h"
#include "src/mail/msg-mail-folder.h"
#include "src/mail/msg-mail-service.h"
#include "src/mail/msg-mail-message.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

static MsgService *service = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
  MsgMailFolder *draft_folder;
  MsgMailMessage *message;
} TempMessageData;

static void
setup_temp_message (TempMessageData *data,
                    const void      *user_data)
{
  g_autoptr (MsgMailMessage) message = NULL;
  g_autoptr (MsgMailMessage) new_message = NULL;
  MsgService *service = (MsgService *) user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "setup-temp-message");

  data->draft_folder = msg_mail_service_get_mail_folder (MSG_MAIL_SERVICE (service), MSG_MAIL_FOLDER_TYPE_DRAFTS, NULL, &error);
  g_assert_no_error (error);
  g_assert (data->draft_folder);

  message = MSG_MAIL_MESSAGE (msg_mail_message_new ());
  msg_mail_message_set_subject (message, "Hello World");
  msg_mail_message_set_receiver (message, "max@mustermann.de");
  msg_mail_message_set_sender (message, "info@example.com");
  g_assert_no_error (error);
  msg_mail_message_set_body (message, "Test :)");
  g_assert_no_error (error);

  data->message = msg_mail_service_create_draft_message (MSG_MAIL_SERVICE (service), message, NULL, &error);
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
    msg_mail_service_delete_message (MSG_MAIL_SERVICE (service), data->message, NULL, &error);
    g_clear_object (&data->message);
  }

  g_clear_object (&data->draft_folder);

  uhm_server_end_trace (mock_server);
}

static void
test_get_messages (__attribute__ ((unused)) TempMessageData *data,
                   const void                               *user_data)
{
  MsgService *service = (MsgService *)user_data;
  MsgMailMessage *message = NULL;
  g_autoptr (GError) error = NULL;
  GList *list;

  msg_test_mock_server_start_trace (mock_server, "get-messages");

  /* List should only contain temporary message */
  list = msg_mail_service_get_messages (MSG_MAIL_SERVICE (service), data->draft_folder, NULL, NULL, NULL, NULL, 0, NULL, &error);
  g_assert_no_error (error);

  g_assert (g_list_length (list) == 1);

  message = MSG_MAIL_MESSAGE (list->data);
  g_assert_cmpstr (msg_mail_message_get_subject (message), ==, "Hello World");
  g_assert_cmpstr (msg_mail_message_get_id (message), ==, msg_mail_message_get_id (data->message));

  uhm_server_end_trace (mock_server);
}

void
test_get_folders (void)
{
  g_autoptr (GError) error = NULL;
  g_autolist (MsgMailFolder) folders = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-folders");
  folders = msg_mail_service_get_mail_folders (MSG_MAIL_SERVICE (service), NULL, NULL, NULL, &error);
  g_assert_nonnull (folders);

  uhm_server_end_trace (mock_server);
}

void
test_finalize (void)
{
  g_autoptr (MsgMailFolder) folder = NULL;
  g_autoptr (MsgMailMessage) message = NULL;

  folder = msg_mail_folder_new ();
  g_clear_object (&folder);

  message = msg_mail_message_new ();
  msg_mail_message_set_sender (message, "Test Sender");
  msg_mail_message_set_cc (message, "Test CC");
  g_clear_object (&message);
}

void
test_folder_attributes (void)
{
  g_autoptr (MsgMailFolder) folder = NULL;

  folder = msg_mail_folder_new ();
  g_assert (folder);

  g_assert (msg_mail_folder_get_unread_item_count (folder) == 0);
  msg_mail_folder_set_unread_item_count (folder, 42);
  g_assert (msg_mail_folder_get_unread_item_count (folder) == 42);

  g_assert (msg_mail_folder_get_total_item_count (folder) == 0);
  msg_mail_folder_set_total_item_count (folder, 24);
  g_assert (msg_mail_folder_get_total_item_count (folder) == 24);

  g_assert_cmpstr (msg_mail_folder_get_id (folder), ==, NULL);
  msg_mail_folder_set_id (folder, "nonsenses");
  g_assert_cmpstr (msg_mail_folder_get_id (folder), ==, "nonsenses");
  msg_mail_folder_set_id (folder, "");

  g_assert_cmpstr (msg_mail_folder_get_display_name (folder), ==, NULL);
  msg_mail_folder_set_display_name (folder, "test-folder");
  g_assert_cmpstr (msg_mail_folder_get_display_name (folder), ==, "test-folder");
  msg_mail_folder_set_display_name (folder, "");

  g_assert (msg_mail_folder_get_folder_type (folder) == MSG_MAIL_FOLDER_TYPE_OTHER);
  msg_mail_folder_set_folder_type (folder, MSG_MAIL_FOLDER_TYPE_DRAFTS);
  g_assert (msg_mail_folder_get_folder_type (folder) == MSG_MAIL_FOLDER_TYPE_DRAFTS);

  g_assert_cmpstr (msg_mail_folder_get_delta_link (folder), ==, NULL);
  msg_mail_folder_set_delta_link (folder, "url://somewhere");
  g_assert_cmpstr (msg_mail_folder_get_delta_link (folder), ==, "url://somewhere");
  msg_mail_folder_set_delta_link (folder, "");
}

void
test_message_attributes (void)
{
  g_autoptr (MsgMailMessage) message = NULL;
  g_autoptr (GDateTime) date = NULL;
  gboolean is_html;

  message = msg_mail_message_new ();
  g_assert (message);

  g_assert_cmpstr (msg_mail_message_get_body_preview (message), ==, NULL);
  msg_mail_message_set_body_preview (message, "test-body-preview");
  g_assert_cmpstr (msg_mail_message_get_body_preview (message), ==, "test-body-preview");
  msg_mail_message_set_body_preview (message, "");

  g_assert_cmpstr (msg_mail_message_get_body (message, NULL), ==, NULL);
  msg_mail_message_set_body (message, "test-body");
  g_assert_cmpstr (msg_mail_message_get_body (message, NULL), ==, "test-body");
  g_assert_cmpstr (msg_mail_message_get_body (message, &is_html), ==, "test-body");
  g_assert (is_html == TRUE);

  msg_mail_message_set_body (message, msg_mail_message_get_body (message, NULL));
  msg_mail_message_set_body (message, NULL);

  g_assert_cmpstr (msg_mail_message_get_subject (message), ==, NULL);
  msg_mail_message_set_subject (message, "test-subject");
  g_assert_cmpstr (msg_mail_message_get_subject (message), ==, "test-subject");

  msg_mail_message_set_subject (message, msg_mail_message_get_subject (message));
  msg_mail_message_set_subject (message, NULL);

  g_assert_cmpstr (msg_mail_message_get_id (message), ==, NULL);
  msg_mail_message_set_id (message, "test-id");
  g_assert_cmpstr (msg_mail_message_get_id (message), ==, "test-id");

  msg_mail_message_set_id (message, msg_mail_message_get_id (message));
  msg_mail_message_set_id (message, NULL);

  g_assert_cmpstr (msg_mail_message_get_sender (message), ==, NULL);
  msg_mail_message_set_sender (message, "test-sender");
  g_assert_cmpstr (msg_mail_message_get_sender (message), ==, "test-sender");

  msg_mail_message_set_sender (message, msg_mail_message_get_sender (message));
  msg_mail_message_set_sender (message, NULL);

  g_assert_cmpstr (msg_mail_message_get_receiver (message), ==, NULL);
  msg_mail_message_set_receiver (message, "test-receiver");
  g_assert_cmpstr (msg_mail_message_get_receiver (message), ==, "test-receiver");

  msg_mail_message_set_receiver (message, msg_mail_message_get_receiver (message));
  msg_mail_message_set_receiver (message, NULL);

  g_assert_cmpstr (msg_mail_message_get_cc (message), ==, NULL);
  msg_mail_message_set_cc (message, "test-cc");
  g_assert_cmpstr (msg_mail_message_get_cc (message), ==, "test-cc");

  msg_mail_message_set_cc (message, msg_mail_message_get_cc (message));
  msg_mail_message_set_cc (message, NULL);

  g_assert (msg_mail_message_get_unread (message) == 0);
  msg_mail_message_set_unread (message, 1);
  g_assert (msg_mail_message_get_unread (message) == 1);
  msg_mail_message_set_unread (message, 0);

  g_assert (msg_mail_message_get_received_date (message) == NULL);
  date = g_date_time_new_local (2024, 11, 25, 14,20, 12);
  msg_mail_message_set_received_date (message, g_date_time_to_unix (date));
  g_assert (g_date_time_compare (msg_mail_message_get_received_date (message), date) == 0);
  msg_mail_message_set_received_date (message, 0);

  g_assert (msg_mail_message_get_has_attachment (message) == FALSE);
  msg_mail_message_set_has_attachment (message, TRUE);
  g_assert (msg_mail_message_get_has_attachment (message) == TRUE);
  msg_mail_message_set_has_attachment (message, FALSE);
}

static void
test_create_message (__attribute__ ((unused)) TempMessageData *data,
                     const void                               *user_data)
{
  MsgService *service = (MsgService *)user_data;
  MsgMailMessage *message = NULL;
  g_autoptr (GError) error = NULL;
  GList *list;
  g_autoptr (GBytes) mime = NULL;

  msg_test_mock_server_start_trace (mock_server, "create-message");

  /* List should only contain temporary message */
  list = msg_mail_service_get_messages (MSG_MAIL_SERVICE (service), data->draft_folder, NULL, NULL, NULL, NULL, 0, NULL, &error);
  g_assert_no_error (error);
  g_assert (g_list_length (list) == 1);

  message = MSG_MAIL_MESSAGE (list->data);
  g_assert_cmpstr (msg_mail_message_get_subject (message), ==, "Hello World");

  mime = msg_mail_service_get_mime_message (MSG_MAIL_SERVICE (service), message, NULL, &error);;
  g_assert_no_error (error);
  g_assert (mime != NULL);

  uhm_server_end_trace (mock_server);
}

struct MailFolders {
  MsgMailFolderType type;
  const char *name;
  int unread;
  int total;
};

struct MailFolders folders[7] = {
  {
    .type = MSG_MAIL_FOLDER_TYPE_INBOX,
    .name = "Posteingang",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_DRAFTS,
    .name = "Entw\303\274rfe",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_SENT_ITEMS,
    .name = "Gesendete Elemente",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_JUNK_EMAIL,
    .name = "Junk-E-Mail",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_DELETED_ITEMS,
    .name = "Gel\303\266schte Elemente",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_OUTBOX,
    .name = "Postausgang",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MAIL_FOLDER_TYPE_ARCHIVE,
    .name = "Archiv",
    .unread = 0,
    .total = 0,
  }
};

void
test_get_folder (void)
{
  g_autoptr (GError) error = NULL;
  MsgMailFolder *folder = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-folder");

  for (int i = 0; i < 7; i++) {
    folder = msg_mail_service_get_mail_folder (MSG_MAIL_SERVICE (service), folders[i].type, NULL, &error);
    g_assert_nonnull (folder);

    g_assert_cmpstr (msg_mail_folder_get_display_name (folder), ==, folders[i].name);
    g_assert (msg_mail_folder_get_unread_item_count (folder) == 0);
    g_assert (msg_mail_folder_get_total_item_count (folder) == 0);

    g_clear_object (&folder);
  }

  g_clear_object (&folder);

  uhm_server_end_trace (mock_server);
}

void
test_get_folder_id (void)
{
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-folder-id");

  for (int i = 0; i < 7; i++) {
    g_autofree char *id = msg_mail_service_get_folder_id (MSG_MAIL_SERVICE (service), folders[i].type, NULL, &error);
    g_assert_nonnull (id);
  }

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
  path = g_test_build_filename (G_TEST_DIST, "traces/mail", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_mail_service_new (authorizer));

  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add_func ("/mailservice/finalize", test_finalize);
  g_test_add_func ("/mailservice/folder/attributes", test_folder_attributes);
  g_test_add_func ("/mailservice/get/folders", test_get_folders);
  g_test_add_func ("/mailservice/get/folder", test_get_folder);

  g_test_add_func ("/mail_service/message/attributes", test_message_attributes);
  g_test_add ("/mailservice/get/create",
                   TempMessageData,
                   service,
                   setup_temp_message,
                   test_create_message,
                   teardown_temp_message);

  g_test_add ("/mailservice/get/messages",
                   TempMessageData,
                   service,
                   setup_temp_message,
                   test_get_messages,
                   teardown_temp_message);

  g_test_add_func ("/mailservice/get/folder_id", test_get_folder_id);

  retval = g_test_run ();

  return retval;
}
