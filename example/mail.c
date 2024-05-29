#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include <msg/msg.h>

#include <stdio.h>
#include <locale.h>

static GMainLoop *loop = NULL;

void
test_message (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (MsgMessageService) service = NULL;
  g_autoptr (GError) error = NULL;
  GList *list = NULL;
  g_autofree char *out_delta_link = NULL;
  MsgMailFolder *inbox;
  MsgMailFolder *archive;

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

  /* return; */

  archive = msg_message_service_get_mail_folder (service, MSG_MESSAGE_MAIL_FOLDER_TYPE_ARCHIVE, NULL, &error);
  list = msg_message_service_get_messages (service, archive, NULL, 0, &out_delta_link, NULL, &error);
  g_print ("out delta link: %s\n", out_delta_link);
  for (GList *l = list; l && l->data; l = l->next) {
    MsgMessage *message = MSG_MESSAGE (l->data);

    g_print ("=> Mail\nSender: %s\nUnread: %d\nDate: %s\nSubject: %s\nPreview: %s\nBody: %s\n\n\n",
             msg_message_get_sender (message),
             msg_message_get_unread (message),
             g_date_time_format (msg_message_get_received_date (message), "%H:%M"),
             msg_message_get_subject (message),
             msg_message_get_body_preview (message),
             msg_message_get_body (message, NULL));
  }
}

int
main (int    argc,
      char **argv)
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

  setlocale(LC_ALL, "en_US.utf8");

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
          test_message (object);
          /* loop = g_main_loop_new (NULL, TRUE); */
          /* g_main_loop_run (loop); */
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


  return 0;
}

