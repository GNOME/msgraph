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

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "msg-drive-item.h"
#include "msg-drive-item-file.h"
#include "msg-drive-item-folder.h"
#include "msg-error.h"
#include "msg-json-utils.h"

typedef struct {
  char *id;
  char *parent_id;
  char *drive_id;
  char *remote_id;
  char *remote_parent_id;
  char *remote_drive_id;
  char *name;
  char *user;
  char *etag;

  GDateTime *created;
  GDateTime *modified;

  guint64 size;

  gboolean is_shared;
} MsgDriveItemPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MsgDriveItem, msg_drive_item, G_TYPE_OBJECT);

static void
msg_drive_item_finalize (GObject *object)
{
  MsgDriveItem *self = MSG_DRIVE_ITEM (object);
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->parent_id, g_free);
  g_clear_pointer (&priv->drive_id, g_free);
  g_clear_pointer (&priv->remote_id, g_free);
  g_clear_pointer (&priv->remote_parent_id, g_free);
  g_clear_pointer (&priv->remote_drive_id, g_free);
  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->user, g_free);
  g_clear_pointer (&priv->etag, g_free);

  g_clear_pointer (&priv->created, g_date_time_unref);
  g_clear_pointer (&priv->modified, g_date_time_unref);

  G_OBJECT_CLASS (msg_drive_item_parent_class)->finalize (object);
}

static void
msg_drive_item_init (__attribute__ ((unused)) MsgDriveItem *item)
{
}

static void
msg_drive_item_class_init (MsgDriveItemClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_drive_item_finalize;
}

/**
 * msg_drive_item_new_from_json:
 * @object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgDriveItem` from json response object.
 *
 * Returns: the newly created `MsgDriveItem`
 */
MsgDriveItem *
msg_drive_item_new_from_json (JsonObject  *object,
                              GError     **error)
{
  g_autoptr (MsgDriveItem) self = NULL;
  g_autoptr (GError) _error = NULL;
  MsgDriveItemPrivate *priv;
  gboolean remote = FALSE;
  gboolean is_file = FALSE;
  gboolean is_folder = FALSE;

  if (json_object_has_member (object, "file"))
    is_file = TRUE;
  else if (json_object_has_member (object, "folder"))
    is_folder = TRUE;

  if (!is_file && !is_folder && json_object_has_member (object, "remoteItem")) {
    JsonObject *remote_item = json_object_get_object_member (object, "remoteItem");

    if (json_object_has_member (remote_item, "file"))
      is_file = TRUE;
    else if (json_object_has_member (remote_item, "folder"))
      is_folder = TRUE;
  }

  if (is_file) {
    self = MSG_DRIVE_ITEM (msg_drive_item_file_new_from_json (object));
  } else if (is_folder) {
    self = MSG_DRIVE_ITEM (msg_drive_item_folder_new_from_json (object, &_error));
  } else {
    g_warning ("Unknown item type\n");
    g_set_error (error,
                 MSG_ERROR,
                 MSG_ERROR_FAILED,
                 "Unknown item type\n");
    return NULL;
  }

  if (!self)
    return NULL;

  priv = msg_drive_item_get_instance_private (self);

  if (json_object_has_member (object, "remoteItem")) {
    JsonObject *remote_item = json_object_get_object_member (object, "remoteItem");
    JsonObject *parent_reference = json_object_get_object_member (remote_item, "parentReference");

    priv->remote_id = g_strdup (msg_json_object_get_string (remote_item, "id"));
    priv->remote_drive_id = g_strdup (msg_json_object_get_string (parent_reference, "driveId"));
    priv->remote_parent_id = g_strdup (msg_json_object_get_string (parent_reference, "id"));
    remote = TRUE;
  }

  priv->is_shared = remote;
  priv->id = g_strdup (msg_json_object_get_string (object, "id"));

  if (json_object_has_member (object, "parentReference")) {
    JsonObject *parent_reference = json_object_get_object_member (object, "parentReference");

    priv->drive_id = g_strdup (msg_json_object_get_string (parent_reference, "driveId"));
  }

  priv->name = g_strdup (msg_json_object_get_string (object, "name"));

  if (json_object_has_member (object, "size"))
    priv->size = json_object_get_int_member (object, "size");
  else
    priv->size = 0;

  priv->etag = g_strdup (msg_json_object_get_string (object, "eTag"));

  if (json_object_has_member (object, "createdBy")) {
    JsonObject *created_by;

    created_by = json_object_get_object_member (object, "createdBy");
    if (created_by && json_object_has_member (created_by, "user")) {
      JsonObject *user = json_object_get_object_member (created_by, "user");
      priv->user = g_strdup (msg_json_object_get_string (user, "displayName"));
    }
  } else if (json_object_has_member (object, "lastModifiedBy")) {
    JsonObject *created_by;

    created_by = json_object_get_object_member (object, "lastModifiedBy");
    if (created_by && json_object_has_member (created_by, "user")) {
      JsonObject *user = json_object_get_object_member (created_by, "user");
      priv->user = g_strdup (msg_json_object_get_string (user, "displayName"));
    }
  }

  if (json_object_has_member (object, "createdDateTime")) {
    const char *time = json_object_get_string_member (object, "createdDateTime");
    priv->created = g_date_time_new_from_iso8601 (time, NULL);
  }

  if (json_object_has_member (object, "lastModifiedDateTime")) {
    const char *time = json_object_get_string_member (object, "lastModifiedDateTime");
    priv->modified = g_date_time_new_from_iso8601 (time, NULL);
  }

  return g_steal_pointer (&self);
}

