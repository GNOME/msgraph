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

  char *mail;
};

G_DEFINE_TYPE (MsgUser, msg_user, G_TYPE_OBJECT);

static void
msg_user_finalize (GObject *object)
{
  MsgUser *self = MSG_USER (object);

  g_clear_pointer (&self->mail, g_free);

  G_OBJECT_CLASS (msg_user_parent_class)->finalize (object);
}

static void
msg_user_init (__attribute__ ((unused)) MsgUser *self)
{
}

static void
msg_user_class_init (MsgUserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_user_finalize;
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
  self->mail = g_strdup (msg_json_object_get_string (json_object, "mail"));

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
