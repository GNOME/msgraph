#include <glib.h>

#include "msg-authorizer.h"
#include "msg-dummy-authorizer.h"

#include "src/msg-service.h"

struct _MsgDummyAuthorizerPrivate {
  GMutex mutex;
};

static void msg_authorizer_interface_init (MsgAuthorizerInterface *iface);


G_DEFINE_TYPE_WITH_CODE (MsgDummyAuthorizer, msg_dummy_authorizer, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (MsgDummyAuthorizer)
                         G_IMPLEMENT_INTERFACE (MSG_TYPE_AUTHORIZER,
                                                msg_authorizer_interface_init));

static void
msg_dummy_authorizer_init (__attribute__ ((unused)) MsgDummyAuthorizer *self)
{
}

static void
msg_dummy_authorizer_class_init (__attribute__ ((unused)) MsgDummyAuthorizerClass *class)
{
}

static void
msg_dummy_authorizer_process_request (__attribute__ ((unused)) MsgAuthorizer *self,
                                      SoupMessage   *message)
{
  g_assert (message != NULL);
  g_assert (SOUP_IS_MESSAGE (message));

  soup_message_headers_append (soup_message_get_request_headers (message), "process_request_null", "1");
}

static gboolean
msg_dummy_authorizer_refresh_authorization (MsgAuthorizer  *self,
                                            GCancellable   *cancellable,
                                            GError        **error)
{
  /* Check the inputs */
  g_assert (MSG_IS_AUTHORIZER (self));
  g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_assert (error == NULL || *error == NULL);

  /* Increment the counter on the authorizer so we know if this function's been called more than once */
  g_object_set_data (G_OBJECT (self), "counter", GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (self), "counter")) + 1));

  /* If we're instructed to set an error, do so (with an arbitrary error code) */
  if (g_object_get_data (G_OBJECT (self), "error") != NULL) {
    /* g_set_error_literal (error, MSG_SERVICE_ERROR, MSG_SERVICE_ERROR_PROTOCOL_ERROR, "Error message"); */
    return FALSE;
  }

  return TRUE;
}

static void
msg_authorizer_interface_init (MsgAuthorizerInterface *iface)
{
  iface->process_request = msg_dummy_authorizer_process_request;
  iface->refresh_authorization = msg_dummy_authorizer_refresh_authorization;
}

/*
 * msg_dummy_authorizer_new:
 * #MsgDummyAuthorizer will be used with
 *
 * Creates a new #MsgDummyAuthorizer.
 *
 * Return value: (transfer full): a new #MsgDummyAuthorizer; unref with
 * g_object_unref()
 */
MsgDummyAuthorizer *
msg_dummy_authorizer_new (void)
{
  return g_object_new (MSG_TYPE_DUMMY_AUTHORIZER, NULL);
}

