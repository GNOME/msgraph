/* Copyright 2022 Jan-Michael Brummer
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

#include <glib.h>

G_BEGIN_DECLS

/**
 * MSG_ERROR:
 *
 * Error domain for libmsgraph. Errors in this domain will be from the
 * #MsgError enumeration. See #GError for more information on error domains.
 */
#define MSG_ERROR (msg_error_quark ())

/**
 * MsgError:
 * @MSG_ERROR_FAILED: An unrecoverable error occurred.
 *
 * This enumeration can be expanded at a later date.
 */
typedef enum
{
  MSG_ERROR_FAILED,
  MSG_ERROR_PROTOCOL_ERROR,
} MsgError;

GQuark    msg_error_quark        (void);

G_END_DECLS

