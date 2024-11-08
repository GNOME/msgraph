#include "msg-json-utils.h"

const char *
msg_json_object_get_string (JsonObject *object,
                            const char *name)
{
  if (json_object_has_member (object, name))
    return json_object_get_string_member (object, name);

  return NULL;
}

gboolean
msg_json_object_get_boolean (JsonObject *object,
                             const char *name)
{
  if (json_object_has_member (object, name)) {
    return json_object_get_boolean_member (object, name);
  }

  return FALSE;
}
