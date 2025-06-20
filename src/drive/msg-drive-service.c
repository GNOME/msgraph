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

#include "msg-authorizer.h"
#include "msg-error.h"
#include "msg-input-stream.h"
#include "msg-private.h"
#include "msg-service.h"
#include "drive/msg-drive.h"
#include "drive/msg-drive-item.h"
#include "drive/msg-drive-item-file.h"
#include "drive/msg-drive-service.h"

struct _MsgDriveService {
  MsgService parent_instance;

  MsgDriveType type;
};

G_DEFINE_TYPE (MsgDriveService, msg_drive_service, MSG_TYPE_SERVICE);

static gboolean
is_valid_name (const char *name)
{
  g_auto (GStrv) split = NULL;
  const char *invalid_names[] = {
    ".lock",
    "CON",
    "PRN",
    "AUX",
    "NUL",
    "COM0",
    "COM1",
    "COM2",
    "COM3",
    "COM4",
    "COM5",
    "COM6",
    "COM7",
    "COM8",
    "COM9",
    "LPT0",
    "LPT1",
    "LPT2",
    "LPT3",
    "LPT4",
    "LPT5",
    "LPT6",
    "LPT7",
    "LPT8",
    "LPT9",
    "_vti_",
    "desktop.ini",
    NULL
  };
  g_autofree char *lower_name = g_utf8_strdown (name, -1);

  /* Check for invalid names */
  for (int i = 0; invalid_names[i] != NULL; i++) {
    g_autofree char *invalid_name = g_utf8_strdown (invalid_names[i], -1);

    if (g_strcmp0 (lower_name, invalid_name) == 0)
      return FALSE;
  }

  /* Leading or Trailing whitespaces are invalid */
  if (g_str_has_prefix (name, " ") || g_str_has_suffix (name, " "))
    return FALSE;

  /* Must not start with ~$ */
  if (g_str_has_prefix (name, "~$"))
    return FALSE;

  /* _vti_ cannot be part of the name */
  if (strstr (name, "_vti_"))
    return FALSE;

  /* Not allowed chars: " * : < > ? / \ | */
  if (strchr (name, '"') ||
      strchr (name, '*') ||
      strchr (name, ':') ||
      strchr (name, '<') ||
      strchr (name, '>') ||
      strchr (name, '?') ||
      strchr (name, '/') ||
      strchr (name, '\\') ||
      strchr (name, '|'))
    return FALSE;

  /* "forms" on root level for a library */
  split = g_strsplit (name, "/", -1);
  if (g_strv_length (split) == 2 && g_strcmp0 (split[1], "forms") == 0)
    return FALSE;

  return TRUE;
}

static void
msg_drive_service_init (__attribute__ ((unused)) MsgDriveService *self)
{
}

static void
msg_drive_service_class_init (__attribute__ ((unused)) MsgDriveServiceClass *class)
{
}

/**
 * msg_drive_service_new:
 * @authorizer: A authorizer
 *
 * Creates a new `MsgDriveService` using #MsgAuthorizer.
 *
 * Returns: the newly created `MsgDriveService`
 */
MsgDriveService *
msg_drive_service_new (MsgAuthorizer *authorizer)
{
  return g_object_new (MSG_TYPE_DRIVE_SERVICE, "authorizer", authorizer, NULL);
}

static gboolean
drive_already_added (GList    *list,
                     MsgDrive *drive)
{
  const char *drive_id = msg_drive_get_id (drive);
  const char *drive_name = msg_drive_get_name (drive);
  gboolean drive_is_bundle = drive_name && g_str_has_prefix (drive_name, "Bundles_");

  for (GList *iter = list; iter && iter->data; iter = iter->next) {
    MsgDrive *list_drive = MSG_DRIVE (iter->data);

    if (g_strcmp0 (msg_drive_get_id (list_drive), drive_id) == 0 && drive_is_bundle)
      return TRUE;
  }

  return FALSE;
}

/**
 * msg_drive_service_get_drives:
 * @self: a #MsgDriveService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Queries the Microsoft Graph API for all the drives of the currently logged in user
 *
 * Returns: (element-type MsgDrive) (transfer full): all drives the user can access
 */
