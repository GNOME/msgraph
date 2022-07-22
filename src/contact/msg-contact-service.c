/* Copyright 2023-2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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
#include "msg-contact.h"
#include "msg-contact-service.h"

struct _MsgContactService {
  MsgService parent_instance;
};

G_DEFINE_TYPE (MsgContactService, msg_contact_service, MSG_TYPE_SERVICE);

static void
msg_contact_service_init (__attribute__ ((unused)) MsgContactService *self)
{
}

static void
msg_contact_service_class_init (__attribute__ ((unused)) MsgContactServiceClass *class)
{
}

MsgContactService *
msg_contact_service_new (MsgAuthorizer *authorizer)
{
  return g_object_new (MSG_TYPE_CONTACT_SERVICE, "authorizer", authorizer, NULL);
}

/**
 * msg_contact_service_get_contacts:
 * @self: a contact service
 * @cancellable: a cancellable
 * @error: a error
 *
 * Get all contacts accessed by contact service.
 *
 * Returns: (element-type MsgContact) (transfer full): all contacts
 */
GList *
msg_contact_service_get_contacts (MsgContactService  *self,
                                  GCancellable       *cancellable,
                                  GError            **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autoptr (GError) local_error = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  GList *list = NULL;
  JsonArray *array = NULL;
  guint array_length = 0, index = 0;
  g_autoptr (GBytes) response = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contacts", NULL);
  g_debug ("%s: Sending message to %s\n", __FUNCTION__, url);

next:
  message = soup_message_new ("GET", url);
  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, local_error);
    return NULL;
  }

  root_object = msg_service_parse_response (response, &local_error);
  if (!root_object) {
    g_propagate_error (error, local_error);
    return NULL;
  }

  array = json_object_get_array_member (root_object, "value");
  g_assert (array != NULL);

  array_length = json_array_get_length (array);
  for (index = 0; index < array_length; index++) {
    MsgContact *contact = NULL;
    JsonObject *contact_object = NULL;

    contact_object = json_array_get_object_element (array, index);

    contact = msg_contact_new_from_json (contact_object, &local_error);
    if (contact) {
      list = g_list_append (list, contact);
    } else {
      g_warning ("Could not parse contact object: %s", local_error->message);
      g_clear_error (&local_error);
    }
  }

  if (json_object_has_member (root_object, "@odata.nextLink")) {
    g_clear_pointer (&url, g_free);

    url = g_strdup (json_object_get_string_member (root_object, "@odata.nextLink"));
    g_clear_object (&message);
    goto next;
  }

  return list;
}
