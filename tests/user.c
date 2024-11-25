#include "src/msg-authorizer.h"
#include "src/user/msg-user-contact-folder.h"
#include "src/user/msg-user-service.h"
#include "src/user/msg-user.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

static MsgService *service = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
  MsgUser *user;
} TempUserData;

static void
test_attributes (void)
{
  g_autoptr (MsgUser) user = NULL;

  user = msg_user_new ();

  g_assert_cmpstr (msg_user_get_display_name (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_mobile_phone (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_office_location (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_surname (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_given_name (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_company_name (user), ==, NULL);
  g_assert_cmpstr (msg_user_get_department (user), ==, NULL);
}

static void
test_contact_attributes (void)
{
  g_autoptr (MsgUserContactFolder) folder = NULL;

  folder = msg_user_contact_folder_new ();

  g_assert_cmpstr (msg_user_contact_folder_get_display_name (folder), ==, NULL);
  g_assert_cmpstr (msg_user_contact_folder_get_id (folder), ==, NULL);
}

static void
test_get_user (void)
{
  g_autoptr (MsgUser) user = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-user");

  user = msg_user_service_get_user (MSG_USER_SERVICE (service), NULL, NULL, &error);
  g_assert (user);

  g_assert_nonnull (msg_user_get_mail (user));
  g_clear_object (&user);

  user = msg_user_service_get_user (MSG_USER_SERVICE (service), "unknown", NULL, &error);
  g_assert (!user);

  uhm_server_end_trace (mock_server);
}

static void
test_get_photo (void)
{
  g_autoptr (MsgUser) user = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GBytes) photo = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-photo");

  user = msg_user_service_get_user (MSG_USER_SERVICE (service), NULL, NULL, &error);
  g_assert (!error);
  g_assert (user);

  g_assert_nonnull (msg_user_get_mail (user));

  photo = msg_user_service_get_photo (MSG_USER_SERVICE (service), msg_user_get_mail (user), NULL, &error);
  g_assert (!error);
  g_clear_object (&user);
  g_assert (photo);

  g_clear_pointer (&photo, g_bytes_unref);

  photo = msg_user_service_get_photo (MSG_USER_SERVICE (service), "unknown", NULL, &error);
  g_assert (error);

  uhm_server_end_trace (mock_server);
}

static void
test_get_contacts (void)
{
  g_autolist (MsgUser) contacts = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-contacts");

  contacts = msg_user_service_get_contacts (MSG_USER_SERVICE (service), NULL, &error);
  g_assert (!error);
  g_assert (contacts);

  uhm_server_end_trace (mock_server);
}

static void
test_get_contact_folders (void)
{
  g_autolist (MsgUserContactFolder) folders = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "get-contact-folders");

  folders = msg_user_service_get_contact_folders (MSG_USER_SERVICE (service), NULL, &error);
  g_assert (!error);
  g_assert (!folders);

  uhm_server_end_trace (mock_server);
}

static void
test_get_find_users (void)
{
  g_autolist (MsgUser) users = NULL;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "find-users");

  /* Expect error as this one is for business accounts only and therefore test account will fail */
  users = msg_user_service_find_users (MSG_USER_SERVICE (service), "mustermann", NULL, &error);
  g_assert (error);
  g_assert (!users);

  uhm_server_end_trace (mock_server);
}

int
main (int    argc,
      char **argv)
{
  g_autoptr (GFile) trace_directory = NULL;
  g_autofree char *path = NULL;
  MsgAuthorizer *authorizer = NULL;
  g_autoptr (GError) error = NULL;
  int retval;

  msg_test_init (argc, argv);

  mock_server = msg_test_get_mock_server ();
  path = g_test_build_filename (G_TEST_DIST, "traces/user", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_user_service_new (authorizer));
  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add_func ("/user/attributes", test_attributes);
  g_test_add_func ("/user/contact_attributes", test_contact_attributes);
  g_test_add_func ("/user/get/user", test_get_user);
  g_test_add_func ("/user/get/photo", test_get_photo);
  g_test_add_func ("/user/get/contacts", test_get_contacts);
  g_test_add_func ("/user/get/contact_folders", test_get_contact_folders);
  g_test_add_func ("/user/get/find_users", test_get_find_users);

  retval = g_test_run ();

  return retval;
}
