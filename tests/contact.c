#include "src/msg-authorizer.h"
#include "src/contact/msg-contact-service.h"
#include "src/contact/msg-contact.h"
#include "src/msg-service.h"

#include "common.h"
#include "msg-dummy-authorizer.h"

static MsgService *service = NULL;
static UhmServer *mock_server = NULL;

typedef struct {
    MsgContact *contact;
} TempContactData;

static void
setup_temp_contact (TempContactData *data,
                    const void      *user_data)
{
  g_autoptr (MsgContact) contact = NULL;
  g_autoptr (MsgContact) new_contact = NULL;
  MsgService *service = (MsgService *) user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "setup-temp-contact");

  contact = MSG_CONTACT (msg_contact_new ());
  msg_contact_set_given_name (contact, "John");
  msg_contact_set_surname (contact, "Doe");

  data->contact = msg_contact_service_create (MSG_CONTACT_SERVICE (service), contact, NULL, &error);
  g_assert_no_error (error);

  uhm_server_end_trace (mock_server);
}

static void
teardown_temp_contact (TempContactData *data,
                       const void      *user_data)
{
  MsgService *service = (MsgService *)user_data;
  g_autoptr (GError) error = NULL;

  msg_test_mock_server_start_trace (mock_server, "teardown-temp-contact");

  if (data->contact) {
    msg_contact_service_delete (MSG_CONTACT_SERVICE (service), data->contact, NULL, &error);
    g_clear_object (&data->contact);
  }

  uhm_server_end_trace (mock_server);
}

static void
test_get_contacts (__attribute__ ((unused)) TempContactData *data,
                   const void                               *user_data)
{
  MsgService *service = (MsgService *)user_data;
  MsgContact *contact = NULL;
  g_autoptr (GError) error = NULL;
  GList *list;

  msg_test_mock_server_start_trace (mock_server, "get-contacts");

  /* List should only contain temporary contact */
  list = msg_contact_service_get_contacts (MSG_CONTACT_SERVICE (service), NULL, &error);
  g_assert_no_error (error);

  /* g_assert (g_list_length (list) == 1); */

  contact = MSG_CONTACT (list->data);
  g_assert_cmpstr (msg_contact_get_name (contact), ==, "John Doe");
  g_assert_cmpstr (msg_contact_get_given_name (contact), ==, "John");
  g_assert_cmpstr (msg_contact_get_surname (contact), ==, "Doe");
  g_assert_cmpstr (msg_contact_get_id (contact), ==, msg_contact_get_id (data->contact));

  uhm_server_end_trace (mock_server);
}

static void
mock_server_notify_resolver_cb (GObject                             *object,
                                __attribute__ ((unused)) GParamSpec *pspec,
                                __attribute__ ((unused)) gpointer    user_data)
{
  UhmServer *server;
  UhmResolver *resolver;

  server = UHM_SERVER (object);

  resolver = uhm_server_get_resolver (server);
  if (resolver != NULL) {
    const char *ip_address = uhm_server_get_address (server);

    g_print ("Add resolver to %s\n", ip_address);
    uhm_resolver_add_A (resolver, "graph.microsoft.com", ip_address);
    uhm_resolver_add_A (resolver, "login.microsoftonline.com", ip_address);
  }
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
  g_signal_connect (G_OBJECT (mock_server), "notify::resolver", (GCallback) mock_server_notify_resolver_cb, NULL);
  path = g_test_build_filename (G_TEST_DIST, "traces/contact", NULL);
  trace_directory = g_file_new_for_path (path);
  uhm_server_set_trace_directory (mock_server, trace_directory);

  authorizer = msg_test_create_global_authorizer ();
  service = MSG_SERVICE (msg_contact_service_new (authorizer));
  if (!uhm_server_get_enable_online (mock_server))
    soup_session_set_proxy_resolver (msg_service_get_session (service), G_PROXY_RESOLVER (uhm_server_get_resolver (mock_server)));

  g_test_add ("/contact/get/contacts",
                   TempContactData,
                   service,
                   setup_temp_contact,
                   test_get_contacts,
                   teardown_temp_contact);

  retval = g_test_run ();

  return retval;
}
