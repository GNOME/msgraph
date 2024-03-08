#include "src/msg-authorizer.h"
#include "src/message/msg-mail-folder.h"
#include "src/message/msg-message-service.h"
#include "src/message/msg-message.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

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
  msg_message_set_subject (message, "Hello World");
  g_assert_no_error (error);
  msg_message_set_body (message, "Test :)");
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

void
test_get_folders (void)
{
  g_autoptr (GError) error = NULL;
  g_autolist (MsgMailFolder) folders = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-folders");
  folders = msg_message_service_get_mail_folders (MSG_MESSAGE_SERVICE (service), NULL, &error);
  g_assert_nonnull (folders);

  uhm_server_end_trace (mock_server);
}

void
test_finalize (void)
{
  g_autoptr (MsgMailFolder) folder = NULL;
  g_autoptr (MsgMessage) message = NULL;

  folder = msg_mail_folder_new ();
  g_clear_object (&folder);

  message = msg_message_new ();
  g_clear_object (&message);
}

struct MailFolders {
  MsgMessageMailFolderType type;
  const char *name;
  int unread;
  int total;
};

struct MailFolders folders[7] = {
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_INBOX,
    .name = "Posteingang",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_DRAFTS,
    .name = "Entw\303\274rfe",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_SENT_ITEMS,
    .name = "Gesendete Elemente",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_JUNK_EMAIL,
    .name = "Junk-E-Mail",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_DELETED_ITEMS,
    .name = "Gel\303\266schte Elemente",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_OUTBOX,
    .name = "Postausgang",
    .unread = 0,
    .total = 0,
  },
  {
    .type = MSG_MESSAGE_MAIL_FOLDER_TYPE_ARCHIVE,
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
    folder = msg_message_service_get_mail_folder (MSG_MESSAGE_SERVICE (service), folders[i].type, NULL, &error);
    g_assert_nonnull (folder);

    g_assert_cmpstr (msg_mail_folder_get_display_name (folder), ==, folders[i].name);
    g_assert (msg_mail_folder_get_unread_item_count (folder) == 0);
    g_assert (msg_mail_folder_get_total_item_count (folder) == 0);

    g_clear_object (&folder);
  }

  g_clear_object (&folder);

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
  path = g_test_build_filename (G_TEST_DIST, "traces/message", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_message_service_new (authorizer));

  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add_func ("/message/finailize", test_finalize);

  g_test_add ("/message/get/messages",
                   TempMessageData,
                   service,
                   setup_temp_message,
                   test_get_messages,
                   teardown_temp_message);

  g_test_add_func ("/message/get/folders", test_get_folders);
  g_test_add_func ("/message/get/folder", test_get_folder);

  retval = g_test_run ();

  return retval;
}
