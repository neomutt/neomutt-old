/**
 * @file
 * Config used by the Help Backend
 *
 * @authors
 * Copyright (C) 2020 Richard Russon <rich@flatcap.org>
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

/**
 * @page help_config Config used by the Help Backend
 *
 * Config used by the Help Backend
 */

#include "config.h"
#include <stddef.h>
#include <config/lib.h>
#include <stdbool.h>

// clang-format off
#ifdef USE_DEVEL_HELP
char *C_HelpDocDir;                 ///< Config: Location of the Help Mailbox
#endif
// clang-format on

struct ConfigDef HelpVars[] = {
  // clang-format off
  { "help_doc_dir", DT_STRING|DT_PATH, &C_HelpDocDir, IP PKGDOCDIR "/help", 0, NULL,
    "Location of the Help Mailbox"
  },
  { NULL, 0, NULL, 0, 0, NULL, NULL },
  // clang-format on
};

/**
 * config_init_help - Register help config variables
 */
bool config_init_help(struct ConfigSet *cs)
{
  return cs_register_variables(cs, HelpVars, 0);
}