GList *
msg_drive_service_get_drives (MsgDriveService  *self,
                              GCancellable     *cancellable,
                              GError          **error)
{
  g_autofree char *url = NULL;
  JsonObject *root_object = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autolist (MsgDrive) list = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/drives", NULL);

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
      MsgDrive *drive = NULL;
      JsonObject *drive_object = NULL;
      g_autoptr (GError) local_error = NULL;

      drive_object = json_array_get_object_element (array, index);
      drive = msg_drive_new_from_json (drive_object, &local_error);
      if (drive) {

        if (!drive_already_added (list, drive)) {
          self->type = msg_drive_get_drive_type (drive);
          list = g_list_append (list, drive);
        }
      } else {
        g_warning ("Could not parse drive object: %s", local_error->message);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}

/**
 * msg_drive_service_get_root:
 * @self: a #MsgDriveService
 * @drive: a #MsgDrive
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get root item of selected drive
 *
 * Returns: (transfer full): root 'MsgDriveItem'
 */
MsgDriveItem *
msg_drive_service_get_root (MsgDriveService  *self,
                            MsgDrive         *drive,
                            GCancellable     *cancellable,
                            GError          **error)
{
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/drives/", msg_drive_get_id (drive), "/root?select=id,remoteItem,file,folder,parentReference,name,createdBy,lastModifiedBy,createdDateTime,lastModifiedDateTime,size", NULL);
  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_drive_item_new_from_json (root_object, error);
}

/**
 * msg_drive_service_download_item:
 * @self: a #MsgDriveService
 * @item: a #MsgDriveItem
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Download item
 *
 * Returns: (transfer full): input stream of drive item
 */
GInputStream *
msg_drive_service_download_item (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GCancellable     *cancellable,
                                 GError          **error)
{
  g_autofree char *url = NULL;
  const char *drive_id = NULL;
  const char *id = NULL;

  if (!MSG_IS_DRIVE_ITEM_FILE (item)) {
    g_warning ("Download only allowed for files");
    return NULL;
  }

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  if (!msg_drive_item_is_shared (item)) {
    drive_id = msg_drive_item_get_drive_id (item);
    id = msg_drive_item_get_id (item);
  } else {
    drive_id = msg_drive_item_get_remote_drive_id (item);
    id = msg_drive_item_get_remote_id (item);
  }

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     drive_id,
                     "/items/",
                     id,
                     "/content",
                     NULL);

  return msg_input_stream_new (MSG_SERVICE (self), url);
}

/**
 * msg_drive_service_list_children:
 * @self: a #MsgDriveService
 * @item: a #MsgDriveItem
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get a list of all files in folder item
 *
 * Returns: (element-type MsgDriveItem) (transfer full): all items in folder
 */
GList *
msg_drive_service_list_children (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GCancellable     *cancellable,
                                 GError          **error)
{
  g_autoptr (MsgDriveItem) child_item = NULL;
  g_autofree char *url = NULL;
  JsonObject *root_object = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  JsonObject *item_object = NULL;
  g_autolist (MsgDriveItem) children = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;
  gboolean add_prefer_header = self->type == MSG_DRIVE_TYPE_BUSINESS;
  const char *drive_id = NULL;
  const char *id = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  if (!msg_drive_item_is_shared (item)) {
    drive_id = msg_drive_item_get_drive_id (item);
    id = msg_drive_item_get_id (item);
  } else {
    drive_id = msg_drive_item_get_remote_drive_id (item);
    id = msg_drive_item_get_remote_id (item);
  }

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     drive_id,
                     "/items/",
                     id,
                     "/children",
                     "?$expand=thumbnails",
                     "&select=id,remoteItem,file,folder,parentReference,name,createdBy,lastModifiedBy,createdDateTime,lastModifiedDateTime,size",
                     NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL /*msg_drive_item_get_etag (item)*/, FALSE);
    if (add_prefer_header)
      soup_message_headers_append (soup_message_get_request_headers (message), "Prefer", "Include-Feature=AddToOneDrive");

    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      item_object = json_array_get_object_element (array, index);

      child_item = msg_drive_item_new_from_json (item_object, &local_error);
      if (local_error) {
        g_warning ("Could not read child item: %s", local_error->message);
        continue;
      }

      children = g_list_prepend (children, g_steal_pointer (&child_item));
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&children);
}

/**
 * msg_drive_service_rename:
 * @self: a #MsgDriveService
 * @item: a #MsgDriveItem
 * @new_name: new name of item
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Sets a new drive item name
 *
 * Returns: (transfer full): the renamed `MsgDriveItem`
 */
MsgDriveItem *
msg_drive_service_rename (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          const char       *new_name,
                          GCancellable     *cancellable,
                          GError          **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) rename_node = NULL;
  g_autofree char *url = NULL;
  g_autofree char *json = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!is_valid_name (new_name)) {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "Invalid characters in name");
    return NULL;
  }

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (item),
                     "/items/",
                     msg_drive_item_get_id (item),
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "PATCH", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "name");
  json_builder_add_string_value (builder, new_name);
  json_builder_end_object (builder);
  rename_node = json_builder_get_root (builder);
  json = json_to_string (rename_node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_drive_item_new_from_json (root_object, error);
}

/**
 * msg_drive_service_download_url:
 * @self: a #MsgDriveService
 * @url: url to download
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Download url
 *
 * Returns: (transfer full): a new `GInputStream` for url
 */
