/* Copyright 2022-2024 Jan-Michael Brummer
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

#include <config.h>

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <libsoup/soup.h>

#include "msg-input-stream.h"
#include "msg-service.h"

static void msg_input_stream_seekable_iface_init (GSeekableIface *seekable_iface);

struct MsgInputStreamPrivate {
  char *uri;
  MsgService *service;
  SoupMessage *msg;
  GInputStream *stream;

  char *range;
  goffset request_offset;
  goffset offset;
};

G_DEFINE_TYPE_WITH_CODE (MsgInputStream, msg_input_stream, G_TYPE_INPUT_STREAM,
                         G_ADD_PRIVATE (MsgInputStream)
                         G_IMPLEMENT_INTERFACE (G_TYPE_SEEKABLE,
                                                msg_input_stream_seekable_iface_init))

static void
msg_input_stream_init (MsgInputStream *stream)
{
  stream->priv = msg_input_stream_get_instance_private (stream);
}

static void
msg_input_stream_finalize (GObject *object)
{
  MsgInputStream *stream = MSG_INPUT_STREAM (object);
  MsgInputStreamPrivate *priv = stream->priv;

  g_clear_pointer (&priv->uri, g_free);
  g_clear_object (&priv->service);
  g_clear_object (&priv->msg);
  g_clear_object (&priv->stream);
  g_free (priv->range);

  G_OBJECT_CLASS (msg_input_stream_parent_class)->finalize (object);
}

/**
 * msg_input_stream_new:
 * @service: a #MsgService
 * @uri: a #GUri
 *
 * Prepares to send a GET request for @uri on @session, and returns a
 * #GInputStream that can be used to read the response.
 *
 * The request will not be sent until the first read call; if you need
 * to look at the status code or response headers before reading the
 * body, you can use msg_input_stream_send() or
 * msg_input_stream_send_async() to force the message to be
 * sent and the response headers read.
 *
 * Returns: (transfer full): a new #GInputStream.
 **/
GInputStream *
msg_input_stream_new (MsgService *service,
                      char       *uri)
{
  MsgInputStream *stream;
  MsgInputStreamPrivate *priv;

  stream = g_object_new (MSG_TYPE_INPUT_STREAM, NULL);
  priv = stream->priv;

  priv->service = g_object_ref (service);
  priv->uri = g_strdup (uri);

  return G_INPUT_STREAM (stream);
}

static void
on_restarted (SoupMessage                      *msg,
              __attribute__ ((unused)) gpointer user_data)
{
  SoupMessageHeaders *headers = soup_message_get_request_headers (msg);
  soup_message_headers_remove (headers, "Authorization");
}

static SoupMessage *
msg_input_stream_ensure_msg (GInputStream *stream)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (stream)->priv;

  if (!priv->msg){
    priv->msg = msg_service_build_message (MSG_SERVICE (priv->service), "GET", priv->uri, NULL, FALSE);
    g_signal_connect (G_OBJECT (priv->msg), "restarted", G_CALLBACK (on_restarted), NULL);
    msg_authorizer_process_request (msg_service_get_authorizer (priv->service), priv->msg);

    priv->offset = 0;
  }


  if (priv->range)
    soup_message_headers_replace (soup_message_get_request_headers (priv->msg),
                                  "Range", priv->range);

  return priv->msg;
}

static void
read_callback (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  GTask *task = user_data;
  GInputStream *vfsstream = g_task_get_source_object (task);
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (vfsstream)->priv;
  GError *error = NULL;
  gssize nread;

  nread = g_input_stream_read_finish (G_INPUT_STREAM (object), result, &error);
  if (nread >= 0){
    priv->offset += nread;
    g_task_return_int (task, nread);
  } else
    g_task_return_error (task, error);
  g_object_unref (task);
}

typedef struct {
  gpointer buffer;
  gsize count;
} ReadAfterSendData;

