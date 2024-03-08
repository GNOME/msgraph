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

#include "msg-drive-item.h"
#include "msg-drive-item-folder.h"

#include "msg-error.h"

struct _MsgDriveItemFolder {
  MsgDriveItem parent_instance;
};

G_DEFINE_TYPE (MsgDriveItemFolder, msg_drive_item_folder, MSG_TYPE_DRIVE_ITEM);

static void
msg_drive_item_folder_init (__attribute__ ((unused)) MsgDriveItemFolder *self)
{
}

static void
msg_drive_item_folder_class_init (__attribute__ ((unused)) MsgDriveItemFolderClass *klass)
{
}

/**
 * msg_drive_item_folder_new:
 *
 * Creates a new `MsgDriveItemFolder`.
 *
 * Returns: the newly created `MsgDriveItemFolder`
 */
MsgDriveItemFolder *
msg_drive_item_folder_new (void)
{
  return g_object_new (MSG_TYPE_DRIVE_ITEM_FOLDER, NULL);
}

/**
 * msg_drive_item_folder_new_from_json:
 * @object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgDriveItemFolder` from json response object.
 *
 * Returns: the newly created `MsgDriveItemFolder`
 */
MsgDriveItemFolder *
msg_drive_item_folder_new_from_json (__attribute__ ((unused)) JsonObject *object,
                                     __attribute__ ((unused)) GError    **error)
{
  MsgDriveItemFolder *self = msg_drive_item_folder_new ();

  return self;
}
