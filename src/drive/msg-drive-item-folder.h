/* Copyright 2022-2023 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#pragma once

#if !defined(_MSG_INSIDE) && !defined(MSG_COMPILATION)
#error "Only <msg.h> can be included directly."
#endif

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include <drive/msg-drive-item.h>

G_BEGIN_DECLS

#define MSG_TYPE_DRIVE_ITEM_FOLDER (msg_drive_item_folder_get_type ())

G_DECLARE_FINAL_TYPE (MsgDriveItemFolder, msg_drive_item_folder, MSG, DRIVE_ITEM_FOLDER, MsgDriveItem);

struct _MsgDriveItemFolderClass {
  MsgDriveItemClass parent_class;

  gpointer padding[4];
};

MsgDriveItemFolder *
msg_drive_item_folder_new (void);

MsgDriveItemFolder *
msg_drive_item_folder_new_from_json (JsonObject  *object,
                                     GError     **error);

G_END_DECLS

