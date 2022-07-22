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

#pragma once

#include <gio/gio.h>
#include <libsoup/soup.h>

#include "msg-service.h"

G_BEGIN_DECLS

#define MSG_TYPE_INPUT_STREAM         (msg_input_stream_get_type ())
#define MSG_INPUT_STREAM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MSG_TYPE_INPUT_STREAM, MsgInputStream))
#define MSG_INPUT_STREAM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MSG_TYPE_INPUT_STREAM, MsgInputStreamClass))
#define MSG_IS_INPUT_STREAM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MSG_TYPE_INPUT_STREAM))
#define MSG_IS_INPUT_STREAM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MSG_TYPE_INPUT_STREAM))
#define MSG_INPUT_STREAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MSG_TYPE_INPUT_STREAM, MsgInputStreamClass))

typedef struct MsgInputStream         MsgInputStream;
typedef struct MsgInputStreamPrivate  MsgInputStreamPrivate;
typedef struct MsgInputStreamClass    MsgInputStreamClass;

struct MsgInputStream
{
  GInputStream parent;

  MsgInputStreamPrivate *priv;
};

struct MsgInputStreamClass
{
  GInputStreamClass parent_class;

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

GType msg_input_stream_get_type (void) G_GNUC_CONST;

GInputStream *
msg_input_stream_new (MsgService *service,
                      char        *uri);

void          msg_input_stream_send_async  (GInputStream        *stream,
                                            int                  io_priority,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data);
gboolean      msg_input_stream_send_finish (GInputStream  *stream,
                                            GAsyncResult  *result,
                                            GError       **error);

SoupMessage  *msg_input_stream_get_message (GInputStream         *stream);

G_END_DECLS

