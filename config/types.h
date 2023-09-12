/**
 * @file
 * Constants for all the config types
 *
 * @authors
 * Copyright (C) 2017-2018 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUTT_CONFIG_TYPES_H
#define MUTT_CONFIG_TYPES_H

#include <stdint.h>

/* Data Types */
#define DT_ADDRESS   1  ///< e-mail address
#define DT_BOOL      2  ///< boolean option
#define DT_ENUM      3  ///< an enumeration
#define DT_HCACHE    4  ///< header cache backend
#define DT_LONG      5  ///< a number (long)
#define DT_MBTABLE   6  ///< multibyte char table
#define DT_NUMBER    7  ///< a number
#define DT_PATH      8  ///< a path to a file/directory
#define DT_QUAD      9  ///< quad-option (no/yes/ask-no/ask-yes)
#define DT_REGEX    10  ///< regular expressions
#define DT_SLIST    11  ///< a list of strings
#define DT_SORT     12  ///< sorting methods
#define DT_STRING   13  ///< a string
#define DT_SYNONYM  14  ///< synonym for another variable
#define DT_MYVAR    15  ///< a user-defined variable (my_foo)
#define DT_EXPANDO  16  ///< an expando

#define DTYPE(x) ((x) & 0x1F)  ///< Mask for the Data Type

#define DT_NO_FLAGS            0   ///< No flags are set

#define DT_NOT_EMPTY     (1 << 6)  ///< Empty strings are not allowed
#define DT_NOT_NEGATIVE  (1 << 7)  ///< Negative numbers are not allowed
#define DT_MAILBOX       (1 << 8)  ///< Don't perform path expansions
#define DT_SENSITIVE     (1 << 9)  ///< Contains sensitive value, e.g. password
#define DT_COMMAND       (1 << 10) ///< A command
#define DT_INHERIT_ACC   (1 << 11) ///< Config item can be Account-specific
#define DT_INHERIT_MBOX  (1 << 12) ///< Config item can be Mailbox-specific
#define DT_PATH_DIR      (1 << 13) ///< Path is a directory
#define DT_PATH_FILE     (1 << 14) ///< Path is a file

#define IS_SENSITIVE(type) (((type) & DT_SENSITIVE) == DT_SENSITIVE)
#define IS_MAILBOX(type)   (((type) & (DT_STRING | DT_MAILBOX)) == (DT_STRING | DT_MAILBOX))
#define IS_COMMAND(type)   (((type) & (DT_STRING | DT_COMMAND)) == (DT_STRING | DT_COMMAND))

/* subtypes for... */
#define DT_SUBTYPE_MASK  0x7FC0  ///< Mask for the Data Subtype

typedef uint32_t ConfigRedrawFlags; ///< Flags for redraw/resort, e.g. #R_INDEX
#define R_REDRAW_NO_FLAGS        0  ///< No refresh/resort flags
#define R_INDEX           (1 << 17) ///< Redraw the index menu (MENU_INDEX)
#define R_RESORT          (1 << 18) ///< Resort the mailbox
#define R_RESORT_SUB      (1 << 19) ///< Resort subthreads
#define R_RESORT_INIT     (1 << 20) ///< Resort from scratch

#define R_REDRAW_MASK  0x01E0000    ///< Mask for the Redraw Flags

/* Private config item flags */
#define DT_FREE_CONFIGDEF (1 << 26)  ///< Config item must have its ConfigDef freed
#define DT_DEPRECATED     (1 << 27)  ///< Config item shouldn't be used any more
#define DT_INHERITED      (1 << 28)  ///< Config item is inherited
#define DT_INITIAL_SET    (1 << 29)  ///< Config item must have its initial value freed
#define DT_DISABLED       (1 << 30)  ///< Config item is disabled

#endif /* MUTT_CONFIG_TYPES_H */
