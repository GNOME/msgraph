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

#define GOA_API_IS_SUBJECT_TO_CHANGE

#include <glib-object.h>
#include <goa/goa.h>

G_BEGIN_DECLS

#define MSG_TYPE_GOA_AUTHORIZER (msg_goa_authorizer_get_type ())

#define MSG_GOA_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MSG_TYPE_GOA_AUTHORIZER, MsgGoaAuthorizer))
#define MSG_GOA_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MSG_TYPE_GOA_AUTHORIZER, MsgGoaAuthorizerClass))
#define MSG_IS_GOA_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MSG_TYPE_GOA_AUTHORIZER))
#define MSG_IS_GOA_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MSG_TYPE_GOA_AUTHORIZER))
#define MSG_GOA_AUTHORIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MSG_TYPE_GOA_AUTHORIZER, MsgGoaAuthorizerClass))

typedef struct _MsgGoaAuthorizer        MsgGoaAuthorizer;
typedef struct _MsgGoaAuthorizerClass   MsgGoaAuthorizerClass;
typedef struct _MsgGoaAuthorizerPrivate MsgGoaAuthorizerPrivate;

/**
 * MsgGoaAuthorizer:
 *
 * The #MsgGoaAuthorizer structure contains only private data and
 * should only be accessed using the provided API.
 */
struct _MsgGoaAuthorizer
{
  GObject parent_instance;
  MsgGoaAuthorizerPrivate *priv;
};

/**
 * MsgGoaAuthorizerClass:
 * @parent_class: The parent class.
 *
 * Class structure for #MsgGoaAuthorizer.
 */
struct _MsgGoaAuthorizerClass
{
  GObjectClass parent_class;
};

GType                msg_goa_authorizer_get_type           (void) G_GNUC_CONST;

MsgGoaAuthorizer    *msg_goa_authorizer_new                (GoaObject *goa_object);

GoaObject           *msg_goa_authorizer_get_goa_object     (MsgGoaAuthorizer *self);

G_END_DECLS

