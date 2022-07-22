#pragma once

#include <glib-object.h>
#include "msg-authorizer.h"

G_BEGIN_DECLS

#define MSG_TYPE_OAUTH2_AUTHORIZER (msg_oauth2_authorizer_get_type ())

#define MSG_OAUTH2_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MSG_TYPE_OAUTH2_AUTHORIZER, MsgOAuth2Authorizer))
#define MSG_OAUTH2_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MSG_TYPE_OAUTH2_AUTHORIZER, MsgOAuth2AuthorizerClass))
#define MSG_IS_OAUTH2_AUTHORIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MSG_TYPE_OAUTH2_AUTHORIZER))
#define MSG_IS_OAUTH2_AUTHORIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MSG_TYPE_OAUTH2_AUTHORIZER))
#define MSG_OAUTH2_AUTHORIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MSG_TYPE_OAUTH2_AUTHORIZER, MsgOAuth2AuthorizerClass))

typedef struct _MsgOAuth2Authorizer        MsgOAuth2Authorizer;
typedef struct _MsgOAuth2AuthorizerClass   MsgOAuth2AuthorizerClass;
typedef struct _MsgOAuth2AuthorizerPrivate MsgOAuth2AuthorizerPrivate;

struct _MsgOAuth2Authorizer
{
  GObject parent_instance;
  MsgOAuth2AuthorizerPrivate *priv;
};

struct _MsgOAuth2AuthorizerClass
{
  GObjectClass parent_class;
};

// G_DECLARE_FINAL_TYPE (MsgOAuth2Authorizer, msg_oauth2_authorizer, MSG, OAUTH2_AUTHORIZER, GObject);

GType msg_oauth2_authorizer_get_type (void) G_GNUC_CONST;

MsgOAuth2Authorizer *
msg_oauth2_authorizer_new (const char *client_id,
                           const char *redirect_uri);

char *
msg_oauth2_authorizer_build_authentication_uri (MsgOAuth2Authorizer *self);

gboolean
msg_oauth2_authorizer_request_authorization (MsgOAuth2Authorizer  *self,
                                             const char           *authorization_code,
                                             GCancellable         *cancellable,
                                             GError              **error);

void
msg_oauth2_authorizer_test_save_credentials (MsgAuthorizer *self);

gboolean
msg_oauth2_authorizer_test_load_credentials (MsgAuthorizer *self);

G_END_DECLS
