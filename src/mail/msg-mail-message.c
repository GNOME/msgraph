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

#include "mail/msg-mail-message.h"
#include "msg-error.h"
#include "msg-json-utils.h"

/**
 * MsgMailMessage:
 *
 * Handling of mail message specific functions.
 */

struct _MsgMailMessage {
  GObject parent_instance;

  char *id;
  char *subject;
  char *body_preview;
  char *body;

  char *sender;
  char *receiver;
  char *cc;

  int unread;
  GDateTime *received_date;
  gboolean has_attachment;
};

G_DEFINE_TYPE (MsgMailMessage, msg_mail_message, G_TYPE_OBJECT);

static void
msg_mail_message_dispose (GObject *object)
{
  MsgMailMessage *self = MSG_MAIL_MESSAGE (object);

  g_clear_pointer (&self->subject, g_free);
  g_clear_pointer (&self->body_preview, g_free);
  g_clear_pointer (&self->body, g_free);
  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->sender, g_free);
  g_clear_pointer (&self->receiver, g_free);
  g_clear_pointer (&self->cc, g_free);
  g_clear_pointer (&self->received_date, g_date_time_unref);

  G_OBJECT_CLASS (msg_mail_message_parent_class)->dispose (object);
}

static void
msg_mail_message_init (__attribute__ ((unused)) MsgMailMessage *self)
{
}

static void
msg_mail_message_class_init (MsgMailMessageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = msg_mail_message_dispose;
}

/**
 * msg_mail_message_new:
 *
 * Creates a new `MsgMailMessage`.
 *
 * Returns: the newly created `MsgMailMessage`
 */
MsgMailMessage *
msg_mail_message_new (void)
{
  return g_object_new (MSG_TYPE_MAIL_MESSAGE, NULL);
}

/**
 * msg_mail_message_new_from_json:
 * @json_object: The json object to parse
 * @error: a #GError
 *
 * Creates a new `MsgMailMessage` from json response object.
 *
 * Returns: the newly created `MsgMailMessage`
 */
MsgMailMessage *
msg_mail_message_new_from_json (JsonObject                       *json_object,
                                __attribute__ ((unused)) GError **error)
{
  MsgMailMessage *self;

  self = msg_mail_message_new ();

  if (json_object_has_member (json_object, "from")) {
    JsonObject *obj = json_object_get_object_member (json_object, "from");
    JsonObject *email_address = json_object_get_object_member (obj, "emailAddress");
    const char *address = msg_json_object_get_string (email_address, "address");
    const char *name = msg_json_object_get_string (email_address, "name");

    self->sender = g_strdup_printf ("%s%s<%s>", name ? name : "", name ? " " : "", address);
  }
  self->received_date = g_date_time_new_from_iso8601 (msg_json_object_get_string (json_object, "receivedDateTime"), NULL);

  self->subject = g_strstrip (g_strdup (msg_json_object_get_string (json_object, "subject")));
  if (json_object_has_member (json_object, "body")) {
    JsonObject *obj = json_object_get_object_member (json_object, "body");
    const char *content = msg_json_object_get_string (obj, "content");

    self->body = g_strdup (content);
  }

  self->body_preview = g_strdup (msg_json_object_get_string (json_object, "bodyPreview"));

  if (json_object_has_member (json_object, "isRead")) {
    self->unread = json_object_get_boolean_member (json_object, "isRead") != TRUE;
  } else {
    self->unread = 0;
  }

  self->id = g_strdup (msg_json_object_get_string (json_object, "id"));

  if (json_object_has_member (json_object, "toRecipients")) {
    JsonArray *array = json_object_get_array_member (json_object, "toRecipients");
    int array_length = json_array_get_length (array);
    GString *receiver = g_string_new ("");

    for (int index = 0; index < array_length; index++) {
      JsonObject *to = NULL;

      to = json_array_get_object_element (array, index);
      if (to) {
        JsonObject *email_address = json_object_get_object_member (to, "emailAddress");
        const char *address = msg_json_object_get_string (email_address, "address");
        const char *name = msg_json_object_get_string (email_address, "name");

        if (receiver->len) {
          g_string_append (receiver, ";");
        }
        g_string_append_printf (receiver, "%s%s<%s>", name ? name : "", name ? " " : "", address);
      }
    }

    self->receiver = g_string_free (receiver, FALSE);
  }

  if (json_object_has_member (json_object, "ccRecipients")) {
    JsonArray *array = json_object_get_array_member (json_object, "ccRecipients");
    int array_length = json_array_get_length (array);

    for (int index = 0; index < array_length; index++) {
      JsonObject *to = NULL;

      to = json_array_get_object_element (array, index);
      if (to) {
        JsonObject *email_address = json_object_get_object_member (to, "emailAddress");
        const char *address = msg_json_object_get_string (email_address, "address");
        const char *name = msg_json_object_get_string (email_address, "name");

        self->cc = g_strdup_printf ("%s%s<%s>", name ? name : "", name ? " " : "", address);
      }
    }
  }

  if (json_object_has_member (json_object, "hasAttachments")) {
    self->has_attachment = msg_json_object_get_boolean (json_object, "hasAttachments");
  }

  return self;
}

/**
 * msg_mail_message_get_subject:
 * @self: a mail_message
 *
 * Get mail subject.
 *
 * Returns: (transfer none): subject of mail_message
 */
const char *
msg_mail_message_get_subject (MsgMailMessage *self)
{
  return self->subject;
}

/**
 * msg_mail_message_get_body_preview:
 * @self: a mail_message
 *
 * Get mail body preview.
 *
 * Returns: (transfer none): body preview of mail_message
 */
