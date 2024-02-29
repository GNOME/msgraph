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
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contacts", NULL);

next:
  message = msg_service_build_message (MSG_SERVICE (self), "GET", url, NULL, FALSE);
  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, error);
  if (error && *error)
    return NULL;

  parser = msg_service_parse_response (response, &root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
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

/**
 * msg_contact_service_create:
 * @self: a #MsgContextService
 * @contact: a #MsgContact
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Create new contact #contact and return new contact object.
 *
 * Returns: (transfer full): a new #MsgContact
 */
MsgContact *
msg_contact_service_create (MsgContactService  *self,
                            MsgContact         *contact,
                            GCancellable       *cancellable,
                            GError            **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autoptr (GError) local_error = NULL;
  JsonObject *root_object = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;
  MsgContact *new_contact = NULL;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) node = NULL;
  g_autofree char *json = NULL;
  GBytes *body = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return NULL;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contacts", NULL);
  message = msg_service_build_message (MSG_SERVICE (self), "POST", url, NULL, FALSE);

  builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "givenName");
  json_builder_add_string_value (builder, msg_contact_get_given_name (contact));
  json_builder_set_member_name (builder, "surname");
  json_builder_add_string_value (builder, msg_contact_get_surname (contact));
  json_builder_end_object (builder);
  node = json_builder_get_root (builder);
  json = json_to_string (node, TRUE);

  body = g_bytes_new (json, strlen (json));
  soup_message_set_request_body_from_bytes (message, "application/json", body);

  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  parser = msg_service_parse_response (response, &root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  new_contact = msg_contact_new_from_json (root_object, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return NULL;
  }

  return new_contact;
}

/**
 * msg_contact_service_delete:
 * @self: a #MsgContactService
 * @contact: a #MsgContact
 * @cancellable: a #GCancellable
 * @error: a #GError
 *
 * Delets #contact.
 *
 * Returns: %TRUE for succes, else %FALSE
 */
gboolean
msg_contact_service_delete (MsgContactService  *self,
                            MsgContact         *contact,
                            GCancellable       *cancellable,
                            GError            **error)
{
  g_autoptr (SoupMessage) message = NULL;
  g_autoptr (GError) local_error = NULL;
  g_autofree char *url = NULL;
  g_autoptr (GBytes) response = NULL;
  g_autoptr (JsonParser) parser = NULL;

  if (!msg_service_refresh_authorization (MSG_SERVICE (self), cancellable, error))
    return FALSE;

  url = g_strconcat (MSG_API_ENDPOINT, "/me/contacts/", msg_contact_get_id (contact), NULL);
  message = msg_service_build_message (MSG_SERVICE (self), "DELETE", url, NULL, FALSE);
  response = msg_service_send_and_read (MSG_SERVICE (self), message, cancellable, &local_error);
  if (local_error) {
    g_propagate_error (error, g_steal_pointer (&local_error));
    return FALSE;
  }

  return TRUE;
}
