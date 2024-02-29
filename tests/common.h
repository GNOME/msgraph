#pragma once

#include "msg-oauth2-authorizer.h"

#include <uhttpmock/uhm.h>

#define CLIENT_ID "c6ab1078-80d3-40ba-a49b-375dd013251f"
#define REDIRECT_URI "https://login.microsoftonline.com/common/oauth2/nativeclient"

void
msg_test_init (int    argc,
               char **argv);

MsgAuthorizer *
msg_test_get_authorizer (void);

char *
msg_test_query_user_for_verifier (const gchar *authentication_uri);

void
msg_test_save_credentials (MsgAuthorizer *self);

UhmServer *
msg_test_get_mock_server (void);

void
msg_test_mock_server_start_trace (UhmServer  *server,
                                  const char *trace_filename);

MsgAuthorizer *
msg_test_create_global_authorizer (void);