static void
read_send_callback (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GTask *task = user_data;
  GInputStream *vfsstream = g_task_get_source_object (task);
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (vfsstream)->priv;
  ReadAfterSendData *rasd = g_task_get_task_data (task);
  GError *error = NULL;

  priv->stream = soup_session_send_finish (SOUP_SESSION (object), result, &error);
  if (!priv->stream){
    g_task_return_error (task, error);
    g_object_unref (task);
    return;
  }
  if (!SOUP_STATUS_IS_SUCCESSFUL (soup_message_get_status (priv->msg))){
    if (soup_message_get_status (priv->msg) == SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE){
      g_input_stream_close (priv->stream, NULL, NULL);
      g_task_return_int (task, 0);
      g_clear_object (&priv->stream);
      g_object_unref (task);
      return;
    }
    g_task_return_new_error (task,
                             G_IO_ERROR,
                             soup_message_get_status (priv->msg),
                             _("HTTP Error: %s"), soup_message_get_reason_phrase (priv->msg));
    g_object_unref (task);
    return;
  }
  if (priv->range){
    gboolean status;
    goffset start, end;

    status = soup_message_headers_get_content_range (soup_message_get_response_headers (priv->msg),
                                                     &start, &end, NULL);
    if (!status || start != priv->request_offset){
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Error seeking in stream"));
      g_object_unref (task);
      return;
    }
  }

  g_input_stream_read_async (priv->stream, rasd->buffer, rasd->count,
                             g_task_get_priority (task),
                             g_task_get_cancellable (task),
                             read_callback, task);
}

static gssize
msg_input_stream_read_fn (GInputStream  *stream,
                          void          *buffer,
                          gsize          count,
                          GCancellable  *cancellable,
                          GError       **error)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (stream)->priv;

  if (!priv->stream) {
    msg_input_stream_ensure_msg (stream);

retry:
    priv->stream = soup_session_send (msg_service_get_session (priv->service), priv->msg, cancellable, error);
    if (msg_service_handle_rate_limiting (priv->msg))
      goto retry;
  }

  return g_input_stream_read (priv->stream, buffer, count, cancellable, error);
}

static void
msg_input_stream_read_async (GInputStream        *stream,
                             void                *buffer,
                             gsize                count,
                             int                  io_priority,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (stream)->priv;
  GTask *task;

  task = g_task_new (stream, cancellable, callback, user_data);
  g_task_set_priority (task, io_priority);

  if (!priv->stream) {
    ReadAfterSendData *rasd;

    rasd = g_new (ReadAfterSendData, 1);
    rasd->buffer = buffer;
    rasd->count = count;
    g_task_set_task_data (task, rasd, g_free);

    msg_input_stream_ensure_msg (stream);
    soup_session_send_async (msg_service_get_session (priv->service), priv->msg, G_PRIORITY_DEFAULT,
                             cancellable, read_send_callback, task);
    return;
  }

  g_input_stream_read_async (priv->stream, buffer, count, io_priority,
                             cancellable, read_callback, task);
}

static gssize
msg_input_stream_read_finish (GInputStream  *stream,
                              GAsyncResult  *result,
                              GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, stream), -1);

  return g_task_propagate_int (G_TASK (result), error);
}

static void
close_callback (GObject      *object,
                GAsyncResult *result,
                gpointer      user_data)
{
  GTask *task = user_data;
  GError *error = NULL;

  if (g_input_stream_close_finish (G_INPUT_STREAM (object), result, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error);
  g_object_unref (task);
}

static void
msg_input_stream_close_async (GInputStream        *stream,
                              int                  io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (stream)->priv;
  GTask *task;

  task = g_task_new (stream, cancellable, callback, user_data);
  g_task_set_priority (task, io_priority);

  if (priv->stream == NULL){
    g_task_return_boolean (task, TRUE);
    return;
  }

  g_input_stream_close_async (priv->stream, io_priority,
                              cancellable, close_callback, task);
}

static gboolean
msg_input_stream_close_finish (GInputStream  *stream,
                               GAsyncResult  *result,
                               GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, stream), -1);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static goffset