/**
 * msg_drive_item_get_name:
 * @self: a drive item
 *
 * Gets name of drive item.
 *
 * Returns: (transfer none): name of drive item
 */
const char *
msg_drive_item_get_name (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->name;
}

/**
 * msg_drive_item_set_name:
 * @self: a drive item
 * @name: new name of drive item
 *
 * Sets name of drive item.
 */
void
msg_drive_item_set_name (MsgDriveItem *self,
                         const char   *name)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  g_clear_pointer (&priv->name, g_free);
  priv->name = g_strdup (name);
}

/**
 * msg_drive_item_get_id:
 * @self: a drive item
 *
 * Get id of drive item.
 *
 * Returns: (transfer none): id of drive item
 */
const char *
msg_drive_item_get_id (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->id;
}

/**
 * msg_drive_item_set_id:
 * @self: a drive item
 * @id: new id of drive item
 *
 * Sets id of drive item.
 */
void
msg_drive_item_set_id (MsgDriveItem *self,
                       const char   *id)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  g_clear_pointer (&priv->id, g_free);
  priv->id = g_strdup (id);
}

/**
 * msg_drive_item_get_created:
 * @self: a drive item
 *
 * Get created time of drive item.
 *
 * Returns: created date time of drive item
 */
gint64
msg_drive_item_get_created (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->created ? g_date_time_to_unix (priv->created) : 0;
}

/**
 * msg_drive_item_get_modified:
 * @self: a drive item
 *
 * Gets modified time of drive item.
 *
 * Returns: modified date time of drive item
 */
gint64
msg_drive_item_get_modified (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->modified ? g_date_time_to_unix (priv->modified) : 0;
}

/**
 * msg_drive_item_get_etag:
 * @self: a drive item
 *
 * Gets etag of drive item.
 *
 * Returns: (transfer none): etag of drive item
 */
const char *
msg_drive_item_get_etag (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->etag;
}

/**
 * msg_drive_item_get_user:
 * @self: a drive item
 *
 * Gets user of drive item.
 *
 * Returns: (transfer none): user of drive item
 */
const char *
msg_drive_item_get_user (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->user;
}

/**
 * msg_drive_item_get_size
 * @self: a drive item
 *
 * Gets size of drive item.
 *
 * Returns: size of drive item
 */
guint64
msg_drive_item_get_size (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->size;
}

/**
 * msg_drive_item_get_drive_id:
 * @self: a drive item
 *
 * Gets drive id of drive item.
 *
 * Returns: (transfer none): drive id of drive item
 */
const char *
msg_drive_item_get_drive_id (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->drive_id;
}

/**
 * msg_drive_item_get_parent_id:
 * @self: a drive item
 *
 * Gets parent id of drive item.
 *
 * Returns: (transfer none): parent id of drive item
 */
const char *
msg_drive_item_get_parent_id (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->parent_id;
}

/**
 * msg_drive_item_is_shared:
 * @self: a drive item
 *
 * Gets whether item is shared.
 *
 * Returns: %TRUE if item is shared, otherwise %FALSE
 */
gboolean
msg_drive_item_is_shared (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->is_shared;
}

/**
 * msg_drive_item_set_parent_id:
 * @self: a drive item
 * @parent_id: parent id
 *
 * Set parent id of drive item.
 */
void
msg_drive_item_set_parent_id (MsgDriveItem *self,
                              const char   *parent_id)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  g_clear_pointer (&priv->parent_id, g_free);
  priv->parent_id = g_strdup (parent_id);
}

/**
 * msg_drive_item_get_remote_drive_id:
 * @self: a drive item
 *
 * Get remote parent drive id of drive item.
 */
const char *
msg_drive_item_get_remote_drive_id (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->remote_drive_id;
}

/**
 * msg_drive_item_get_remote_id:
 * @self: a drive item
 *
 * Get remote id of drive item.
 */
const char *
msg_drive_item_get_remote_id (MsgDriveItem *self)
{
  MsgDriveItemPrivate *priv = msg_drive_item_get_instance_private (self);

  return priv->remote_id;
}
