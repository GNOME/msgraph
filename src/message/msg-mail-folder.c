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

#include "message/msg-mail-folder.h"
#include "msg-error.h"

/**
 * MsgMailFolder:
 *
 * Handling of mail folder specific functions.
 */

struct _MsgMailFolder {
  GObject parent_instance;

  char *id;
  char *parent_folder_id;
  char *display_name;
  int child_folder_count;
  int unread_item_count;
  int total_item_count;
  gboolean is_hidden;
};

G_DEFINE_TYPE (MsgMailFolder, msg_mail_folder, G_TYPE_OBJECT);

static void
msg_mail_folder_finalize (GObject *object)
{
  MsgMailFolder *self = MSG_MAIL_FOLDER (object);

  g_clear_pointer (&self->display_name, g_free);

  G_OBJECT_CLASS (msg_mail_folder_parent_class)->finalize (object);
}

static void
msg_mail_folder_init (__attribute__ ((unused)) MsgMailFolder *self)
{
}

static void
msg_mail_folder_class_init (MsgMailFolderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_mail_folder_finalize;
}

/**
 * msg_mail_folder_new:
 *
 * Creates a new `MsgMailFolder`.
 *
 * Returns: the newly created `MsgMailFolder`
 */
MsgMailFolder *
msg_mail_folder_new (void)
{
  return g_object_new (MSG_TYPE_MAIL_FOLDER, NULL);
}

/**
 * msg_mail_folder_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgMailFolder` from json response object.
 *
 * Returns: the newly created `MsgMailFolder`
 */
MsgMailFolder *
msg_mail_folder_new_from_json (JsonObject                       *json_object,
                               __attribute__ ((unused)) GError **error)
{
  MsgMailFolder *self;

  self = msg_mail_folder_new ();
  g_assert (self != NULL);

  if (json_object_has_member (json_object, "displayName"))
    self->display_name = g_strdup (json_object_get_string_member (json_object, "displayName"));
  else
    self->display_name = g_strdup ("");

  self->unread_item_count = json_object_get_int_member (json_object, "unreadItemCount");
  self->total_item_count = json_object_get_int_member (json_object, "totalItemCount");

  return self;
}

/**
 * msg_mail_folder_get_display_name:
 * @self: a mail folder
 *
 * Returns: (transfer none): display name of mail folder
 */
const char *
msg_mail_folder_get_display_name (MsgMailFolder *self)
{
  return self->display_name;
}

int
msg_mail_folder_get_unread_item_count (MsgMailFolder *self)
{
  return self->unread_item_count;
}

int
msg_mail_folder_get_total_item_count (MsgMailFolder *self)
{
  return self->total_item_count;
}
