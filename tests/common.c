#include <locale.h>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/xmlsave.h>

#include "common.h"
#include "msg-dummy-authorizer.h"
#include "src/msg-service.h"

/* %TRUE if interactive tests should be skipped because we're running automatically (for example) */
static gboolean no_interactive = TRUE;

/* TRUE if tests should be run online and a trace file written for each; FALSE if tests should run offline against existing trace files. */
static gboolean write_traces = FALSE;

/* TRUE if tests should be run online and the server's responses compared to the existing trace file for each; FALSE if tests should run offline without comparison. */
static gboolean compare_traces = FALSE;

static MsgAuthorizer *authorizer = NULL;

static UhmServer *mock_server = NULL;

char *
msg_test_query_user_for_verifier (const gchar *authentication_uri)
{
  char verifier[200];
  g_autofree char *query = NULL;

  /* Wait for the user to retrieve and enter the verifier */
  g_print ("Please navigate to the following URI and grant access: %s\n", authentication_uri);
  g_print ("Enter verifier (EOF to skip test): ");
  if (scanf ("%200s", verifier) != 1) {
          /* Skip the test */
          g_test_message ("Skipping test on user request.");
          return NULL;
  }

  g_uri_split (verifier, G_URI_FLAGS_NONE, NULL, NULL, NULL, NULL, NULL, &query, NULL, NULL);

  g_test_message ("Proceeding with user-provided verifier “%s”.", query + 5);

  return g_strdup (query + 5);
}

/* Output a log message. Note the output is prefixed with ‘# ’ so that it
 * doesn’t interfere with TAP output. */
static void
output_commented_lines (const char *message)
{
  const char *i, *next_newline;

  for (i = message; i != NULL && *i != '\0'; i = next_newline) {
    char *line;

    next_newline = strchr (i, '\n');
    if (next_newline != NULL) {
      line = g_strndup (i, next_newline - i);
      next_newline++;
    } else {
      line = g_strdup (i);
    }

    printf ("# %s\n", line);

    g_free (line);
  }
}


static void
output_log_message (const char *message)
{
  if (strlen (message) > 2 && message[2] == '<') {
    /* As debug string starts with direction indicator and space, t.i. "< ", */
    /* we need access string starting from third character and see if it's   */
    /* looks like xml - t.i. it starts with '<' */
    xmlChar *xml_buff;
    int buffer_size;
    xmlDocPtr xml_doc;
    /* we need to cut to the beginning of XML string */
    message = message + 2;
    /* create xml document and dump it to string buffer */
    xml_doc = xmlParseDoc ((const xmlChar*) message);
    xmlDocDumpFormatMemory (xml_doc, &xml_buff, &buffer_size, 1);
    /* print out structured xml - if it's not xml, it will get error in output */
    output_commented_lines ((gchar*) xml_buff);
    /* free xml structs */
    xmlFree (xml_buff);
    xmlFreeDoc (xml_doc);
  } else {
    output_commented_lines ((gchar*) message);
  }
}

static void
msg_test_debug_handler (__attribute__ ((unused)) const char     *log_domain,
                        __attribute__ ((unused)) GLogLevelFlags  log_level,
                        const char                              *message,
                        __attribute__ ((unused)) gpointer        user_data)
{
  output_log_message (message);

  /* Log to the trace file. */
  if ((*message == '<' || *message == '>' || *message == ' ') && *(message + 1) == ' ') {
    uhm_server_received_message_chunk (mock_server, message, strlen (message), NULL);
  }
}

void
test_authentication (void)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *authentication_uri = NULL;
  g_autofree char *authorisation_code = NULL;

  /* if (msg_service_refresh_authorization (MSG_SERVICE (service), NULL, &error)) */
  /*   return; */

  msg_test_mock_server_start_trace (mock_server, "setup-oauth2-authorizer-data-authenticated");

  /* Get an authentication URI. */
  authentication_uri = msg_oauth2_authorizer_build_authentication_uri (MSG_OAUTH2_AUTHORIZER (msg_test_get_authorizer ()));
  g_assert (authentication_uri != NULL);

  if (uhm_server_get_enable_online (mock_server)) {
    authorisation_code = msg_test_query_user_for_verifier (authentication_uri);
    if (!authorisation_code)
      return;
  } else {
    /* Hard-coded default to match the trace file. */
    authorisation_code = g_strdup ("4/GeYb_3HkYh4vyephp-lbvzQs1GAb.YtXAxmx-uJ0eoiIBeO6P2m9iH6kvkQI");
  }

  g_assert (msg_oauth2_authorizer_request_authorization (MSG_OAUTH2_AUTHORIZER (msg_test_get_authorizer ()), authorisation_code, NULL, NULL) == TRUE);

  msg_oauth2_authorizer_test_save_credentials (msg_test_get_authorizer ());

  uhm_server_end_trace (mock_server);
}

