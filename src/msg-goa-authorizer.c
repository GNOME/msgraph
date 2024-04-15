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

#include "config.h"

#include <glib.h>
#include <libsoup/soup.h>

#include "msg-authorizer.h"
#include "msg-goa-authorizer.h"

/**
 * SECTION:msg-goa-authorizer
 * @title: MsgGoaAuthorizer
 * @short_description: A GNOME Online Accounts authorizer object for MS Graph.
 * @include: msg/msg.h
 *
 * #MsgGoaAuthorizer provides an implementation of the #MsgAuthorizer
 * interface using GNOME Online Accounts.
 */

struct _MsgGoaAuthorizerPrivate {
  GMutex mutex;
  GoaObject *goa_object;
  char *access_token;

  SoupSession *session;
};

enum {
  PROP_0,
  PROP_GOA_OBJECT
};

static void msg_authorizer_interface_init (MsgAuthorizerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MsgGoaAuthorizer, msg_goa_authorizer, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (MsgGoaAuthorizer)
                         G_IMPLEMENT_INTERFACE (MSG_TYPE_AUTHORIZER,
                                                msg_authorizer_interface_init));

static void
msg_goa_authorizer_process_message (MsgAuthorizer *iface,
                                    SoupMessage   *message)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (iface);
  MsgGoaAuthorizerPrivate *priv = self->priv;
  g_autofree char *auth_value = NULL;

  g_mutex_lock (&priv->mutex);

  if (priv->access_token == NULL)
    goto out;

  auth_value = g_strconcat ("Bearer ", priv->access_token, NULL);
  soup_message_headers_append (soup_message_get_request_headers (message), "Authorization", auth_value);

out:
  g_mutex_unlock (&priv->mutex);
}

static gboolean
msg_goa_authorizer_refresh_authorization (MsgAuthorizer  *iface,
                                          GCancellable   *cancellable,
                                          GError        **error)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (iface);
  MsgGoaAuthorizerPrivate *priv = self->priv;
  GoaAccount *account;
  GoaOAuth2Based *oauth2_based;
  gboolean ret_val = FALSE;

  g_mutex_lock (&priv->mutex);

  g_clear_pointer (&priv->access_token, g_free);

  account = goa_object_peek_account (priv->goa_object);
  oauth2_based = goa_object_peek_oauth2_based (priv->goa_object);

  if (!goa_account_call_ensure_credentials_sync (account, NULL, cancellable, error))
    goto out;

  if (!goa_oauth2_based_call_get_access_token_sync (oauth2_based, &priv->access_token, NULL, cancellable, error))
    goto out;

  ret_val = TRUE;

out:

  g_mutex_unlock (&priv->mutex);
  return ret_val;
}


static void
msg_goa_authorizer_set_goa_object (MsgGoaAuthorizer *self,
                                   GoaObject        *goa_object)
{
  GoaAccount *account;
  GoaOAuth2Based *oauth2_based;

  g_return_if_fail (GOA_IS_OBJECT (goa_object));

  oauth2_based = goa_object_peek_oauth2_based (goa_object);
  g_return_if_fail (oauth2_based != NULL && GOA_IS_OAUTH2_BASED (oauth2_based));

  account = goa_object_peek_account (goa_object);
  g_return_if_fail (account != NULL && GOA_IS_ACCOUNT (account));

  g_object_ref (goa_object);
  self->priv->goa_object = goa_object;
  g_object_notify (G_OBJECT (self), "goa-object");
}

static void
msg_goa_authorizer_dispose (GObject *object)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (object);

  g_clear_object (&self->priv->goa_object);

  G_OBJECT_CLASS (msg_goa_authorizer_parent_class)->dispose (object);
}

static void
msg_goa_authorizer_finalize (GObject *object)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (object);
  MsgGoaAuthorizerPrivate *priv = self->priv;

  g_mutex_clear (&priv->mutex);
  g_free (priv->access_token);

  G_OBJECT_CLASS (msg_goa_authorizer_parent_class)->finalize (object);
}

static void
msg_goa_authorizer_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (object);

  switch (prop_id) {
    case PROP_GOA_OBJECT:
      g_value_set_object (value, self->priv->goa_object);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
msg_goa_authorizer_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MsgGoaAuthorizer *self = MSG_GOA_AUTHORIZER (object);

  switch (prop_id) {
    case PROP_GOA_OBJECT:
      msg_goa_authorizer_set_goa_object (self, GOA_OBJECT (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
msg_goa_authorizer_init (MsgGoaAuthorizer *self)
{
  self->priv = msg_goa_authorizer_get_instance_private (self);
  g_mutex_init (&self->priv->mutex);

  self->priv->session = soup_session_new ();
}

static void
msg_goa_authorizer_class_init (MsgGoaAuthorizerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = msg_goa_authorizer_dispose;
  object_class->finalize = msg_goa_authorizer_finalize;
  object_class->get_property = msg_goa_authorizer_get_property;
  object_class->set_property = msg_goa_authorizer_set_property;

  g_object_class_install_property (object_class,
                                   PROP_GOA_OBJECT,
                                   g_param_spec_object ("goa-object",
                                                        "GoaObject",
                                                        "The GOA account to authenticate.",
                                                        GOA_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
msg_authorizer_interface_init (MsgAuthorizerInterface *iface)
{
  iface->process_request = msg_goa_authorizer_process_message;
  iface->refresh_authorization = msg_goa_authorizer_refresh_authorization;
}

/**
 * msg_goa_authorizer_new:
 * @goa_object: A #GoaObject representing a MS Graph account.
 *
 * Creates a new #MsgGoaAuthorizer using @goa_object.
 *
 * Returns: (transfer full): A new #MsgGoaAuthorizer. Free the returned
 * object with g_object_unref().
 */
MsgGoaAuthorizer *
msg_goa_authorizer_new (GoaObject *goa_object)
{
  return g_object_new (MSG_TYPE_GOA_AUTHORIZER, "goa-object", goa_object, NULL);
}

/**
 * msg_goa_authorizer_get_goa_object:
 * @self: A #MsgGoaAuthorizer.
 *
 * Gets the GOA account used by @self for authorization.
 *
 * Returns: (transfer none): A #GoaObject. The returned object is
 * owned by #MsgGoaAuthorizer and should not be modified or freed.
 */
GoaObject *
msg_goa_authorizer_get_goa_object (MsgGoaAuthorizer *self)
{
  return self->priv->goa_object;
}
