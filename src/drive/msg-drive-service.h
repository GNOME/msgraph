/* Copyright 2023 Jan-Michael Brummer
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

#include <drive/msg-drive.h>
#include <drive/msg-drive-item.h>
#include <msg-service.h>

G_BEGIN_DECLS

#define MSG_TYPE_DRIVE_SERVICE (msg_drive_service_get_type ())

G_DECLARE_FINAL_TYPE (MsgDriveService, msg_drive_service, MSG, DRIVE_SERVICE, MsgService);

MsgDriveService *
msg_drive_service_new (MsgAuthorizer *authorizer);

GList *
msg_drive_service_get_drives (MsgDriveService  *self,
                              GCancellable     *cancellable,
                              GError          **error);

MsgDriveItem *
msg_drive_service_get_root (MsgDriveService  *self,
                            MsgDrive         *drive,
                            GCancellable     *cancellable,
                            GError          **error);

GList *
msg_drive_service_list_children (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GCancellable     *cancellable,
                                 GError          **error);

GInputStream *
msg_drive_service_download_url (MsgDriveService  *self,
                                const char       *url,
                                GCancellable     *cancellable,
                                GError          **error);

GInputStream *
msg_drive_service_download_item (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GCancellable     *cancellable,
                                 GError          **error);

/* Write support */

MsgDriveItem *
msg_drive_service_rename (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          const char       *new_name,
                          GCancellable     *cancellable,
                          GError          **error);


MsgDriveItem *
msg_drive_service_create_folder (MsgDriveService  *self,
                                 MsgDriveItem     *parent,
                                 const char       *name,
                                 GCancellable     *cancellable,
                                 GError          **error);

gboolean
msg_drive_service_delete (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          GCancellable     *cancellable,
                          GError          **error);

GOutputStream *
msg_drive_service_update (MsgDriveService  *self,
                          MsgDriveItem     *item,
                          GCancellable     *cancellable,
                          GError          **error);

MsgDriveItem *
msg_drive_service_update_finish (MsgDriveService  *self,
                                 MsgDriveItem     *item,
                                 GOutputStream    *stream,
                                 GCancellable     *cancellable,
                                 GError          **error);

MsgDriveItem *
msg_drive_service_add_item_to_folder (MsgDriveService  *self,
                                      MsgDriveItem     *parent,
                                      MsgDriveItem     *item,
                                      GCancellable     *cancellable,
                                      GError          **error);

GList *
msg_drive_service_get_shared_with_me (MsgDriveService  *self,
                                      GCancellable     *cancellable,
                                      GError          **error);

gboolean
msg_drive_service_copy_file (MsgDriveService  *self,
                             MsgDriveItem     *file,
                             MsgDriveItem     *destination,
                             GCancellable     *cancellable,
                             GError          **error);

MsgDriveItem *
msg_drive_service_move_file (MsgDriveService  *self,
                             MsgDriveItem     *file,
                             MsgDriveItem     *destination,
                             GCancellable     *cancellable,
                             GError          **error);

G_END_DECLS
