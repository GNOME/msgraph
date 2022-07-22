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

  char *name;
};

G_DEFINE_TYPE (MsgContact, msg_contact, G_TYPE_OBJECT);

static void
msg_contact_finalize (GObject *object)
{
  MsgContact *self = MSG_CONTACT (object);

  g_clear_pointer (&self->name, g_free);

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
  const char *contact_type = NULL;

  self = msg_contact_new ();
  g_assert (self != NULL);

  if (json_object_has_member (json_object, "displayName"))
    self->name = g_strdup (json_object_get_string_member (json_object, "displayName"));
  else
    self->name = g_strdup (contact_type);

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
