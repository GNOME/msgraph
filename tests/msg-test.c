#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>
#include "msg-authorizer.h"
#include "msg-goa-authorizer.h"
#include "contact/msg-contact.h"
#include "contact/msg-contact-service.h"
#include "drive/msg-drive-service.h"
#include "drive/msg-drive.h"
#include "drive/msg-drive-item.h"
#include "drive/msg-drive-item-file.h"
#include "message/msg-mail-folder.h"
#include "message/msg-message.h"
#include "message/msg-message-service.h"

#include <stdio.h>

#define SIZE    400000

char buffer[SIZE];
static GMainLoop *loop = NULL;

void
read_cb (GObject      *source_object,
         GAsyncResult *res,
         gpointer      user_data)
{
  GError *error = NULL;
  GInputStream *stream = G_INPUT_STREAM (source_object);
  gssize nread;

  nread = g_input_stream_read_finish (stream, res, &error);
  if (error != NULL)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      goto out;
    }

  g_print ("%s: Read %ld\n", __FUNCTION__, nread);
  g_print("%s\n", buffer);
  g_main_loop_quit (loop);

 out:
  g_debug ("- read\n");
}

static void
test_drive (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (MsgDriveService) service = NULL;
  GList *list = NULL, *list_iterator = NULL;

  auth = msg_goa_authorizer_new (goa_account);
  g_assert (auth != NULL);
  service = msg_drive_service_new (MSG_AUTHORIZER (auth));
  g_assert (service != NULL);

  list = msg_drive_service_get_drives (service, NULL, &error);
  if (error) {
      printf ("Encountered error: %s\n", error->message);
      return;
  }

  printf ("Listing drives: \n");
  for (list_iterator = list; list_iterator != NULL; list_iterator = g_list_next (list_iterator)) {
      MsgDrive *drive = MSG_DRIVE (list_iterator->data);
      g_autoptr (MsgDriveItem) root = NULL;
      GList *children, *child;

      printf (" - Drive %s\n", msg_drive_get_name (drive));
      root = msg_drive_service_get_root (service, drive, NULL, &error);
      if (!root) {
          printf ("error: %s\n\n", error->message);
          g_clear_error (&error);
          continue;
      }

      printf ("   %s: %s:\n", MSG_IS_DRIVE_ITEM_FILE (root) ? "File" : "Folder", msg_drive_item_get_name (root));

      children = msg_drive_service_list_children (service, root, NULL, &error);
      if (!children || error) {
        printf ("Could not load children: %s\n", error->message);
        continue;
      }

      for (child = children; child != NULL; child = g_list_next (child)) {
          MsgDriveItem *item = child->data;
          printf ("   └ %s\n", msg_drive_item_get_name (item));

        if (g_str_has_suffix (msg_drive_item_get_name (item), ".png")) {
          g_autoptr (GError) le = NULL;
          GInputStream *input = msg_drive_service_download_item (service, item, NULL, &le);
          GFileOutputStream *output;
          GFile *file;

          g_print ("input: %p\n", input);
          /* file = g_file_new_for_path ("./test.png"); */
          /* output = g_file_create (file, G_FILE_CREATE_NONE, NULL, &le); */
          /* g_output_stream_splice (G_OUTPUT_STREAM (output), input, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &le); */
          g_input_stream_read_async (input, buffer, SIZE -1, G_PRIORITY_DEFAULT, NULL, read_cb, NULL);
        }
      }
  }

  GList *children = msg_drive_service_get_shared_with_me (service, NULL, &error);
  if (!children || error) {
    printf ("Could not load shared with me: %s\n", error->message);
    return;
  }

  for (GList *child = children; child != NULL; child = g_list_next (child)) {
      MsgDriveItem *item = child->data;
      printf ("   └ %s\n", msg_drive_item_get_name (item));
  }

  g_clear_list (&list, g_object_unref);
}

