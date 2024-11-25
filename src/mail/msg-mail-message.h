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

#pragma once

#if !defined(_MSG_INSIDE) && !defined(MSG_COMPILATION)
#error "Only <msg.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

#define MSG_TYPE_MAIL_MESSAGE (msg_mail_message_get_type ())

G_DECLARE_FINAL_TYPE (MsgMailMessage, msg_mail_message, MSG, MAIL_MESSAGE, GObject);

MsgMailMessage *
msg_mail_message_new (void);

MsgMailMessage *
msg_mail_message_new_from_json (JsonObject  *json_object,
                         GError     **error);

const char *
msg_mail_message_get_subject (MsgMailMessage *self);

void
msg_mail_message_set_subject (MsgMailMessage *self,
                         const char *subject);

const char *
msg_mail_message_get_body_preview (MsgMailMessage *self);

void
msg_mail_message_set_body_preview (MsgMailMessage *self,
                              const char *preview);

void
msg_mail_message_set_body (MsgMailMessage *self,
                      const char *body);

const char *
msg_mail_message_get_id (MsgMailMessage *self);

void
msg_mail_message_set_id (MsgMailMessage *self,
                    const char *id);

int
msg_mail_message_get_unread (MsgMailMessage *self);

void
msg_mail_message_set_unread (MsgMailMessage *self,
                        int         unread);

const char *
msg_mail_message_get_sender (MsgMailMessage *self);

void
msg_mail_message_set_sender (MsgMailMessage *self,
                        const char *sender);

GDateTime *
msg_mail_message_get_received_date (MsgMailMessage *self);

void
msg_mail_message_set_received_date (MsgMailMessage *self,
                               gint64      timestamp);

const char *
msg_mail_message_get_body (MsgMailMessage *self, gboolean *is_html);

const char *
msg_mail_message_get_receiver (MsgMailMessage *self);

void
msg_mail_message_set_receiver (MsgMailMessage *self,
                          const char *receiver);

const char *
msg_mail_message_get_cc (MsgMailMessage *self);

void
msg_mail_message_set_cc (MsgMailMessage *self,
                    const char *cc);

gboolean
msg_mail_message_get_has_attachment (MsgMailMessage *self);

void
msg_mail_message_set_has_attachment (MsgMailMessage *self,
                                gboolean    has_attachment);

G_END_DECLS