const char *
msg_mail_message_get_body_preview (MsgMailMessage *self)
{
  return self->body_preview;
}

void
msg_mail_message_set_body_preview (MsgMailMessage *self,
                                   const char     *preview)
{
  g_clear_pointer (&self->body_preview, g_free);
  self->body_preview = g_strdup (preview);
}

/**
 * msg_mail_message_set_body:
 * @self: a mail_message
 * @body: mail body
 *
 * Set mail body.
 */
void
msg_mail_message_set_body (MsgMailMessage *self,
                           const char     *body)
{
  if (self->body == body)
    return;

  g_clear_pointer (&self->body, g_free);
  self->body = g_strdup (body);
}

/**
 * msg_mail_message_set_subject:
 * @self: a mail_message
 * @subject: mail subject
 *
 * Set mail subject.
 */
void
msg_mail_message_set_subject (MsgMailMessage *self,
                              const char     *subject)
{
  if (self->subject == subject)
    return;

  g_clear_pointer (&self->subject, g_free);
  self->subject = g_strdup (subject);
}

/**
 * msg_mail_message_get_id:
 * @self: a mail_message
 *
 * Get id.
 *
 * Returns: (transfer none): unique mail id
 */
const char *
msg_mail_message_get_id (MsgMailMessage *self)
{
  return self->id;
}

/**
 * msg_mail_message_set_id:
 * @self: a mail_message
 * @id: mail_message id
 *
 * Set mail id.
 */
void
msg_mail_message_set_id (MsgMailMessage *self,
                         const char     *id)
{
  g_clear_pointer (&self->id, g_free);
  self->id = g_strdup (id);
}

/**
 * msg_mail_message_get_sender:
 * @self: a mail_message
 *
 * Get mail sender.
 *
 * Returns: (transfer none): mail sender
 */
const char *
msg_mail_message_get_sender (MsgMailMessage *self)
{
  return self->sender;
}

/**
 * msg_mail_message_set_sender:
 * @self: a mail_message
 * @sender: mail_message sender
 *
 * Set mail sender.
 */
void
msg_mail_message_set_sender (MsgMailMessage *self,
                             const char     *sender)
{
  g_clear_pointer (&self->sender, g_free);
  self->sender = g_strdup (sender);
}

/**
 * msg_mail_message_get_receiver:
 * @self: a mail_message
 *
 * Get mail receiver.
 *
 * Returns: (transfer none): mail receiver
 */
const char *
msg_mail_message_get_receiver (MsgMailMessage *self)
{
  return self->receiver;
}

/**
 * msg_mail_message_set_receiver:
 * @self: a mail_message
 * @receiver: mail_message receiver
 *
 * Set mail receiver.
 */
void
msg_mail_message_set_receiver (MsgMailMessage *self,
                               const char     *receiver)
{
  g_clear_pointer (&self->receiver, g_free);
  self->receiver = g_strdup (receiver);
}

/**
 * msg_mail_message_get_cc:
 * @self: a mail_message
 *
 * Get mail cc.
 *
 * Returns: (transfer none): mail cc
 */
const char *
msg_mail_message_get_cc (MsgMailMessage *self)
{
  return self->cc;
}

/**
 * msg_mail_message_set_cc:
 * @self: a mail_message
 * @cc: carbon copy string
 *
 * Set mail cc.
 */
void
msg_mail_message_set_cc (MsgMailMessage *self,
                         const char     *cc)
{
  g_clear_pointer (&self->cc, g_free);
  self->cc = g_strdup (cc);
}

/**
 * msg_mail_message_get_unread:
 * @self: a mail_message
 *
 * Get mail unread.
 *
 * Returns: unread count
 */
int
msg_mail_message_get_unread (MsgMailMessage *self)
{
  return self->unread;
}

/**
 * msg_mail_message_set_unread:
 * @self: a mail_message
 * @unread: unread count
 *
 * Set mail unread count.
 */
void
msg_mail_message_set_unread (MsgMailMessage *self,
                             int             unread)
{
  self->unread = unread;
}

/**
 * msg_mail_message_get_received_date:
 * @self: a mail_message
 *
 * Get mail received date.
 *
 * Returns: (transfer none): received date
 */
GDateTime *
msg_mail_message_get_received_date (MsgMailMessage *self)
{
  return self->received_date;
}

/**
 * msg_mail_message_set_received_date:
 * @self: a mail_message
 * @timestamp: received timestamp
 *
 * Set mail received timestamp.
 */
void
msg_mail_message_set_received_date (MsgMailMessage *self,
                                    gint64          timestamp)
{
  g_clear_pointer (&self->received_date, g_date_time_unref);
  self->received_date = g_date_time_new_from_unix_utc (timestamp);
}

/**
 * msg_mail_message_get_body:
 * @self: a mail_message
 *
 * Get mail body.
 *
 * Returns: (transfer none): mail body
 */
const char *
msg_mail_message_get_body (MsgMailMessage *self,
                           gboolean       *is_html)
{
  if (is_html)
    *is_html = TRUE;

  return self->body;
}

/**
 * msg_mail_message_get_has_attachment:
 * @self: a mail_message
 *
 * Get whether mail has attachments.
 *
 * Returns: %TRUE if mail has attachments otherwise %FALSE
 */
gboolean
msg_mail_message_get_has_attachment (MsgMailMessage *self)
{
  return self->has_attachment;
}

/**
 * msg_mail_message_set_has_attachment:
 * @self: a mail_message
 * @has_attachment: flag to set attachments
 *
 * Set whether mail has attachments.
 */
void
msg_mail_message_set_has_attachment (MsgMailMessage *self,
                                     gboolean        has_attachment)
{
  self->has_attachment = has_attachment;
}