void
test_message (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (MsgMessageService) service = NULL;
  g_autoptr (GError) error = NULL;
  GList *list = NULL;
  MsgMailFolder *inbox;

  auth = msg_goa_authorizer_new (goa_account);
  g_assert (auth != NULL);
  service = msg_message_service_new (MSG_AUTHORIZER (auth));
  g_assert (service != NULL);

  inbox = msg_message_service_get_mail_folder (service, MSG_MESSAGE_MAIL_FOLDER_TYPE_INBOX, NULL, &error);
  g_print ("Inbox Folder: %s\n", msg_mail_folder_get_display_name (inbox));

  list = msg_message_service_get_mail_folders (service, NULL, &error);
  for (GList *l = list; l && l->data; l = l->next) {
    MsgMailFolder *folder = MSG_MAIL_FOLDER (l->data);

    g_print ("Mail Folder: %s (%d/%d)\n",
             msg_mail_folder_get_display_name (folder),
             msg_mail_folder_get_unread_item_count (folder),
             msg_mail_folder_get_total_item_count (folder));
  }

  list = msg_message_service_get_messages (service, NULL, &error);
  for (GList *l = list; l && l->data; l = l->next) {
    MsgMessage *message = MSG_MESSAGE (l->data);

    g_print ("Mail Subject: %s Preview: %s\n", msg_message_get_subject (message), msg_message_get_body_preview (message));
  }
}

void
test_contact (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (MsgContactService) service = NULL;
  g_autoptr (GError) error = NULL;
  GList *list = NULL;

  auth = msg_goa_authorizer_new (goa_account);
  g_assert (auth != NULL);
  service = msg_contact_service_new (MSG_AUTHORIZER (auth));
  g_assert (service != NULL);

  list = msg_contact_service_get_contacts (service, NULL, &error);
  for (GList *l = list; l && l->data; l = l->next) {
    MsgContact *contact = MSG_CONTACT (l->data);

    g_print ("Contact: %s\n", msg_contact_get_name (contact));
  }
}

int
main (void)
{
  GoaClient *client = NULL;
  GList *accounts = NULL;
  GList *account_iterator = NULL;
  GoaObject *object = NULL;
  GoaAccount *account = NULL;
  GError *error = NULL;
  gchar *provider_type = NULL;
  gchar *provider_name = NULL;
  gchar *identity = NULL;

  client = goa_client_new_sync (NULL, &error);
  if (error != NULL) {
      fprintf (stderr, "Error creating client\n");
      return 1;
    }

  accounts = goa_client_get_accounts (client);

  for (account_iterator = accounts; account_iterator != NULL; account_iterator = account_iterator->next) {
      object = GOA_OBJECT (account_iterator->data);
      account = goa_object_get_account (object);

      g_object_get (G_OBJECT (account),
                    "provider-type", &provider_type,
                    "provider-name", &provider_name,
                    "presentation-identity", &identity, NULL);

      printf ("Got an account: %s (%s)", identity, provider_name);

      if (g_strcmp0 (provider_type, "ms_graph") == 0) {
          printf (" - matches: testing\n");
          test_drive (object);
          /* test_message (object); */
          /* test_contact (object); */
          loop = g_main_loop_new (NULL, TRUE);
        g_main_loop_run (loop);
          return 0;
      } else {
        printf ("\n");
      }

      g_free (provider_type);
      g_free (provider_name);
      g_free (identity);
      g_object_unref (account);
      g_object_unref (object);
  }



  g_list_free (accounts);

  g_object_unref (client);

  return 0;
}

#if 0
static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be split up between
	 * the different unit test suites, but that's too much effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

              g_print ("Adding graph\n");
		uhm_resolver_add_A (resolver, "graph.microsoft.com", ip_address);
	}
}

void
msg_test_mock_server_start_trace (UhmServer *server, const gchar *trace_filename)
{
  GError *child_error = NULL;

  uhm_server_start_trace (server, trace_filename, &child_error);
  g_assert_no_error (child_error);
  /* gdata_test_set_https_port (server); */
}




static void
test_authentication (void)
{
  /* MsgOAuth2Authorizer *authorizer = NULL;   */ /* owned */
  gchar *authentication_uri, *authorisation_code;

  msg_test_mock_server_start_trace (mock_server, "authentication");
  uhm_server_end_trace (mock_server);
}

static MsgAuthorizer *
create_global_authorizer (void)
{
  return MSG_AUTHORIZER (msg_dummy_authorizer_new (MSG_TYPE_DRIVE_SERVICE));
}

int
main_new(void)
{
  MsgAuthorizer *authorizer = NULL;
  MsgDriveService *service = NULL;
  gint retval;

  msg_test_init ();

  g_signal_connect (G_OBJECT (mock_server), "notify::resolver", (GCallback) mock_server_notify_resolver_cb, NULL);

  /* authorizer = create_global_authorizer (); */
  service = MSG_DRIVE_SERVICE (msg_drive_service_new (authorizer));

  g_test_add_func ("/drive/authentication", test_authentication);

  retval = g_test_run ();

  if (service != NULL)
    g_object_unref (service);

  return retval;
}
#endif
