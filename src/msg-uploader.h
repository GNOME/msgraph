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

#define MSG_TYPE_UPLOADER		(msg_uploader_get_type ())

G_DECLARE_FINAL_TYPE (MsgUploader, msg_uploader, MSG, UPLOADER, GObject)

typedef struct _MsgUploaderPrivate  MsgUploaderPrivate;

struct _MsgUploader {
  GObject parent;
  MsgUploaderPrivate *priv;
};

struct _MsgUploaderClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

MsgUploader *
msg_uploader_new (MsgService  *service,
                  const gchar *method,
                  const gchar *upload_uri,
                  const gchar *slug,
                  const gchar *content_type);

G_END_DECLS