GInputStream *
msg_drive_service_download_url (MsgDriveService  *self,
                                const char       *url,
                                GCancellable     *cancellable,
                                GError          **error)
{
  g_autoptr (SoupMessage) message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);

  return msg_service_send (MSG_SERVICE (self), message, cancellable, error);
}

/**
 * msg_drive_service_create_folder:
 * @self: a drive service
 * @parent: parent drive item
 * @name: name of new folder
 * @cancellable: a cancellable
 * @error: a error
 *
 * Creates a new folder called name under parent.
 *
 * Returns: (transfer full): a newly created `MsgDriveItem`
 */
MsgDriveItem *
msg_drive_service_create_folder (MsgDriveService  *self,
                                 MsgDriveItem     *parent,
                                 const char       *name,
                                 GCancellable     *cancellable,
                                 GError          **error)
{
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *json = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) create_node = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  if (!is_valid_name (name)) {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "Invalid characters in name");
    return NULL;
  }

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (parent),
                     "/items/",
                     msg_drive_item_get_id (parent),
                     "/children",
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "POST", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "name");
  json_builder_add_string_value (builder, name);
  json_builder_set_member_name (builder, "@microsoft.graph.conflictBehavior");
  json_builder_add_string_value (builder, "rename");
  json_builder_set_member_name (builder, "folder");
  json_builder_begin_object (builder);
  json_builder_end_object (builder);
  json_builder_end_object (builder);
  create_node = json_builder_get_root (builder);
  json = json_to_string (create_node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_drive_item_new_from_json (root_object, error);
}

/**
 * msg_drive_service_delete:
 * @self: a drive service
 * @item: a #MsgDriveItem
 * @cancellable: a cancellable
 * @error: a error
 *
 * Deletes item.
 *
 * Returns: %TRUE when item has been deleted, otherwise %FALSE
 */
gboolean
msg_drive_service_delete (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          GCancellable     *cancellable,
                          GError          **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) bytes = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return FALSE;

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (item),
                     "/items/",
                     msg_drive_item_get_id (item),
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "DELETE", url, NULL, FALSE);

  bytes = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, error);
  return bytes != NULL;
}

/**
 * msg_drive_service_update:
 * @self: a drive service
 * @item: a drive item
 * @cancellable: a cancellable
 * @error: a error
 *
 * Creates an update stream for drive item in order to update it's content.
 *
 * Returns: (transfer full): an output stream
 */
GOutputStream *
msg_drive_service_update (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          GCancellable     *cancellable,
                          GError          **error)
{
  GOutputStream *stream = NULL;
  g_autofree char *url = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) create_node = NULL;
  g_autoptr (SoupMessage) message = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *json = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  const char *upload_url;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (item),
                     "/items/",
                     msg_drive_item_get_id (item),
                     "/createUploadSession",
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "POST", url, NULL, FALSE);
  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "@microsoft.graph.conflictBehavior");
  json_builder_add_string_value (builder, "rename");
  json_builder_end_object (builder);
  create_node = json_builder_get_root (builder);
  json = json_to_string (create_node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  if (!json_object_has_member (root_object, "uploadUrl")) {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "No uploadUrl found");
    return NULL;
  }

  upload_url = json_object_get_string_member (root_object, "uploadUrl");

  stream = g_memory_output_stream_new_resizable ();
  g_object_set_data_full (G_OBJECT (stream), "ms-graph-upload-url", g_strdup (upload_url), g_free);
  g_object_set_data (G_OBJECT (stream), "ms-graph-item", item);

  return stream;
}

/**
 * msg_drive_service_update_finish:
 * @self: a #MsgDriveService
 * @item: a #MsgDriveItem
 * @stream: stream where data is store and needs to be transfered
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Finish a update session of given #item.
 *
 * Returns: (transfer full): a new #MsgDriveItem or %NULL on error.
 */
MsgDriveItem *
msg_drive_service_update_finish (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GOutputStream    *stream,
                                 GCancellable     *cancellable,
                                 GError          **error)
{
  g_autoptr (GBytes) bytes = NULL;
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (item),
                     "/items/",
                     msg_drive_item_get_id (item),
                     "/content",
                     NULL);

  g_output_stream_close (stream, NULL, NULL);
  bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (stream));

  message = msg_service_build_message (MSG_SERVICE (self), "PUT", url, NULL, FALSE);
  soup_message_set_request_body_from_bytes (message, "application/octet-stream", bytes);
  g_clear_object (&stream);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_drive_item_new_from_json (root_object, error);
}

/**
 * msg_drive_service_add_item_to_folder:
 * @self: a msg drive service
 * @parent: parent drive item
 * @item: drive item to add
 * @cancellable: a cancellable
 * @error: a error
 *
 * Adds item to parent folder
 *
 * Returns: (transfer full): a new drive item
 */
