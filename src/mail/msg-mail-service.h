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

#include <glib-object.h>

#include <msg-authorizer.h>
#include <mail/msg-mail-folder.h>
#include <mail/msg-mail-message.h>
#include <msg-service.h>

#define MSG_TYPE_MAIL_SERVICE (msg_mail_service_get_type ())

G_DECLARE_FINAL_TYPE (MsgMailService, msg_mail_service, MSG, MAIL_SERVICE, MsgService);

MsgMailService *msg_mail_service_new (MsgAuthorizer *authorizer);

GList *
msg_mail_service_get_messages (MsgMailService  *self,
                               MsgMailFolder   *folder,
                               const char      *next_link,
                               char           **out_next_link,
                               const char      *delta_link,
                               char           **out_delta_link,
                               int              max_page_size,
                               GCancellable    *cancellable,
                               GError         **error);

GList *
msg_mail_service_get_mail_folders (MsgMailService  *self,
                                   char            *delta_url,
                                   char           **delta_url_out,
                                   GCancellable    *cancellable,
                                   GError         **error);

MsgMailFolder *
msg_mail_service_get_mail_folder (MsgMailService     *self,
                                  MsgMailFolderType   type,
                                  GCancellable       *cancellable,
                                  GError            **error);

MsgMailMessage *
msg_mail_service_create_draft_message (MsgMailService  *self,
                                       MsgMailMessage  *mail,
                                       GCancellable    *cancellable,
                                       GError         **error);

gboolean
msg_mail_service_delete_message (MsgMailService  *self,
                                 MsgMailMessage  *mail,
                                 GCancellable    *cancellable,
                                 GError         **error);

GBytes *
msg_mail_service_get_mime_message (MsgMailService  *self,
                                   MsgMailMessage  *mail,
                                   GCancellable    *cancellable,
                                   GError         **error);

char *
msg_mail_service_get_folder_id (MsgMailService     *self,
                                MsgMailFolderType   type,
                                GCancellable       *cancellable,
                                GError            **error);

