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
#include <message/msg-mail-folder.h>
#include <message/msg-message.h>
#include <msg-service.h>

#define MSG_TYPE_MESSAGE_SERVICE (msg_message_service_get_type ())

G_DECLARE_FINAL_TYPE (MsgMessageService, msg_message_service, MSG, MESSAGE_SERVICE, MsgService);

MsgMessageService *msg_message_service_new (MsgAuthorizer *authorizer);


typedef enum {
  MSG_MESSAGE_MAIL_FOLDER_TYPE_INBOX,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_DRAFTS,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_SENT_ITEMS,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_JUNK_EMAIL,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_DELETED_ITEMS,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_OUTBOX,
  MSG_MESSAGE_MAIL_FOLDER_TYPE_ARCHIVE,
} MsgMessageMailFolderType;

GList *
msg_message_service_get_messages (MsgMessageService  *self,
                                  MsgMailFolder      *folder,
                                  const char         *delta_link,
                                  int                 max_page_size,
                                  char              **out_delta_link,
                                  GCancellable       *cancellable,
                                  GError            **error);

GList *
msg_message_service_get_mail_folders (MsgMessageService  *self,
                                      GCancellable       *cancellable,
                                      GError            **error);

MsgMailFolder *
msg_message_service_get_mail_folder (MsgMessageService         *self,
                                     MsgMessageMailFolderType   type,
                                     GCancellable              *cancellable,
                                     GError                   **error);

MsgMessage *
msg_message_service_create_draft (MsgMessageService  *self,
                                  MsgMessage         *message,
                                  GCancellable       *cancellable,
                                  GError            **error);

gboolean
msg_message_service_delete (MsgMessageService  *self,
                            MsgMessage         *message,
                            GCancellable       *cancellable,
                            GError            **error);

GBytes *
msg_message_service_get_mime_message (MsgMessageService  *self,
                                      MsgMessage         *message,
                                      GCancellable       *cancellable,
                                      GError            **error);
