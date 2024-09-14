/* Copyright 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
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

#pragma once

#if !defined(_MSG_INSIDE) && !defined(MSG_COMPILATION)
#error "Only <msg.h> can be included directly."
#endif

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

#define MSG_TYPE_SITE (msg_site_get_type ())

G_DECLARE_FINAL_TYPE (MsgSite, msg_site, MSG, SITE, GObject);

struct _MsgSiteClass {
  GObjectClass parent_class;

  gpointer padding[4];
};

MsgSite *
msg_site_new_from_json (JsonObject  *object);

const char *
msg_site_get_id (MsgSite *self);

const char *
msg_site_get_name (MsgSite *self);
