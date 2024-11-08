#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include <src/msg.h>
#include <src/user/msg-user-service.h>

#include <stdio.h>
#include <locale.h>

static GMainLoop *loop = NULL;

void
test_message (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (MsgMessageService) service = NULL;
  g_autoptr (MsgUserService) user_service = NULL;
  g_autoptr (GError) error = NULL;
  GList *list = NULL;
  g_autofree char *out_delta_link = NULL;
  MsgMailFolder *inbox;
  MsgMailFolder *archive;
  char *id;

  auth = msg_goa_authorizer_new (goa_account);
  g_assert (auth != NULL);


  /* user_service = msg_user_service_new (MSG_AUTHORIZER (auth)); */
  /* msg_user_service_get_user (user_service, "jan-michael.brummer1@volkswagen.de", NULL, NULL); */
  /* msg_user_service_get_photo (user_service, "jan-michael.brummer1@volkswagen.de", NULL, &error); */
  /* if (error) */
  /*   g_warning ("Could not load photo: %s\n", error->message); */
  /* msg_user_service_get_user (user_service, "marcel.rothe.extern@diconium.com", NULL, NULL); */
  /* msg_user_service_get_photo (user_service, "marcel.rothe.extern@diconium.com", NULL, NULL); */
  /* return; */
/* #if 1 */
  service = msg_message_service_new (MSG_AUTHORIZER (auth));
  g_assert (service != NULL);
  char *delta_link_out = NULL;

  /* id = msg_message_service_get_folder_id (service, MSG_MESSAGE_MAIL_FOLDER_TYPE_INBOX, NULL, &error); */
  /* g_print ("WE GOT: %s\n", id); */
  /* return; */

  /* inbox = msg_message_service_get_mail_folder (service, MSG_MESSAGE_MAIL_FOLDER_TYPE_DELETED_ITEMS, NULL, &error); */
  /* g_print ("Folder: %s\n", msg_mail_folder_get_display_name (inbox)); */

  /* list = msg_message_service_get_mail_folders (service, NULL, NULL, NULL, &error); */
  /* for (GList *l = list; l && l->data; l = l->next) { */
  /*   MsgMailFolder *folder = MSG_MAIL_FOLDER (l->data); */

  /*   g_print ("Mail Folder: %s (%d/%d)\n", */
  /*            msg_mail_folder_get_display_name (folder), */
  /*            msg_mail_folder_get_unread_item_count (folder), */
  /*            msg_mail_folder_get_total_item_count (folder)); */
  /*   break; */
  /* } */

  /* return; */

  archive = msg_message_service_get_mail_folder (service, MSG_MESSAGE_MAIL_FOLDER_TYPE_DELETED_ITEMS, NULL, &error);
  list = msg_message_service_get_messages (service, archive, NULL, NULL, NULL, &out_delta_link, 1, NULL, &error);
                /* { */
                /* MsgMessage *message = MSG_MESSAGE (list->data); */
                /* GBytes *data; */
                /* g_print ("ID: %s\n", msg_message_get_id (message)); */
                /* data = msg_message_service_get_mime_message (service, message, NULL, &error); */
                /* g_print ("MIME: %s\n", g_bytes_get_data (data, NULL)); */
                /* return; */
                /* } */
  for (GList *l = list; l && l->data; l = l->next) {
    MsgMessage *message = MSG_MESSAGE (l->data);

    g_print ("=> Mail\nSender: %s\nUnread: %d\nDate: %s\nSubject: %s\nPreview: \nBody: \n\n\n",
             msg_message_get_sender (message),
             msg_message_get_unread (message),
             g_date_time_format (msg_message_get_received_date (message), "%H:%M"));
             /* msg_message_get_subject (message));, */
             /* msg_message_get_body_preview (message), */
             /* msg_message_get_body (message, NULL)); */
      /* break; */
  }
  /* user_service = msg_user_service_new (MSG_AUTHORIZER (auth)); */
  /* GBytes *photo = msg_user_service_get_photo(MSG_USER_SERVICE (user_service), "jan-michael.brummer1@volkswagen.de", NULL, NULL); */
  /*   g_print("photo: %p\n", photo); */
  /*       const char *data = g_bytes_get_data (photo, NULL); */
  /*   for (int i = 0; i < 100; i++) */
  /*     { */
  /*       g_print("%c", data[i]); */
  /*     } */
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
      gboolean mail_disabled;

      g_object_get (G_OBJECT (account),
                    "provider-type", &provider_type,
                    "provider-name", &provider_name,
                    "presentation-identity", &identity,
                    "mail-disabled", &mail_disabled,
                    NULL);

      printf ("Got an account: %s (%s)", identity, provider_name);

      if (!mail_disabled && g_strcmp0 (provider_type, "ms_graph") == 0) {
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

