#include "src/msg-authorizer.h"
#include "src/msg-service.h"
#include "src/drive/msg-drive-service.h"

#include "common.h"

static void
test_response (void)
{
  g_autofree char *tmp = NULL;
  g_autoptr (JsonParser) parser = NULL;

  tmp = g_strdup ("[]");
  parser = msg_service_parse_response (g_bytes_new (tmp, strlen (tmp)), NULL, NULL);
  g_clear_pointer (&tmp, g_free);
  g_assert (!parser);
  g_clear_object (&parser);

  tmp = g_strdup ("{\"error\": {\"message\": \"test\"}}");
  parser = msg_service_parse_response (g_bytes_new (tmp, strlen (tmp)), NULL, NULL);
  g_clear_pointer (&tmp, g_free);
  g_assert (!parser);

  tmp = g_strdup ("{}");
  parser = msg_service_parse_response (g_bytes_new (tmp, strlen (tmp)), NULL, NULL);
  g_clear_pointer (&tmp, g_free);
  g_assert (parser);
  g_clear_object (&parser);
}

static void
test_service (void)
{
  g_autoptr (MsgService) service = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (SoupMessage) message = NULL;
  gboolean ret;

  service = MSG_SERVICE (msg_drive_service_new (NULL));
  ret = msg_service_refresh_authorization (service, NULL, &error);
  g_assert (ret == FALSE);
  g_assert (error);

  g_unsetenv ("MSG_HTTPS_PORT");
  g_assert (msg_service_get_https_port () == 443);

  g_setenv ("MSG_HTTPS_PORT", "nonsense", TRUE);
  g_assert (msg_service_get_https_port () == 443);

  message = msg_service_build_message (service, "GET", "http://graph.microsoft.com", NULL, FALSE);
  g_assert (message == NULL);

  message = msg_service_build_message (service, "GET", "https://graph.microsoft.com", "DUMMY", FALSE);
  g_assert (message);
  g_clear_object (&message);

  message = msg_service_build_message (service, "GET", "https://graph.microsoft.com", "DUMMY", TRUE);
  g_assert (message);
  g_clear_object (&message);

  g_test_expect_message ("GLib-GObject", G_LOG_LEVEL_CRITICAL, "*g_object_get_is_valid_property*MsgDriveService*");
  GValue val;
  g_object_get_property (G_OBJECT (service), "invlid", &val);
  /* g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT); */
  /* g_test_trap_assert_failed (); */
  /* g_test_trap_assert_stderr ("*CRITICAL*g_object_get_is_valid_property*MsgDriveService*"); */
}

int
main (int    argc,
      char **argv)
{
  int retval;

  msg_test_init (argc, argv);

  g_test_add_func ("/service/response", test_response);
  g_test_add_func ("/service/service", test_service);

  retval = g_test_run ();

  return retval;
}