MsgDriveItem *
msg_drive_service_add_item_to_folder (MsgDriveService  *self,
                                      MsgDriveItem     *parent,
                                      MsgDriveItem     *item,
                                      GCancellable     *cancellable,
                                      GError          **error)
{
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *escaped_name = NULL;
  JsonObject *root_object = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!is_valid_name (msg_drive_item_get_name (item))) {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "Invalid characters in name");
    return NULL;
  }

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return FALSE;

  escaped_name = g_uri_escape_string (msg_drive_item_get_name (item), NULL, TRUE);
  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (parent),
                     "/items/",
                     msg_drive_item_get_id (parent),
                     ":/",
                     escaped_name,
                     ":/content",
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "PUT", url, NULL, FALSE);
  body = g_bytes_new ("", 0);
  soup_message_set_request_body_from_bytes (message, "text/plain", body);

  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return FALSE;

  return msg_drive_item_new_from_json (root_object, error);
}

/**
 * msg_drive_service_get_shared_with_me:
 * @self: a #MsgDriveService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Requests all shared with me items
 *
 * Returns: (element-type MsgDriveItem) (transfer full): shared with me list
 */
GList *
msg_drive_service_get_shared_with_me (MsgDriveService  *self,
                                      GCancellable     *cancellable,
                                      GError          **error)
{
  g_autoptr (MsgDriveItem) child_item = NULL;
  g_autofree char *url = NULL;
  JsonObject *root_object = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  JsonObject *item_object = NULL;
  g_autolist (MsgDriveItem) children = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/me/drive/sharedWithMe",
                     "?select=id,remoteItem,file,folder,parentReference,name,createdBy,lastModifiedBy,createdDateTime,lastModifiedDateTime,size",
                     NULL);
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
      item_object = json_array_get_object_element (array, index);

      child_item = msg_drive_item_new_from_json (item_object, &local_error);
      if (local_error) {
        g_warning ("Could not load shared with me item: %s", local_error->message);
        continue;
      }

      children = g_list_prepend (children, g_steal_pointer (&child_item));
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&children);
}

/**
 * msg_drive_service_copy_file:
 * @self: #MsgDriveService
 * @file: source #MsgDriveItem
 * @destination: destination directory #MsgDriveItem
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Copy a file async on remote server to a new directory.
 *
 * Returns: %TRUE if accepted, %FALSE on error
 */
gboolean
msg_drive_service_copy_file (MsgDriveService  *self,
                             MsgDriveItem     *file,
                             MsgDriveItem     *destination,
                             GCancellable     *cancellable,
                             GError          **error)
{
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *json = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) create_node = NULL;
  GError *local_error = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, &local_error)) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return FALSE;
  }

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (file),
                     "/items/",
                     msg_drive_item_get_id (file),
                     "/copy",
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "POST", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "parentReference");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "driveId");
  json_builder_add_string_value (builder, msg_drive_item_get_drive_id (destination));
  json_builder_set_member_name (builder, "id");
  json_builder_add_string_value (builder, msg_drive_item_get_id (destination));
  json_builder_end_object (builder);

  json_builder_end_object (builder);
  create_node = json_builder_get_root (builder);
  json = json_to_string (create_node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return FALSE;
  }

  return TRUE;
}

/**
 * msg_drive_service_move_file:
 * @self: #MsgDriveService
 * @file: source #MsgDriveItem
 * @destination: destination directory #MsgDriveItem
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Move a file async on remote server to a new directory.
 *
 * Returns: (transfer full): moved #MsgDriveItem
 */
MsgDriveItem *
msg_drive_service_move_file (MsgDriveService  *self,
                             MsgDriveItem     *file,
                             MsgDriveItem     *destination,
                             GCancellable     *cancellable,
                             GError          **error)
{
  g_autofree char *url = NULL;
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *json = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) create_node = NULL;
  GError *local_error = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (GBytes) body = NULL;
  g_autoptr (JsonParser) parser = NULL;
  MsgDriveItem *item = NULL;
  JsonObject *root_object = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, &local_error)) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  url = g_strconcat (MSG_API_ENDPOINT,
                     "/drives/",
                     msg_drive_item_get_drive_id (file),
                     "/items/",
                     msg_drive_item_get_id (file),
                     NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "PATCH", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "parentReference");
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "id");
  json_builder_add_string_value (builder, msg_drive_item_get_id (destination));
  json_builder_end_object (builder);

  json_builder_end_object (builder);
  create_node = json_builder_get_root (builder);
  json = json_to_string (create_node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  parser = msg_service_parse_response (response, &root_object, &local_error);
  if (local_error != NULL) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  item = msg_drive_item_new_from_json (root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return FALSE;
  }

  return item;
}
