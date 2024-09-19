/* Copyright 2022-2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#include "drive/msg-drive-item-file.h"
#include "msg-error.h"
#include "msg-json-utils.h"

struct _MsgDriveItemFile {
  MsgDriveItem parent_instance;
  char *mime_type;
  char *thumbnail_uri;
};

G_DEFINE_TYPE (MsgDriveItemFile, msg_drive_item_file, MSG_TYPE_DRIVE_ITEM);

static void
msg_drive_item_file_finalize (GObject *_self)
{
  MsgDriveItemFile *self = MSG_DRIVE_ITEM_FILE (_self);

  g_clear_pointer (&self->mime_type, g_free);
  g_clear_pointer (&self->thumbnail_uri, g_free);

  G_OBJECT_CLASS (msg_drive_item_file_parent_class)->finalize (_self);
}

static void
msg_drive_item_file_init (__attribute__ ((unused)) MsgDriveItemFile *self)
{
}

static void
msg_drive_item_file_class_init (MsgDriveItemFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = msg_drive_item_file_finalize;
}

/**
 * msg_drive_item_file_new:
 *
 * Creates a new `MsgDriveItemFile`.
 *
 * Returns: the newly created `MsgDriveItemFile`
 */
MsgDriveItemFile *
msg_drive_item_file_new (void)
{
  return g_object_new (MSG_TYPE_DRIVE_ITEM_FILE, NULL);
}

/**
 * msg_drive_item_file_new_from_json:
 * @object: The json object to parse
 *
 * Creates a new `MsgDriveItemFile` from json response object.
 *
 * Returns: the newly created `MsgDriveItemFile`
 */
MsgDriveItemFile *
msg_drive_item_file_new_from_json (JsonObject *object)
{
  MsgDriveItemFile *self = msg_drive_item_file_new ();
  JsonObject *file = NULL;

  if (json_object_has_member (object, "file"))
    file = json_object_get_object_member (object, "file");
  else {
    JsonObject *remote_item = json_object_get_object_member (object, "remoteItem");
    file = json_object_get_object_member (remote_item, "file");
  }

  self->mime_type = g_strdup (msg_json_object_get_string (file, "mimeType"));

  if (json_object_has_member (object, "thumbnails")) {
    JsonArray *array;
    guint array_length;
    guint index;

    array = json_object_get_array_member (object, "thumbnails");
    array_length = json_array_get_length (array);

    for (index = 0; index < array_length; index++) {
      JsonObject *item_object;
      JsonObject *small;

      /* TODO: Extract best thumbnail */
      item_object = json_array_get_object_element (array, index);
      small = json_object_get_object_member (item_object, "small");

      self->thumbnail_uri = g_strdup (msg_json_object_get_string (small, "url"));
    }
  }

  return self;
}

/**
 * msg_drive_item_file_get_mime_type:
 * @self: a drive item file
 *
 * Gets mime type of drive item file.
 *
 * Returns: (transfer none): mime type of drive item file
 */
const char *
msg_drive_item_file_get_mime_type (MsgDriveItemFile *self)
{
  return self->mime_type;
}

/**
 * msg_drive_item_file_get_thumbnail_uri:
 * @self: a drive item file
 *
 * Gets thumbnail uri of drive item file.
 *
 * Returns: (transfer none): thumbnail uri of drive item file
 */
const char *
msg_drive_item_file_get_thumbnail_uri (MsgDriveItemFile *self)
{
  return self->thumbnail_uri;
}
