/* Copyright 2022-2024 Jan-Michael Brummer
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

#include <glib-object.h>

#include <config.h>

G_BEGIN_DECLS

#define MSG_API_ENDPOINT "https://graph.microsoft.com/v1.0"
#define MSG_BETA_API_ENDPOINT "https://graph.microsoft.com/beta"

#ifdef USE_LIBSOUP2
#define soup_message_get_request_headers(x) x->request_headers
#define soup_message_get_response_headers(x) x->response_headers
#define soup_message_get_status(x) x->status_code
#define soup_message_get_reason_phrase(x) x->reason_phrase
#endif

G_END_DECLS

