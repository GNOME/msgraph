/* Copyright 2023-2024 Jan-Michael Brummer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include <stdio.h>

#include "msg-authorizer.h"
#include "msg-error.h"
#include "msg-private.h"
#include "msg-service.h"
#include "msg-message.h"
#include "msg-mail-folder.h"
#include "msg-message-service.h"

struct _MsgMessageService {
  MsgService parent_instance;
};

G_DEFINE_TYPE (MsgMessageService, msg_message_service, MSG_TYPE_SERVICE);

static void
msg_message_service_init (__attribute__ ((unused)) MsgMessageService *self)
{
}

static void
msg_message_service_class_init (__attribute__ ((unused)) MsgMessageServiceClass *class)
{
}

MsgMessageService *
msg_message_service_new (MsgAuthorizer *authorizer)
{
  return g_object_new (MSG_TYPE_MESSAGE_SERVICE, "authorizer", authorizer, NULL);
}

/**
 * msg_message_service_get_messages
 * @self: a #MsgMessageService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get all messages for given service
 *
 * Returns: (element-type MsgMessage) (transfer full): all messages the user can access
 */
GList *
msg_message_service_get_messages (MsgMessageService  *self,
                                  GCancellable       *cancellable,
                                  GError            **error)
{
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autolist (MsgMessage) list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/messages", NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      MsgMessage *msg = NULL;
      JsonObject *message_object = NULL;

      message_object = json_array_get_object_element (array, index);

      msg = msg_message_new_from_json (message_object, &local_error);
      if (msg) {
        list = g_list_append (list, msg);
      } else {
        g_warning ("Could not parse message object: %s", local_error->message);
        g_clear_error (&local_error);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}

/**
 * msg_message_service_get_mail_folders
 * @self: a #MsgMessageService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get all folders for given service
 *
 * Returns: (element-type MsgMailFolder) (transfer full): all mail folders the user can access
 */
GList *
msg_message_service_get_mail_folders (MsgMessageService  *self,
                                      GCancellable       *cancellable,
                                      GError            **error)
{
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autolist (MsgMailFolder) list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders", NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      MsgMailFolder *folder = NULL;
      JsonObject *object = NULL;

      object = json_array_get_object_element (array, index);

      folder = msg_mail_folder_new_from_json (object, &local_error);
      if (folder) {
        list = g_list_append (list, folder);
      } else {
        g_warning ("Could not parse mail folder object: %s", local_error->message);
        g_clear_error (&local_error);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}

/**
 * msg_message_service_get_mail_folder
 * @self: a #MsgMessageService
 * @type: a #MsgMessageMailFolderType
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get a specific mail folder for given service
 *
 * Returns: (transfer full): a #MsgMailFolder
 */
MsgMailFolder *
msg_message_service_get_mail_folder (MsgMessageService         *self,
                                     MsgMessageMailFolderType   type,
                                     GCancellable              *cancellable,
                                     GError                   **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  switch (type) {
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_INBOX:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/inbox", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_DRAFTS:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/drafts", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_SENT_ITEMS:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/sentitems", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_JUNK_EMAIL:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/junkemail", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_DELETED_ITEMS:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/deleteditems", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_OUTBOX:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/outbox", NULL);
      break;
    case MSG_MESSAGE_MAIL_FOLDER_TYPE_ARCHIVE:
      url = g_strconcat (MSG_API_ENDPOINT, "/me/mailFolders/archive", NULL);
      break;
  }

  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_mail_folder_new_from_json (root_object, error);
}

/**
 * msg_message_service_delete:
 * @self: a #MsgMessageService
 * @message: a #MsgMessage
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Delets #message.
 *
 * Returns: %TRUE for succes, else &FALSE
 */
gboolean
msg_message_service_delete (MsgMessageService  *self,
                            MsgMessage         *message,
                            GCancellable       *cancellable,
                            GError            **error)
{
  g_autoptr (SoupMessage) soup_message = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return FALSE;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/messages/", msg_message_get_id (message), NULL);
  soup_message = msg_service_build_message (MSG_SERVICE (self), "DELETE", url, NULL, FALSE);
  response = msg_service_send_and_read (MSG_SERVICE (self), soup_message, cancellable, error);
  if (!response)
    return FALSE;

  return TRUE;
}

/**
 * msg_message_service_create_draft:
 * @self: a #MsgContextService
 * @message: a #MsgMessage
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Create new draft message #message and return new message object.
 *
 * Returns: (transfer full): a new #MsgMessage
 */
MsgMessage *
msg_message_service_create_draft (MsgMessageService  *self,
                                  MsgMessage         *message,
                                  GCancellable       *cancellable,
                                  GError            **error)
{
  g_autoptr (SoupMessage) soup_message = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autoptr (JsonParser) parser = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) node = NULL;
  g_autofree char *json = NULL;
  GBytes *body = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/messages", NULL);
  soup_message = msg_service_build_message (MSG_SERVICE (self), "POST", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "subject");
  json_builder_add_string_value (builder, msg_message_get_subject (message));

  json_builder_set_member_name (builder, "body");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "content");
  json_builder_add_string_value (builder, msg_message_get_body_preview (message));
  json_builder_end_object (builder);
  json_builder_end_object (builder);
  node = json_builder_get_root (builder);
  json = json_to_string (node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (soup_message, "application/json", body);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), soup_message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_message_new_from_json (root_object, error);
}
