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

#define MSG_TYPE_MESSAGE (msg_message_get_type ())

G_DECLARE_FINAL_TYPE (MsgMessage, msg_message, MSG, MESSAGE, GObject);

MsgMessage *
msg_message_new (void);

MsgMessage *
msg_message_new_from_json (JsonObject  *json_object,
                         GError     **error);

const char *
msg_message_get_subject (MsgMessage *self);

gboolean
msg_message_set_subject (MsgMessage *self,
                         const char *subject);

const char *
msg_message_get_body_preview (MsgMessage *self);

gboolean
msg_message_set_body (MsgMessage *self,
                      const char *body);

const char *
msg_message_get_id (MsgMessage *self);

gboolean
msg_message_get_unread (MsgMessage *self);

const char *
msg_message_get_sender (MsgMessage *self);

GDateTime *
msg_message_get_received_date (MsgMessage *self);

const char *
msg_message_get_body (MsgMessage *self, gboolean *is_html);

G_END_DECLS