msg_input_stream_tell (GSeekable *seekable)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (seekable)->priv;

  return priv->offset;
}

static gboolean
msg_input_stream_can_seek (__attribute__ ((unused)) GSeekable *seekable)
{
  return TRUE;
}

static gboolean
msg_input_stream_seek (GSeekable                             *seekable,
                       goffset                                offset,
                       GSeekType                              type,
                       __attribute__ ((unused)) GCancellable *cancellable,
                       GError                               **error)
{
  GInputStream *stream = G_INPUT_STREAM (seekable);
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (seekable)->priv;

  if (type == G_SEEK_END && priv->msg){
    goffset content_length = soup_message_headers_get_content_length (soup_message_get_response_headers (priv->msg));

    if (content_length){
      type = G_SEEK_SET;
      offset = priv->request_offset + content_length + offset;
    }
  }

  if (type == G_SEEK_END){
    /* We could send "bytes=-offset", but since we don't know the
     * Content-Length, we wouldn't be able to answer a tell()
     * properly after that. We could maybe find the Content-Length
     * by doing a HEAD... but that would require blocking.
     */
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         "G_SEEK_END not supported");
    return FALSE;
  }

  if (!g_input_stream_set_pending (stream, error))
    return FALSE;

  if (priv->stream){
    if (!g_input_stream_close (priv->stream, NULL, error))
      return FALSE;
    g_clear_object (&priv->stream);
  }

  g_clear_pointer (&priv->range, g_free);

  switch (type){
    case G_SEEK_CUR:
      offset += priv->offset;
    /* fall through */

    case G_SEEK_SET:
      priv->range = g_strdup_printf ("bytes=%"G_GUINT64_FORMAT "-", (guint64)offset);
      priv->request_offset = offset;
      priv->offset = offset;
      break;

    default:
      g_return_val_if_reached (FALSE);
  }

  g_input_stream_clear_pending (stream);
  return TRUE;
}

static gboolean
msg_input_stream_can_truncate (__attribute__ ((unused)) GSeekable *seekable)
{
  return FALSE;
}

static gboolean
msg_input_stream_truncate (__attribute__ ((unused)) GSeekable    *seekable,
                           __attribute__ ((unused)) goffset       offset,
                           __attribute__ ((unused)) GCancellable *cancellable,
                           GError                               **error)
{
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "Truncate not allowed on input stream");
  return FALSE;
}

/**
 * msg_input_stream_get_message:
 * @stream: a #GInputStream
 *
 * Gets corresponding SoupMessage
 *
 * Returns: (transfer full): a #SoupMessage
 */
SoupMessage *
msg_input_stream_get_message (GInputStream *stream)
{
  MsgInputStreamPrivate *priv = MSG_INPUT_STREAM (stream)->priv;

  msg_input_stream_ensure_msg (stream);
  return g_object_ref (priv->msg);
}

static void
msg_input_stream_class_init (MsgInputStreamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *stream_class = G_INPUT_STREAM_CLASS (klass);

  gobject_class->finalize = msg_input_stream_finalize;

  stream_class->read_fn = msg_input_stream_read_fn;
  stream_class->read_async = msg_input_stream_read_async;
  stream_class->read_finish = msg_input_stream_read_finish;
  stream_class->close_async = msg_input_stream_close_async;
  stream_class->close_finish = msg_input_stream_close_finish;
}

static void
msg_input_stream_seekable_iface_init (GSeekableIface *seekable_iface)
{
  seekable_iface->tell = msg_input_stream_tell;
  seekable_iface->can_seek = msg_input_stream_can_seek;
  seekable_iface->seek = msg_input_stream_seek;
  seekable_iface->can_truncate = msg_input_stream_can_truncate;
  seekable_iface->truncate_fn = msg_input_stream_truncate;
}
