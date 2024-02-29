/* Copyright 2024 Jan-Michael Brummer
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

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include <stdio.h>

#include "msg-authorizer.h"
#include "msg-error.h"
#include "msg-private.h"
#include "msg-service.h"
#include "msg-user.h"
#include "msg-user-service.h"

struct _MsgUserService {
  MsgService parent_instance;
};

G_DEFINE_TYPE (MsgUserService, msg_user_service, MSG_TYPE_SERVICE);

static void
msg_user_service_init (__attribute__ ((unused)) MsgUserService *self)
{
}

static void
msg_user_service_class_init (__attribute__ ((unused)) MsgUserServiceClass *class)
{
}

MsgUserService *
msg_user_service_new (MsgAuthorizer *authorizer)
{
  return g_object_new (MSG_TYPE_USER_SERVICE, "authorizer", authorizer, NULL);
}

/**
 * msg_user_service_get_user:
 * @self: a #MsgUserService
 * @name: user name (%NULL for me)
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get user information
 *
 * Returns: (transfer full): request user
 */
MsgUser *
msg_user_service_get_user (MsgUserService  *self,
                           char            *name,
                           GCancellable    *cancellable,
                           GError         **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autoptr (GError) local_error = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (MsgUser) user = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  if (!name)
    url = g_strconcat (MSG_API_ENDPOINT, "/me", NULL);
  else
    url = g_strconcat (MSG_API_ENDPOINT, "/users/", name, NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, local_error);
    return NULL;
  }

  parser = msg_service_parse_response (response, &root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, local_error);
    return NULL;
  }

  user = msg_user_new_from_json (root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, local_error);
    return NULL;
  }

  return g_steal_pointer (&user);
}
