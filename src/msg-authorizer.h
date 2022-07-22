/* Copyright 2022 Jan-Michael Brummer
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

#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

#define MSG_TYPE_AUTHORIZER (msg_authorizer_get_type ())
#define MSG_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MSG_TYPE_AUTHORIZER, MsgAuthorizer))
#define MSG_IS_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MSG_TYPE_AUTHORIZER))
#define MSG_AUTHORIZER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MSG_TYPE_AUTHORIZER, MsgAuthorizerInterface))

typedef struct _MsgAuthorizer          MsgAuthorizer;
typedef struct _MsgAuthorizerInterface MsgAuthorizerInterface;

/**
 * MsgAuthorizerInterface:
 * @parent_iface: The parent interface.
 * @process_request: A method to append authorization headers to a
 *   #SoupMessage. Types of messages include DELETE, GET and POST.
 * @refresh_authorization: A synchronous method to force a refresh of
 *   any authorization tokens held by the authorizer. It should return
 *   %TRUE on success. An asynchronous version will be defined by
 *   invoking this in a thread.
 *
 * Interface structure for #MsgAuthorizer. All methods should be
 * thread safe.
 */
struct _MsgAuthorizerInterface
{
  GTypeInterface parent_iface;

  void        (*process_request)             (MsgAuthorizer *iface,
                                              SoupMessage   *message);
  gboolean    (*refresh_authorization)       (MsgAuthorizer  *iface,
                                              GCancellable   *cancellable,
                                              GError        **error);
};

GType               msg_authorizer_get_type                       (void) G_GNUC_CONST;

void                msg_authorizer_process_request                (MsgAuthorizer          *iface,
                                                                   SoupMessage            *message);

gboolean            msg_authorizer_refresh_authorization          (MsgAuthorizer  *iface,
                                                                   GCancellable   *cancellable,
                                                                   GError        **error);

G_END_DECLS