void
msg_test_init (int    argc,
               char **argv)
{
  g_autoptr (GTlsCertificate) cert = NULL;
  g_autoptr (GError) child_error = NULL;
  g_autofree char *cert_path = NULL;
  g_autofree char *key_path = NULL;
  g_autofree gchar *trace_path = NULL;
  int i;

  setlocale (LC_ALL, "");

  /* Parse the custom options */
  for (i = 1; i < argc; i++) {
    if (strcmp ("--no-interactive", argv[i]) == 0 || strcmp ("-ni", argv[i]) == 0) {
      no_interactive = TRUE;
      argv[i] = (char*) "";
    } else if (strcmp ("--interactive", argv[i]) == 0 || strcmp ("-i", argv[i]) == 0) {
      no_interactive = FALSE;
      argv[i] = (char*) "";
    } else if (strcmp ("--write-traces", argv[i]) == 0 || strcmp ("-w", argv[i]) == 0) {
      write_traces = TRUE;
      argv[i] = (char*) "";
    } else if (strcmp ("--compare-traces", argv[i]) == 0 || strcmp ("-c", argv[i]) == 0) {
      compare_traces = TRUE;
      argv[i] = (char*) "";
    } else if (strcmp ("-?", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0 || strcmp ("-h" , argv[i]) == 0) {
      /* We have to override --help in order to document --no-interactive and the trace flags. */
      printf ("Usage:\n"
                "  %s [OPTION...]\n\n"
                "Help Options:\n"
                "  -?, --help                     Show help options\n"
                "Test Options:\n"
                "  -l                             List test cases available in a test executable\n"
                "  -seed=RANDOMSEED               Provide a random seed to reproduce test\n"
                "                                 runs using random numbers\n"
                "  --verbose                      Run tests verbosely\n"
                "  -q, --quiet                    Run tests quietly\n"
                "  -p TESTPATH                    Execute all tests matching TESTPATH\n"
                "  -m {perf|slow|thorough|quick}  Execute tests according modes\n"
                "  --debug-log                    Debug test logging output\n"
                "  -ni, --no-interactive          Only execute tests which don't require user interaction\n"
                "  -i, --interactive              Execute tests including those requiring user interaction\n"
                "  -t, --trace-dir [directory]    Read/Write trace files in the specified directory\n"
                "  -w, --write-traces             Work online and write trace files to --trace-dir\n"
                "  -c, --compare-traces           Work online and compare with existing trace files in --trace-dir\n",
                argv[0]);
      exit (0);
    }
  }

  /* --[write|compare]-traces are mutually exclusive. */
  if (write_traces == TRUE && compare_traces == TRUE) {
    fprintf (stderr, "Error: --write-traces and --compare-traces are mutually exclusive.\n");
    exit (1);
  }

  g_test_init (&argc, &argv, NULL);

  g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) msg_test_debug_handler, NULL);

  if (!g_setenv ("MSG_DEBUG", "3", FALSE))
    g_warning ("Could not set MSG_DEBUG");

  if (!g_setenv ("G_MESSAGES_DEBUG", "msgraph", FALSE))
    g_warning ("Could not set G_MESSAGES_DEBUG");

  if (!g_setenv ("MSG_LAX_SSL_CERTIFICATES", "1", FALSE))
    g_warning ("Could not set MSG_LAX_SSL_CERTIFICATES, test may not work");

  mock_server = uhm_server_new ();

  uhm_server_set_enable_logging (mock_server, write_traces);
  uhm_server_set_enable_online (mock_server, write_traces || compare_traces);

  /* Build the certificate. */
  cert_path = g_test_build_filename (G_TEST_DIST, "cert.pem", NULL);
  key_path = g_test_build_filename (G_TEST_DIST, "key.pem", NULL);
  cert = g_tls_certificate_new_from_files (cert_path, key_path, &child_error);
  g_assert_no_error (child_error);
  uhm_server_set_tls_certificate (mock_server, cert);
}

MsgAuthorizer *
msg_test_create_global_authorizer (void)
{
  if (!uhm_server_get_enable_online (mock_server)) {
    return MSG_AUTHORIZER (msg_dummy_authorizer_new ());
  }

  if (authorizer)
    return authorizer;

  authorizer = MSG_AUTHORIZER (msg_oauth2_authorizer_new (CLIENT_ID, REDIRECT_URI));
  msg_oauth2_authorizer_test_load_credentials (authorizer);

  test_authentication ();

  return authorizer;
}

MsgAuthorizer *
msg_test_get_authorizer (void)
{
  return authorizer;
}

void
msg_test_set_https_port (UhmServer *server)
{
  g_autofree char *port_string = g_strdup_printf ("%u", uhm_server_get_port (server));
  if (!g_setenv ("MSG_HTTPS_PORT", port_string, TRUE))
    g_warning ("Could not set MSG_HTTPS_PORT, testing will not work");
}

void
msg_test_mock_server_start_trace (UhmServer  *server,
                                  const char *trace_filename)
{
  GError *child_error = NULL;

  uhm_server_start_trace (server, trace_filename, &child_error);
  g_assert_no_error (child_error);
  msg_test_set_https_port (server);
}

UhmServer *
msg_test_get_mock_server (void)
{
  return mock_server;
}
