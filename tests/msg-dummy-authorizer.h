/* Copyright 2022 Jan-Michael Brummer
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
#include <glib.h>
#include <glib-object.h>

#include "msg-authorizer.h"

G_BEGIN_DECLS

#define MSG_TYPE_DUMMY_AUTHORIZER (msg_dummy_authorizer_get_type ())

#define MSG_DUMMY_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MSG_TYPE_DUMMY_AUTHORIZER, MsgDummyAuthorizer))
#define MSG_DUMMY_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MSG_TYPE_DUMMY_AUTHORIZER, MsgDummyAuthorizerClass))
#define MSG_IS_DUMMY_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MSG_TYPE_DUMMY_AUTHORIZER))
#define MSG_IS_DUMMY_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MSG_TYPE_DUMMY_AUTHORIZER))
#define MSG_DUMMY_AUTHORIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MSG_TYPE_DUMMY_AUTHORIZER, MsgDummyAuthorizerClass))

typedef struct _MsgDummyAuthorizer          MsgDummyAuthorizer;
typedef struct _MsgDummyAuthorizerClass MsgDummyAuthorizerClass;
typedef struct _MsgDummyAuthorizerPrivate MsgDummyAuthorizerPrivate;

struct _MsgDummyAuthorizer
{
  GObject parent_instance;
  MsgDummyAuthorizerPrivate *priv;
};

struct _MsgDummyAuthorizerClass
{
  GObjectClass parent_class;
};

MsgDummyAuthorizer *
msg_dummy_authorizer_new (void);

G_END_DECLS
