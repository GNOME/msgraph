/* Copyright 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#define MSG_TYPE_USER (msg_user_get_type ())

G_DECLARE_FINAL_TYPE (MsgUser, msg_user, MSG, USER, GObject);

MsgUser *
msg_user_new (void);

MsgUser *
msg_user_new_from_json (JsonObject  *json_object,
                         GError     **error);

const char *
msg_user_get_mail (MsgUser *self);

const char *
msg_user_get_display_name (MsgUser *self);

const char *
msg_user_get_given_name (MsgUser *self);

const char *
msg_user_get_mail_name (MsgUser *self);

const char *
msg_user_get_mobile_phone (MsgUser *self);

const char *
msg_user_get_office_location (MsgUser *self);

const char *
msg_user_get_surname (MsgUser *self);

const char *
msg_user_get_company_name (MsgUser *self);

const char *
msg_user_get_department (MsgUser *self);

G_END_DECLS
