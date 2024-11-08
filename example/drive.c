#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include <msg/msg.h>
/* #include <drive/msg-site.h> */
/* #include <src/drive/msg-drive-service.h> */

/* #include <src/message/msg-message-service.h> */

#include <stdio.h>
#include <locale.h>

static GMainLoop *loop = NULL;

void
test_drive (GoaObject *goa_account)
{
  MsgGoaAuthorizer *auth = NULL;
  g_autoptr (MsgDriveService) service = NULL;
  g_autoptr (GError) error = NULL;
  GList *drives = NULL;
  GList *sites = NULL;
  MsgDriveItemFile *item;
  MsgDriveItem *new_item;

  auth = msg_goa_authorizer_new (goa_account);
  g_assert (auth != NULL);
  service = msg_drive_service_new (MSG_AUTHORIZER (auth));
  g_assert (service != NULL);

  /* item = msg_drive_item_file_new (); */
  /* msg_drive_item_set_name (MSG_DRIVE_ITEM (item), "test"); */
  /* new_item = msg_drive_service_add_item_to_folder (service, parent, MSG_DRIVE_ITEM (item), NULL, &error); */

  /* msg_drive_service_  */

  /* return; */

  g_print ("Query drives:\n");
  drives =  msg_drive_service_get_drives (service, NULL, &error);
  for (GList *list = drives; list && list->data; list = list->next) {
      MsgDrive *drive = MSG_DRIVE (list->data);
      MsgDriveItem *root;
      GList *childs;
      const char *etag = NULL;
      static gboolean skip = TRUE;

    if (skip) {
      skip = FALSE;
      continue;
    }

      g_print (" - %s\n", msg_drive_get_name (drive));
      root = msg_drive_service_get_root (service, drive, NULL, NULL);
      childs = msg_drive_service_list_children (service, root, NULL, NULL);

      /* continue; */
      for (GList *l = childs; l && l->data; l = l->next) {
        MsgDriveItem *i = MSG_DRIVE_ITEM (l->data);
        g_print ("   |- %s\n", msg_drive_item_get_name (i));
        if (MSG_IS_DRIVE_ITEM_FOLDER (i) && g_strcmp0(msg_drive_item_get_name (i), "Documents") == 0) {
          childs = msg_drive_service_list_children (service, i, NULL, NULL);
          g_print ("FOLDER\n");
          etag = msg_drive_item_get_etag (i);

          /* etag = "c:{881BD4D3-EC58-4B64-A1C1-D8786CE1BF54},0"; */
          /* etag = "{881BD4D3-EC58-4B64-A1C1-D8786CE1BF54}"; */
          etag = "{0F5A0FD5-9251-4E98-9365-1A04D64FE636},1";
          g_print ("etag: %s\n", etag);
          /* childs = msg_drive_service_list_children_etag (service, i, etag, NULL, NULL); */
          return;
        }
        /* msg_drive_service_file_can_write (service, i, NULL, NULL); */
      }
      break;
  }
  return;

  /* msg_drive_service_get_sites_drives (service, NULL, &error); */
  /* return; */

  /* g_print ("Query followed sites:\n"); */
  /* sites = msg_drive_service_get_followed_sites (service, NULL, &error); */
  /* if (error) { */
  /*   g_warning ("Could not query followed sites: %s", error->message); */
  /* } */

  /* for (GList *list = sites; list && list->data; list = list->next) { */
  /*   MsgSite *site = MSG_SITE (list->data); */

  /*   g_print ("Site: %s Id %s\n", msg_site_get_name (site), msg_site_get_id (site)); */
  /* } */
  /* return; */

  /* g_print ("Query site drives:\n"); */
  /* drives = msg_drive_service_get_drives (service, NULL, NULL); */
  /* for (GList *list = drives; list && list->data; list = list->next) { */
  /*   MsgDrive *drive = MSG_DRIVE (list->data); */
  /*   MsgDriveItem *root; */
  /*   GList *childs; */

  /*   g_print (" - %s\n", msg_drive_get_name (drive)); */
  /*   root = msg_drive_service_get_root (service, drive, NULL, NULL); */
  /*   childs = msg_drive_service_list_children (service, root, NULL, NULL); */
  /*   for (GList *l = childs; l && l->data; l = l->next) { */
  /*     MsgDriveItem *i = MSG_DRIVE_ITEM (l->data); */
  /*     g_print ("   |- %s\n", msg_drive_item_get_name (i)); */

  /*   } */
  /* } */
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

  int i = 0;

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

          if (i++ == 0) {
            test_drive (object);
            return 0;
          }
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

