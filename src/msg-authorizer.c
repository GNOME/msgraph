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

#include "msg-authorizer.h"

G_DEFINE_INTERFACE (MsgAuthorizer, msg_authorizer, G_TYPE_OBJECT);

static void
msg_authorizer_refresh_authorization_thread_func (GTask                            *task,
                                                  gpointer                          object,
                                                  __attribute__ ((unused)) gpointer data,
                                                  GCancellable                     *cancellable)
{
  g_autoptr (GError) error = NULL;

  msg_authorizer_refresh_authorization (MSG_AUTHORIZER (object), cancellable, &error);
  if (error)
    g_task_return_error (task, error);
}

static void
msg_authorizer_default_init (__attribute__ ((unused)) MsgAuthorizerInterface *iface)
{
}

/**
 * msg_authorizer_process_request:
 * @iface: A #MsgAuthorizer.
 * @message: A #SoupMessage.
 *
 * Adds the necessary authorization to @message. The type of @message
 * can be DELETE, GET and POST.
 *
 * This method modifies @message in place and is thread safe.
 */
void
msg_authorizer_process_request (MsgAuthorizer *iface,
                                SoupMessage   *message)
{
  g_return_if_fail (MSG_IS_AUTHORIZER (iface));
  MSG_AUTHORIZER_GET_INTERFACE (iface)->process_request (iface, message);
}

/**
 * msg_authorizer_refresh_authorization:
 * @iface: A #MsgAuthorizer.
 * @cancellable: (allow-none): An optional #GCancellable object, or
 *   %NULL.
 * @error: (allow-none): An optional #GError, or %NULL.
 *
 * Synchronously forces @iface to refresh any authorization tokens
 * held by it. See msg_authorizer_refresh_authorization_async() for the
 * asynchronous version of this call.
 *
 * This method is thread safe.
 *
 * Returns: %TRUE if the authorizer now has a valid token.
 */
gboolean
msg_authorizer_refresh_authorization (MsgAuthorizer  *iface,
                                      GCancellable   *cancellable,
                                      GError        **error)
{
  g_return_val_if_fail (MSG_IS_AUTHORIZER (iface), FALSE);
  return MSG_AUTHORIZER_GET_INTERFACE (iface)->refresh_authorization (iface, cancellable, error);
}

/**
 * msg_authorizer_refresh_authorization_async:
 * @iface: A #MsgAuthorizer.
 * @cancellable: (allow-none): An optional #GCancellable object, or
 *   %NULL.
 * @callback: (scope async): A #GAsyncReadyCallback to call when the
 *   request is satisfied.
 * @user_data: (closure): The data to pass to @callback.
 *
 * Asynchronously forces @iface to refresh any authorization tokens
 * held by it. See msg_authorizer_refresh_authorization() for the
 * synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You can
 * then call msg_authorizer_refresh_authorization_finish() to get the
 * result of the operation.
 *
 * This method is thread safe.
 */
void
msg_authorizer_refresh_authorization_async (MsgAuthorizer       *iface,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
  GTask *task;

  g_return_if_fail (MSG_IS_AUTHORIZER (iface));

  task = g_task_new (iface, cancellable, callback, user_data);

  g_task_run_in_thread (task, msg_authorizer_refresh_authorization_thread_func);

  g_object_unref (task);
}

/**
 * msg_authorizer_refresh_authorization_finish:
 * @iface: A #MsgAuthorizer.
 * @res: A #GAsyncResult.
 * @error: (allow-none): An optional #GError, or %NULL.
 *
 * Finishes an asynchronous operation started with
 * msg_authorizer_refresh_authorization_async().
 *
 * Returns: %TRUE if the authorizer now has a valid token.
 */
gboolean
msg_authorizer_refresh_authorization_finish (MsgAuthorizer  *iface,
                                             GAsyncResult   *res,
                                             GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (res, iface), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}
