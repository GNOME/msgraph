/* Copyright 2022-2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "msg-site.h"
#include "msg-error.h"
#include "msg-json-utils.h"

struct _MsgSite {
  GObject parent_instance;
};

typedef struct {
  char *id;
  char *name;
} MsgSitePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MsgSite, msg_site, G_TYPE_OBJECT);

static void
msg_site_finalize (GObject *object)
{
  MsgSite *self = MSG_SITE (object);
  MsgSitePrivate *priv = msg_site_get_instance_private (self);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->name, g_free);

  G_OBJECT_CLASS (msg_site_parent_class)->finalize (object);
}

static void
msg_site_init (__attribute__ ((unused)) MsgSite *item)
{
}

static void
msg_site_class_init (MsgSiteClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = msg_site_finalize;
}

/**
 * msg_site_new_from_json:
 * @object: The json object to parse
 *
 * Creates a new `MsgSite` from json response object.
 *
 * Returns: the newly created `MsgSite`
 */
MsgSite *
msg_site_new_from_json (JsonObject  *object)
{
  g_autoptr (MsgSite) self = NULL;
  g_autoptr (GError) _error = NULL;
  MsgSitePrivate *priv;

  self = g_object_new (MSG_TYPE_SITE, NULL);
  priv = msg_site_get_instance_private (self);
  priv->id = g_strdup (msg_json_object_get_string (object, "id"));
  priv->name = g_strdup (msg_json_object_get_string (object, "displayName"));

  return g_steal_pointer (&self);
}

/**
 * msg_site_get_nid:
 * @self: a site
 *
 * Gets ID of site.
 *
 * Returns: (transfer none): id of site
 */
const char *
msg_site_get_id (MsgSite *self)
{
  MsgSitePrivate *priv = msg_site_get_instance_private (self);

  return priv->id;
}

/**
 * msg_site_get_name:
 * @self: a site
 *
 * Gets name of site.
 *
 * Returns: (transfer none): name of site
 */
const char *
msg_site_get_name (MsgSite *self)
{
  MsgSitePrivate *priv = msg_site_get_instance_private (self);

  return priv->name;
}

