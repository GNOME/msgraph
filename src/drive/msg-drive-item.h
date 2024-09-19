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
#include <libsoup/soup.h>

G_BEGIN_DECLS

#define MSG_TYPE_DRIVE_ITEM (msg_drive_item_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsgDriveItem, msg_drive_item, MSG, DRIVE_ITEM, GObject);

struct _MsgDriveItemClass {
  GObjectClass parent_class;

  gpointer padding[4];
};

MsgDriveItem *
msg_drive_item_new_from_json (JsonObject  *object,
                              GError     **error);

const char *
msg_drive_item_get_name (MsgDriveItem *self);

void
msg_drive_item_set_name (MsgDriveItem *self,
                         const char   *name);

const char *
msg_drive_item_get_id (MsgDriveItem *self);

void
msg_drive_item_set_id (MsgDriveItem *self,
                       const char   *id);

gint64
msg_drive_item_get_created (MsgDriveItem *self);

gint64
msg_drive_item_get_modified (MsgDriveItem *self);

const char *
msg_drive_item_get_etag (MsgDriveItem *self);

const char *
msg_drive_item_get_user (MsgDriveItem *self);

guint64
msg_drive_item_get_size (MsgDriveItem *self);

const char *
msg_drive_item_get_drive_id (MsgDriveItem *self);

const char *
msg_drive_item_get_parent_id (MsgDriveItem *self);

gboolean
msg_drive_item_is_shared (MsgDriveItem *self);

void
msg_drive_item_set_parent_id (MsgDriveItem *self,
                              const char   *parent_id);

const char *
msg_drive_item_get_remote_drive_id (MsgDriveItem *self);

const char *
msg_drive_item_get_remote_id (MsgDriveItem *self);

G_END_DECLS
