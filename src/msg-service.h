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

#include <glib-object.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "msg-authorizer.h"

G_BEGIN_DECLS

#define MSG_TYPE_SERVICE (msg_service_get_type ())

struct _MsgServiceClass {
  GObjectClass parent;
};

G_DECLARE_DERIVABLE_TYPE (MsgService, msg_service, MSG, SERVICE, GObject);

gboolean
msg_service_refresh_authorization(MsgService    *self,
                                  GCancellable  *cancellable,
                                  GError       **error);

GInputStream *
msg_service_send (MsgService    *self,
                  SoupMessage   *message,
                  GCancellable  *cancellable,
                  GError       **error);

GBytes *
msg_service_send_and_read (MsgService    *self,
                           SoupMessage   *message,
                           GCancellable  *cancellable,
                           GError       **error);

JsonParser *
msg_service_parse_response (GBytes        *bytes,
                            JsonObject   **object,
                            GError       **error);

SoupMessage *
msg_service_build_message (MsgService *self,
                           const char *method,
                           const char *uri,
                           const char *etag,
                           gboolean    etag_if_match);

gboolean
msg_service_accept_certificate_cb (SoupMessage          *msg,
                                   GTlsCertificate      *tls_cert,
                                   GTlsCertificateFlags  tls_errors,
                                   gpointer              session);

guint
msg_service_get_https_port (void);

SoupSession *
msg_service_get_session (MsgService *self);

MsgAuthorizer *
msg_service_get_authorizer (MsgService *self);

JsonParser *
msg_service_send_and_parse_response (MsgService    *self,
                                     SoupMessage   *message,
                                     JsonObject   **object,
                                     GCancellable  *cancellable,
                                     GError       **error);

char *
msg_service_get_next_link (JsonObject *object);

gboolean
msg_service_handle_rate_limiting (SoupMessage *msg);

G_END_DECLS
