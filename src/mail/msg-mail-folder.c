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

#include "mail/msg-mail-folder.h"
#include "msg-error.h"
#include "msg-json-utils.h"

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

  MsgMailFolderType type;
  char *delta_link;
};

G_DEFINE_TYPE (MsgMailFolder, msg_mail_folder, G_TYPE_OBJECT);

static void
msg_mail_folder_dispose (GObject *object)
{
  MsgMailFolder *self = MSG_MAIL_FOLDER (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->parent_folder_id, g_free);
  g_clear_pointer (&self->display_name, g_free);
  g_clear_pointer (&self->delta_link, g_free);

  G_OBJECT_CLASS (msg_mail_folder_parent_class)->dispose (object);
}

static void
msg_mail_folder_init (MsgMailFolder *self)
{
  self->type = MSG_MAIL_FOLDER_TYPE_OTHER;
}

static void
msg_mail_folder_class_init (MsgMailFolderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = msg_mail_folder_dispose;
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

  self->type = MSG_MAIL_FOLDER_TYPE_OTHER;
  self->id = g_strdup (msg_json_object_get_string (json_object, "id"));
  self->display_name = g_strdup (msg_json_object_get_string (json_object, "displayName"));
  self->parent_folder_id = g_strdup (msg_json_object_get_string (json_object, "parentFolderId"));

  self->child_folder_count = json_object_get_int_member (json_object, "childFolderCount");
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

/**
 * msg_mail_folder_get_unread_item_count:
 * @self: a mail folder
 *
 * Returns: unread item number of mail folder
 */
int
msg_mail_folder_get_unread_item_count (MsgMailFolder *self)
{
  return self->unread_item_count;
}

void
msg_mail_folder_set_unread_item_count (MsgMailFolder *self,
                                       guint          count)
{
  self->unread_item_count = count;
}

/**
 * msg_mail_folder_get_total_item_count:
 * @self: a mail folder
 *
 * Returns: total item number of mail folder
 */
int
msg_mail_folder_get_total_item_count (MsgMailFolder *self)
{
  return self->total_item_count;
}

void
msg_mail_folder_set_total_item_count (MsgMailFolder *self,
                                      guint          count)
{
  self->total_item_count = count;
}

/**
 * msg_mail_folder_get_id:
 * @self: a mail folder
 *
 * Returns: id of mail folder
 */
const char *
msg_mail_folder_get_id (MsgMailFolder *self)
{
  return self->id;
}

void
msg_mail_folder_set_id (MsgMailFolder *self,
                        const char    *id)
{
  g_clear_pointer (&self->id, g_free);
  self->id = g_strdup (id);
}

void
msg_mail_folder_set_display_name (MsgMailFolder *self,
                                  const char    *display_name)
{
  g_clear_pointer (&self->display_name, g_free);
  self->display_name = g_strdup (display_name);
}

MsgMailFolderType
msg_mail_folder_get_folder_type (MsgMailFolder *self)
{
  return self->type;
}

void
msg_mail_folder_set_folder_type (MsgMailFolder     *self,
                                 MsgMailFolderType  type)
{
  self->type = type;
}

void
msg_mail_folder_set_delta_link (MsgMailFolder *self,
                                const char    *delta_link)
{
  g_clear_pointer (&self->delta_link, g_free);
  self->delta_link = g_strdup (delta_link);
}

const char *
msg_mail_folder_get_delta_link (MsgMailFolder *self)
{
  return self->delta_link;
}

const char *
msg_mail_folder_get_parent_id (MsgMailFolder *self)
{
  return self->parent_folder_id;
}

void
msg_mail_folder_set_parent_id (MsgMailFolder *self,
                               const char    *id)
{
  g_clear_pointer (&self->parent_folder_id, g_free);
  self->parent_folder_id = g_strdup (id);
}

int
msg_mail_folder_get_child_folder_count (MsgMailFolder *self)
{
  return self->child_folder_count;
}

void
msg_mail_folder_set_child_folder_count (MsgMailFolder *self,
                                      guint          count)
{
  self->child_folder_count = count;
}

