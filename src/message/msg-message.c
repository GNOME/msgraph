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

#include "message/msg-message.h"
#include "msg-error.h"
#include "msg-json-utils.h"

/**
 * MsgMessage:
 *
 * Handling of message specific functions.
 *
 * Details: https://learn.microsoft.com/en-us/graph/api/resources/message?view=graph-rest-1.0
 */

struct _MsgMessage {
  GObject parent_instance;

  char *id;
  char *subject;
  char *body_preview;
  char *body;
};

G_DEFINE_TYPE (MsgMessage, msg_message, G_TYPE_OBJECT);

static void
msg_message_finalize (GObject *object)
{
  MsgMessage *self = MSG_MESSAGE (object);

  g_clear_pointer (&self->subject, g_free);
  g_clear_pointer (&self->body_preview, g_free);
  g_clear_pointer (&self->body, g_free);
  g_clear_pointer (&self->id, g_free);

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
  self->subject = g_strdup (msg_json_object_get_string (json_object, "subject"));
  self->body_preview = g_strdup (msg_json_object_get_string (json_object, "bodyPreview"));
  self->id = g_strdup (msg_json_object_get_string (json_object, "id"));

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

gboolean
msg_message_set_body (MsgMessage *self,
                      const char *body)
{
  g_clear_pointer (&self->body, g_free);
  self->body = g_strdup (body);
  return TRUE;
}

gboolean
msg_message_set_subject (MsgMessage *self,
                         const char *subject)
{
  g_clear_pointer (&self->subject, g_free);
  self->subject = g_strdup (subject);
  return TRUE;
}

const char *
msg_message_get_id (MsgMessage *self)
{
  return self->id;
}
