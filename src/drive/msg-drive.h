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
#include <gio/gio.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * MsgDriveType:
 * @MSG_DRIVE_TYPE_PERSONAL: Personal OneDrive drive
 * @MSG_DRIVE_TYPE_BUSINESS: OneDrive for Business drive
 * @MSG_DRIVE_TYPE_DOCUMENT_LIBRARY: SharePoint document library
 *
 * The type of Drive
 * <ulink url="https://docs.microsoft.com/en-us/graph/api/resources/drive?view=graph-rest-1.0">
 * objects</ulink>.
 */
typedef enum
{
  MSG_DRIVE_TYPE_PERSONAL,
  MSG_DRIVE_TYPE_BUSINESS,
  MSG_DRIVE_TYPE_DOCUMENT_LIBRARY,
} MsgDriveType;

#define MSG_TYPE_DRIVE (msg_drive_get_type ())

G_DECLARE_FINAL_TYPE (MsgDrive, msg_drive, MSG, DRIVE, GObject);

MsgDrive *
msg_drive_new (void);

MsgDrive *
msg_drive_new_from_json (JsonObject  *object,
                         GError     **error);

const char *
msg_drive_get_id (MsgDrive *self);

MsgDriveType
msg_drive_get_drive_type (MsgDrive *self);

const char *
msg_drive_get_name (MsgDrive *self);

gulong
msg_drive_get_total (MsgDrive *self);

gulong
msg_drive_get_used (MsgDrive *self);

gulong
msg_drive_get_remaining (MsgDrive *self);

const GDateTime *
msg_drive_get_created (MsgDrive *self);

const GDateTime *
msg_drive_get_modified (MsgDrive *self);

G_END_DECLS
