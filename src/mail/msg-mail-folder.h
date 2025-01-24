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

#define MSG_TYPE_MAIL_FOLDER (msg_mail_folder_get_type ())

G_DECLARE_FINAL_TYPE (MsgMailFolder, msg_mail_folder, MSG, MAIL_FOLDER, GObject);

typedef enum {
  MSG_MAIL_FOLDER_TYPE_0,
  MSG_MAIL_FOLDER_TYPE_INBOX,
  MSG_MAIL_FOLDER_TYPE_DRAFTS,
  MSG_MAIL_FOLDER_TYPE_SENT_ITEMS,
  MSG_MAIL_FOLDER_TYPE_JUNK_EMAIL,
  MSG_MAIL_FOLDER_TYPE_DELETED_ITEMS,
  MSG_MAIL_FOLDER_TYPE_OUTBOX,
  MSG_MAIL_FOLDER_TYPE_ARCHIVE,
  MSG_MAIL_FOLDER_TYPE_OTHER,
  MSG_MAIL_FOLDER_TYPE_MAX
} MsgMailFolderType;

MsgMailFolder *
msg_mail_folder_new (void);

MsgMailFolder *
msg_mail_folder_new_from_json (JsonObject  *json_object,
                               GError     **error);

const char *
msg_mail_folder_get_display_name (MsgMailFolder *self);

int
msg_mail_folder_get_unread_item_count (MsgMailFolder *self);

void
msg_mail_folder_set_unread_item_count (MsgMailFolder *self,
                                       guint          count);

int
msg_mail_folder_get_total_item_count (MsgMailFolder *self);

void
msg_mail_folder_set_total_item_count (MsgMailFolder *self,
                                      guint          count);

const char *
msg_mail_folder_get_id (MsgMailFolder *self);

void
msg_mail_folder_set_id (MsgMailFolder *self,
                        const char    *id);

MsgMailFolderType
msg_mail_folder_get_folder_type (MsgMailFolder *self);

void
msg_mail_folder_set_folder_type (MsgMailFolder            *self,
                                 MsgMailFolderType  type);

void
msg_mail_folder_set_display_name (MsgMailFolder *self,
                                  const char    *display_name);

void
msg_mail_folder_set_delta_link (MsgMailFolder *self,
                                const char    *delta_link);

const char *
msg_mail_folder_get_delta_link (MsgMailFolder *self);

const char *
msg_mail_folder_get_parent_id (MsgMailFolder *self);

void
msg_mail_folder_set_parent_id (MsgMailFolder *self,
                               const char    *id);

int
msg_mail_folder_get_child_folder_count (MsgMailFolder *self);

void
msg_mail_folder_set_child_folder_count (MsgMailFolder *self,
                                        guint          count);

G_END_DECLS
