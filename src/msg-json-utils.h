#include <json-glib/json-glib.h>

#pragma once

const char *
msg_json_object_get_string (JsonObject *object,
                            const char *name);
