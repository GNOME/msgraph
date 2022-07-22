/* Copyright 2022-2023 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#include "message/msg-message.h"
#include "msg-error.h"

/**
 * MsgMessage:
 *
 * Handling of message specific functions.
 *
 * Details: https://learn.microsoft.com/en-us/graph/api/resources/message?view=graph-rest-1.0
 */

struct _MsgMessage {
  GObject parent_instance;

  char *subject;
  char *body_preview;
};

G_DEFINE_TYPE (MsgMessage, msg_message, G_TYPE_OBJECT);

static void
msg_message_finalize (GObject *object)
{
  MsgMessage *self = MSG_MESSAGE (object);

  g_clear_pointer (&self->subject, g_free);
  g_clear_pointer (&self->body_preview, g_free);

  G_OBJECT_CLASS (msg_message_parent_class)->finalize (object);
}

static void
msg_message_init (__attribute__ ((unused)) MsgMessage *self)
{
}

static void
msg_message_class_init (MsgMessageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_message_finalize;
}

/**
 * msg_message_new:
 *
 * Creates a new `MsgMessage`.
 *
 * Returns: the newly created `MsgMessage`
 */
MsgMessage *
msg_message_new (void)
{
  return g_object_new (MSG_TYPE_MESSAGE, NULL);
}

/**
 * msg_message_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgMessage` from json response object.
 *
 * Returns: the newly created `MsgMessage`
 */
MsgMessage *
msg_message_new_from_json (JsonObject                       *json_object,
                           __attribute__ ((unused)) GError **error)
{
  MsgMessage *self;

  self = msg_message_new ();
  g_assert (self != NULL);

  if (json_object_has_member (json_object, "subject"))
    self->subject = g_strdup (json_object_get_string_member (json_object, "subject"));
  else
    self->subject = g_strdup ("");

  if (json_object_has_member (json_object, "bodyPreview"))
    self->body_preview = g_strdup (json_object_get_string_member (json_object, "bodyPreview"));
  else
    self->body_preview = g_strdup ("");

  return self;
}

/**
 * msg_message_get_subject:
 * @self: a message
 *
 * Returns: (transfer none): subject of message
 */
const char *
msg_message_get_subject (MsgMessage *self)
{
  return self->subject;
}

/**
 * msg_message_get_body_preview:
 * @self: a message
 *
 * Returns: (transfer none): body preview of message
 */
const char *
msg_message_get_body_preview (MsgMessage *self)
{
  return self->body_preview;
}
