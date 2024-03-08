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

#include "drive/msg-drive.h"
#include "msg-error.h"
#include "msg-json-utils.h"

/**
 * MsgDrive:
 *
 * #MsgDriveService is a subclass of #MsgService for communicating with the MS Graph API.
 *
 * Details: https://learn.microsoft.com/en-us/graph/api/resources/drive?view=graph-rest-1.0
 */

struct _MsgDrive {
  GObject parent_instance;

  MsgDriveType drive_type;
  char *id;
  char *name;
  gulong remaining;
  gulong total;
  gulong used;
  GDateTime *created;
  GDateTime *modified;
};

G_DEFINE_TYPE (MsgDrive, msg_drive, G_TYPE_OBJECT);

static void
msg_drive_finalize (GObject *object)
{
  MsgDrive *self = MSG_DRIVE (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->created, g_date_time_unref);
  g_clear_pointer (&self->modified, g_date_time_unref);

  G_OBJECT_CLASS (msg_drive_parent_class)->finalize (object);
}

static void
msg_drive_init (__attribute__ ((unused)) MsgDrive *self)
{
}

static void
msg_drive_class_init (MsgDriveClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_drive_finalize;
}

/**
 * msg_drive_new:
 *
 * Creates a new `MsgDrive`.
 *
 * Returns: the newly created `MsgDrive`
 */
MsgDrive *
msg_drive_new (void)
{
  return g_object_new (MSG_TYPE_DRIVE, NULL);
}

/**
 * msg_drive_new_from_json:
 * @object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgDrive` from json response object.
 *
 * Returns: the newly created `MsgDrive`
 */
MsgDrive *
msg_drive_new_from_json (JsonObject  *object,
                         GError     **error)
{
  MsgDrive *self;
  const char *drive_type = NULL;

  self = msg_drive_new ();
  g_assert (self != NULL);

  drive_type = json_object_get_string_member (object, "driveType");
  if (g_strcmp0 (drive_type, "personal") == 0) {
    self->drive_type = MSG_DRIVE_TYPE_PERSONAL;
  } else if (g_strcmp0 (drive_type, "business") == 0) {
    self->drive_type = MSG_DRIVE_TYPE_BUSINESS;
  } else if (g_strcmp0 (drive_type, "documentLibrary") == 0) {
    self->drive_type = MSG_DRIVE_TYPE_DOCUMENT_LIBRARY;
  } else {
    g_set_error (error,
                 msg_error_quark (),
                 MSG_ERROR_FAILED,
                 "Unknown drive type: %s", drive_type);

    return NULL;
  }

  self->id = g_strdup (msg_json_object_get_string (object, "id"));
  self->name = g_strdup (msg_json_object_get_string (object, "name"));

  if (json_object_has_member (object, "createdDateTime"))
    self->created = g_date_time_new_from_iso8601 (json_object_get_string_member (object, "createdDateTime"), NULL);
  if (json_object_has_member (object, "lastModifiedDateTime"))
    self->modified = g_date_time_new_from_iso8601 (json_object_get_string_member (object, "lastModifiedDateTime"), NULL);

  if (json_object_has_member (object, "quota")) {
    JsonObject *quota = json_object_get_object_member (object, "quota");

    self->total = json_object_get_int_member (quota, "total");
    self->used = json_object_get_int_member (quota, "used");
    self->remaining = json_object_get_int_member (quota, "remaining");
  }

  return self;
}

/**
 * msg_drive_get_id:
 * @self: a drive
 *
 * Gets the ID of the drive.
 *
 * Returns: (transfer none): the id of the drive
 */
const char *
msg_drive_get_id (MsgDrive *self)
{
  return self->id;
}

/**
 * msg_drive_get_drive_type:
 * @self: a drive
 *
 * Gets the drive type of the drive.
 *
 * Returns: the drive type of drive
 */
MsgDriveType
msg_drive_get_drive_type (MsgDrive *self)
{
  return self->drive_type;
}

/**
 * msg_drive_get_name:
 * @self: a drive
 *
 * Gets tthe name of the drive.
 *
 * Returns: (transfer none): name of drive
 */
const char *
msg_drive_get_name (MsgDrive *self)
{
  return self->name;
}

/**
 * msg_drive_get_total:
 * @self: a drive
 *
 * Gets the total size of the drive.
 *
 * Returns: total size of drive
 */
gulong
msg_drive_get_total (MsgDrive *self)
{
  return self->total;
}

/**
 * msg_drive_get_used:
 * @self: a drive
 *
 * Gets the used size of the drive.
 *
 * Returns: used size of drive
 */
gulong
msg_drive_get_used (MsgDrive *self)
{
  return self->used;
}

/**
 * msg_drive_get_remaining:
 * @self: a drive
 *
 * Gets the remaining size of the drive.
 *
 * Returns: remaining size of drive
 */
gulong
msg_drive_get_remaining (MsgDrive *self)
{
  return self->remaining;
}

/**
 * msg_drive_get_created:
 * @self: a drive
 *
 * Get created time of drive.
 *
 * Returns: (transfer none): created time of drive
 */
const GDateTime *
msg_drive_get_created (MsgDrive *self)
{
  return self->created;
}

/**
 * msg_drive_get_modified:
 * @self: a drive
 *
 * Gets the modified time of the drive.
 *
 * Returns: (transfer none): modified time of drive
 */
const GDateTime *
msg_drive_get_modified (MsgDrive *self)
{
  return self->modified;
}
