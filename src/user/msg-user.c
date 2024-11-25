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

#include "user/msg-user.h"
#include "msg-error.h"
#include "msg-json-utils.h"

/**
 * MsgUser:
 *
 * Handling of user specific functions.
 */

struct _MsgUser {
  GObject parent_instance;

  char *id;

  GList *business_phones;
  char *display_name;
  char *given_name;
  char *mail;
  char *mobile_phone;
  char *office_location;
  char *surname;
  char *company_name;
  char *department;
};

G_DEFINE_TYPE (MsgUser, msg_user, G_TYPE_OBJECT);

static void
msg_user_dispose (GObject *object)
{
  MsgUser *self = MSG_USER (object);

  g_clear_pointer (&self->mail, g_free);
  g_clear_pointer (&self->display_name, g_free);
  g_clear_pointer (&self->given_name, g_free);
  g_clear_pointer (&self->mobile_phone, g_free);
  g_clear_pointer (&self->office_location, g_free);
  g_clear_pointer (&self->surname, g_free);
  g_clear_pointer (&self->company_name, g_free);
  g_clear_pointer (&self->department, g_free);
  g_clear_list (&self->business_phones, g_free);

  G_OBJECT_CLASS (msg_user_parent_class)->dispose (object);
}

static void
msg_user_init (__attribute__ ((unused)) MsgUser *self)
{
}

static void
msg_user_class_init (MsgUserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = msg_user_dispose;
}

/**
 * msg_user_new:
 *
 * Creates a new `MsgUser`.
 *
 * Returns: the newly created `MsgUser`
 */
MsgUser *
msg_user_new (void)
{
  return g_object_new (MSG_TYPE_USER, NULL);
}

/**
 * msg_user_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgUser` from json response object.
 *
 * Returns: the newly created `MsgUser`
 */
MsgUser *
msg_user_new_from_json (JsonObject                       *json_object,
                        __attribute__ ((unused)) GError **error)
{
  MsgUser *self;

  self = msg_user_new ();
  if (json_object_has_member (json_object, "emailAddresses")) {
    JsonArray *email_addresses = json_object_get_array_member (json_object, "emailAddresses");
    JsonObject *email_address = json_array_get_object_element (email_addresses, 0);

    self->mail = g_utf8_strdown (msg_json_object_get_string (email_address, "address"), -1);
  } else {
    self->mail = g_strdup (msg_json_object_get_string (json_object, "mail"));
  }
  self->display_name = g_strdup (msg_json_object_get_string (json_object, "displayName"));
  self->mobile_phone = g_strdup (msg_json_object_get_string (json_object, "mobilePhone"));
  self->office_location = g_strdup (msg_json_object_get_string (json_object, "officeLocation"));
  self->surname = g_strdup (msg_json_object_get_string (json_object, "surname"));
  self->given_name = g_strdup (msg_json_object_get_string (json_object, "givenName"));
  self->company_name = g_strdup (msg_json_object_get_string (json_object, "companyName"));
  self->department = g_strdup (msg_json_object_get_string (json_object, "department"));

  return self;
}

/**
 * msg_user_get_mail:
 * @self: a user
 *
 * Returns: (transfer none): mail of user or %NULL if not existing
 */
const char *
msg_user_get_mail (MsgUser *self)
{
  return self->mail;
}

/**
 * msg_user_get_display_name:
 * @self: a user
 *
 * Returns: (transfer none): display name of user or %NULL if not existing
 */
const char *
msg_user_get_display_name (MsgUser *self)
{
  return self->display_name;
}

/**
 * msg_user_get_mobile_phone:
 * @self: a user
 *
 * Returns: (transfer none): mobile phone of user or %NULL if not existing
 */
const char *
msg_user_get_mobile_phone (MsgUser *self)
{
  return self->mobile_phone;
}

/**
 * msg_user_get_office_location:
 * @self: a user
 *
 * Returns: (transfer none): office location of user or %NULL if not existing
 */
const char *
msg_user_get_office_location (MsgUser *self)
{
  return self->office_location;
}

/**
 * msg_user_get_surname:
 * @self: a user
 *
 * Returns: (transfer none): surname of user or %NULL if not existing
 */
const char *
msg_user_get_surname (MsgUser *self)
{
  return self->surname;
}

/**
 * msg_user_get_given_name:
 * @self: a user
 *
 * Returns: (transfer none): given name of user or %NULL if not existing
 */
const char *
msg_user_get_given_name (MsgUser *self)
{
  return self->given_name;
}

/**
 * msg_user_get_company_name:
 * @self: a user
 *
 * Returns: (transfer none): company name of user or %NULL if not existing
 */
const char *
msg_user_get_company_name (MsgUser *self)
{
  return self->company_name;
}

/**
 * msg_user_get_department:
 * @self: a user
 *
 * Returns: (transfer none): department of user or %NULL if not existing
 */
const char *
msg_user_get_department (MsgUser *self)
{
  return self->department;
}
