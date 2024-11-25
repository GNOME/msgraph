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

#include "user/msg-user-contact-folder.h"
#include "msg-error.h"
#include "msg-json-utils.h"

/**
 * MsgUserContactFolder:
 *
 * Handling of user_contact folder specific functions.
 */

struct _MsgUserContactFolder {
  GObject parent_instance;

  char *id;
  char *display_name;
};

G_DEFINE_TYPE (MsgUserContactFolder, msg_user_contact_folder, G_TYPE_OBJECT);

static void
msg_user_contact_folder_dispose (GObject *object)
{
  MsgUserContactFolder *self = MSG_USER_CONTACT_FOLDER (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->display_name, g_free);

  G_OBJECT_CLASS (msg_user_contact_folder_parent_class)->dispose (object);
}

static void
msg_user_contact_folder_init (__attribute__ ((unused)) MsgUserContactFolder *self)
{
}

static void
msg_user_contact_folder_class_init (MsgUserContactFolderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = msg_user_contact_folder_dispose;
}

/**
 * msg_user_contact_folder_new:
 *
 * Creates a new `MsgUserContactFolder`.
 *
 * Returns: the newly created `MsgUserContactFolder`
 */
MsgUserContactFolder *
msg_user_contact_folder_new (void)
{
  return g_object_new (MSG_TYPE_USER_CONTACT_FOLDER, NULL);
}

/**
 * msg_user_contact_folder_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgUserContactFolder` from json response object.
 *
 * Returns: the newly created `MsgUserContactFolder`
 */
MsgUserContactFolder *
msg_user_contact_folder_new_from_json (JsonObject                       *json_object,
                                       __attribute__ ((unused)) GError **error)
{
  MsgUserContactFolder *self;

  self = msg_user_contact_folder_new ();

  self->id = g_strdup (msg_json_object_get_string (json_object, "id"));
  self->display_name = g_strdup (msg_json_object_get_string (json_object, "displayName"));

  return self;
}

/**
 * msg_user_contact_folder_get_display_name:
 * @self: a user_contact folder
 *
 * Returns: (transfer none): display name of user_contact folder
 */
const char *
msg_user_contact_folder_get_display_name (MsgUserContactFolder *self)
{
  return self->display_name;
}

/**
 * msg_user_contact_folder_get_id:
 * @self: a user_contact folder
 *
 * Returns: id of user_contact folder
 */
const char *
msg_user_contact_folder_get_id (MsgUserContactFolder *self)
{
  return self->id;
}
