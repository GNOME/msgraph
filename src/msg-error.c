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

#include "config.h"

#include "msg-error.h"


/**
 * SECTION:msg-error
 * @title: MsgError
 * @short_description: Error codes.
 * @include: msg/msg.h
 *
 * Error codes used to represent errors thrown by the MS Graph APIs.
 */
GQuark
msg_error_quark (void)
{
  return g_quark_from_static_string ("msg-error-quark");
}
