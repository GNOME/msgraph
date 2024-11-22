/* Copyright 2023-2024 Jan-Michael Brummer
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
#include <msg-service.h>
#include <user/msg-user.h>

#define MSG_TYPE_USER_SERVICE (msg_user_service_get_type ())

G_DECLARE_FINAL_TYPE (MsgUserService, msg_user_service, MSG, USER_SERVICE, MsgService);

MsgUserService *msg_user_service_new (MsgAuthorizer *authorizer);

MsgUser *
msg_user_service_get_user (MsgUserService  *self,
                           const char      *name,
                           GCancellable    *cancellable,
                           GError         **error);

GBytes *
msg_user_service_get_photo (MsgUserService  *self,
                            const char      *mail,
                            GCancellable    *cancellable,
                            GError         **error);

GList *
msg_user_service_get_contacts (MsgUserService  *self,
                               GCancellable    *cancellable,
                               GError         **error);

GList *
msg_user_service_find_users (MsgUserService  *self,
                             const char      *name,
                             GCancellable    *cancellable,
                             GError         **error);

GList *
msg_user_service_get_contact_folders (MsgUserService  *self,
                                      GCancellable    *cancellable,
                                      GError         **error);
