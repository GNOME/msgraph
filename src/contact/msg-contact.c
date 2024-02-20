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

#include "contact/msg-contact.h"
#include "msg-error.h"

/**
 * MsgContact:
 *
 * Handling of contact specific functions.
 *
 * Details: https://learn.microsoft.com/en-us/graph/api/resources/contact?view=graph-rest-1.0
 */

struct _MsgContact {
  GObject parent_instance;

  char *id;
  char *name;
  char *given_name;
  char *surname;
};

G_DEFINE_TYPE (MsgContact, msg_contact, G_TYPE_OBJECT);

static void
msg_contact_finalize (GObject *object)
{
  MsgContact *self = MSG_CONTACT (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->given_name, g_free);
  g_clear_pointer (&self->surname, g_free);

  G_OBJECT_CLASS (msg_contact_parent_class)->finalize (object);
}

static void
msg_contact_init (__attribute__ ((unused)) MsgContact *self)
{
}

static void
msg_contact_class_init (MsgContactClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_contact_finalize;
}

/**
 * msg_contact_new:
 *
 * Creates a new `MsgContact`.
 *
 * Returns: the newly created `MsgContact`
 */
MsgContact *
msg_contact_new (void)
{
  return g_object_new (MSG_TYPE_CONTACT, NULL);
}

/**
 * msg_contact_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgContact` from json response object.
 *
 * Returns: the newly created `MsgContact`
 */
MsgContact *
msg_contact_new_from_json (JsonObject                       *json_object,
                           __attribute__ ((unused)) GError **error)
{
  MsgContact *self;

  self = msg_contact_new ();
  g_assert (self != NULL);

  if (json_object_has_member (json_object, "displayName"))
    self->name = g_strdup (json_object_get_string_member (json_object, "displayName"));
  if (json_object_has_member (json_object, "givenName"))
    self->given_name = g_strdup (json_object_get_string_member (json_object, "givenName"));
  if (json_object_has_member (json_object, "surname"))
    self->surname = g_strdup (json_object_get_string_member (json_object, "surname"));
  if (json_object_has_member (json_object, "id"))
    self->id = g_strdup (json_object_get_string_member (json_object, "id"));

  return self;
}

/**
 * msg_contact_get_name:
 * @self: a contact
 *
 * Returns: (transfer none): name of contact
 */
const char *
msg_contact_get_name (MsgContact *self)
{
  return self->name;
}

/**
 * msg_contact_set_given_name:
 * @self: a #MsgContact
 * @given_name: new give name
 *
 * Sets contacts given name
 */
void
msg_contact_set_given_name (MsgContact *self,
                            const char *given_name)
{
  g_clear_pointer (&self->given_name, g_free);
  self->given_name = g_strdup (given_name);
}

/**
 * msg_contact_get_given_name:
 * @self: a #MsgContact
 *
 * Gets given name.
 *
 * Returns: (transfer none): given name
 */
const char *
msg_contact_get_given_name (MsgContact *self)
{
  return self->given_name;
}

/**
 * msg_contact_set_surname:
 * @self: a #MsgContact
 * @surname: new sirname
 *
 * Sets contacts surname
 */
void
msg_contact_set_surname (MsgContact *self,
                         const char *surname)
{
  g_clear_pointer (&self->surname, g_free);
  self->surname = g_strdup (surname);
}

/**
 * msg_contact_get_surname:
 * @self: a #MsgContact
 *
 * Gets surname.
 *
 * Returns: (transfer none): surname
 */
const char *
msg_contact_get_surname (MsgContact *self)
{
  return self->surname;
}

/**
 * msg_contact_get_id:
 * @self: a #MsgContact
 *
 * Gets ID
 *
 * Returns: (transfer none): identifier
 */
const char *
msg_contact_get_id (MsgContact *self)
{
  return self->id;
}
