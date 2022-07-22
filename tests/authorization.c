#include "src/msg-authorizer.h"
#include "msg-dummy-authorizer.h"
#include "common.h"

typedef struct {
  MsgAuthorizer *authorizer;
} AuthorizerData;

static void
set_up_normal_authorizer_data (AuthorizerData *data,
                               __attribute__ ((unused)) gconstpointer   user_data)
{
  data->authorizer = MSG_AUTHORIZER (msg_dummy_authorizer_new ());
}

static void
tear_down_authorizer_data (AuthorizerData *data,
                           __attribute__ ((unused)) gconstpointer   user_data)
{
  g_object_unref (data->authorizer);
}

/* Test that calling refresh_authorization() on an authorizer which implements it returns TRUE without error, and only calls the implementation
 * once */
static void
test_authorizer_refresh_authorization (AuthorizerData *data,
                                       __attribute__ ((unused)) gconstpointer   user_data)
{
  gboolean success;
  GError *error = NULL;

  /* Set a counter on the authoriser to check that the interface implementation is only called once */
  g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));

  success = msg_authorizer_refresh_authorization (data->authorizer, NULL, &error);
  g_assert_no_error (error);
  g_assert (success == TRUE);
  g_clear_error (&error);

  g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 1);
}


int
main (int argc, char *argv[])
{
  int retval;

  msg_test_init (argc, argv);

  g_test_add ("/authorizer/refresh-authorization",
              AuthorizerData,
              NULL,
              set_up_normal_authorizer_data,
              test_authorizer_refresh_authorization,
              tear_down_authorizer_data);

  retval = g_test_run ();

  return retval;
}

