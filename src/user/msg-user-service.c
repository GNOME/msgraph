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
#include "msg-user-contact-folder.h"

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
                           const char      *name,
                           GCancellable    *cancellable,
                           GError         **error)
{
  g_autoptr (SoupMessage) message = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  if (!name)
    url = g_strconcat (MSG_API_ENDPOINT, "/me", NULL);
  else
    url = g_strconcat (MSG_BETA_API_ENDPOINT, "/me/contacts/users/", name, NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
  if (!parser)
    return NULL;

  return msg_user_new_from_json (root_object, error);
}

/**
 * msg_user_service_get_photo:
 * @self: a #MsgUserService
 * @mail: mail address
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Try to load user photo using provided %mail address.
 *
 * Returns: (transfer full): user photo or %NULL if not found.
 */
GBytes *
msg_user_service_get_photo (MsgUserService  *self,
                            const char      *mail,
                            GCancellable    *cancellable,
                            GError         **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) photo = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/users/", mail, "/photo/$value", NULL);

  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  photo = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, error);

  if (soup_message_get_status (message) == SOUP_STATUS_OK) {
    return g_steal_pointer (&photo);
  }

  g_set_error (error,
               msg_error_quark (),
               MSG_ERROR_FAILED,
               "Could not load photo");

  return NULL;
}

/**
 * msg_user_service_get_contact_folders:
 * @self: a #MsgUserService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get all folders for given service
 *
 * Returns: (element-type MsgUserContactFolder) (transfer full): all user contact folders the user can access
 */
GList *
msg_user_service_get_contact_folders (MsgUserService  *self,
                                      GCancellable    *cancellable,
                                      GError         **error)
{
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autolist (MsgUserContactFolder) list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contactFolders", NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      MsgUserContactFolder *folder = NULL;
      JsonObject *object = NULL;

      object = json_array_get_object_element (array, index);

      folder = msg_user_contact_folder_new_from_json (object, &local_error);
      if (folder) {
        list = g_list_append (list, folder);
      } else {
        g_warning ("Could not parse contact folder object: %s", local_error->message);
        g_clear_error (&local_error);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}

/**
 * msg_user_service_get_contacts:
 * @self: a #MsgUserService
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Get all contats within users 'Contact' folder.
 *
 * Returns: (element-type MsgUserContactFolder) (transfer full): all contact in users contact folder
 */
GList *
msg_user_service_get_contacts (MsgUserService  *self,
                               GCancellable    *cancellable,
                               GError         **error)
{
  g_autoptr (SoupMessage) soup_message = NULL;
  g_autofree char *url = NULL;
  JsonObject *root_object = NULL;
  g_autolist (MsgUser) list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contacts/", NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      MsgUser *user = NULL;
      JsonObject *object = NULL;

      object = json_array_get_object_element (array, index);

      user = msg_user_new_from_json (object, &local_error);
      if (user) {
        list = g_list_append (list, user);
      } else {
        g_warning ("Could not parse contact user object: %s", local_error->message);
        g_clear_error (&local_error);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}

/**
 * msg_user_service_find_users:
 * @self: a #MsgUserService
 * @name: name to search
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Find all users with the given @name. (Business accounts only!)
 *
 * Returns: (element-type MsgUser) (transfer full): a list of contacts with the given name.
 */
GList *
msg_user_service_find_users (MsgUserService  *self,
                             const char      *display_name,
                             GCancellable    *cancellable,
                             GError         **error)
{
  g_autoptr (SoupMessage) soup_message = NULL;
  g_autofree char *url = NULL;
  JsonObject *root_object = NULL;
  g_autolist (MsgUser) list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/users?$search=\"displayName:", display_name, "\"", NULL);

  do {
    g_autoptr (SoupMessage) message = NULL;

    message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
    soup_message_headers_append (soup_message_get_request_headers (message), "ConsistencyLevel", "eventual");
    parser = msg_service_send_and_parse_response (MSG_SERVICE (self), message, &root_object, cancellable, error);
    if (!parser)
      return NULL;

    array = json_object_get_array_member (root_object, "value");
    g_assert (array != NULL);

    array_length = json_array_get_length (array);
    for (index = 0; index < array_length; index++) {
      g_autoptr (GError) local_error = NULL;
      MsgUser *user = NULL;
      JsonObject *object = NULL;

      object = json_array_get_object_element (array, index);

      user = msg_user_new_from_json (object, &local_error);
      if (user) {
        list = g_list_append (list, user);
      } else {
        g_warning ("Could not parse contact user object: %s", local_error->message);
        g_clear_error (&local_error);
      }
    }

    g_clear_pointer (&url, g_free);
    url = msg_service_get_next_link (root_object);
  } while (url != NULL);

  return g_steal_pointer (&list);
}
