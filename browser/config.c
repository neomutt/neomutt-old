/**
 * @file
 * Config used by libbrowser
 *
 * @authors
 * Copyright (C) 2021 Richard Russon <rich@flatcap.org>
 * Copyright (C) 2022 Carlos Henrique Lima Melara <charlesmelara@outlook.com>
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
 * @page browser_config Config used by libbrowser
 *
 * Config used by libbrowser
 */

#include "config.h"
#include <stddef.h>
#include <stdbool.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "expando/lib.h"

/**
 * SortBrowserMethods - Sort methods for the folder/dir browser
 */
static const struct Mapping SortBrowserMethods[] = {
  // clang-format off
  { "alpha",    SORT_SUBJECT },
  { "count",    SORT_COUNT },
  { "date",     SORT_DATE },
  { "desc",     SORT_DESC },
  { "new",      SORT_UNREAD },
  { "unread",   SORT_UNREAD },
  { "size",     SORT_SIZE },
  { "unsorted", SORT_ORDER },
  { NULL, 0 },
  // clang-format on
};

/**
 * BrowserVars - Config definitions for the browser
 */
static struct ConfigDef BrowserVars[] = {
  // clang-format off
  { "browser_abbreviate_mailboxes", DT_BOOL, true, 0, NULL,
    "Abbreviate mailboxes using '~' and '=' in the browser"
  },
  { "folder_format", DT_EXPANDO|DT_NOT_EMPTY, IP "%2C %t %N %F %2l %-8.8u %-8.8g %8s %d %i", 0, expando_validator,
    "printf-like format string for the browser's display of folders"
  },
  { "group_index_format", DT_EXPANDO|DT_NOT_EMPTY|R_INDEX, IP "%4C %M%N %5s  %-45.45f %d", 0, expando_validator,
    "(nntp) printf-like format string for the browser's display of newsgroups"
  },
  { "mailbox_folder_format", DT_EXPANDO|DT_NOT_EMPTY, IP "%2C %<n?%6n&      > %6m %i", 0, expando_validator,
    "printf-like format string for the browser's display of mailbox folders"
  },
  { "mask", DT_REGEX|DT_REGEX_MATCH_CASE|DT_REGEX_ALLOW_NOT|DT_REGEX_NOSUB, IP "!^\\.[^.]", 0, NULL,
    "Only display files/dirs matching this regex in the browser"
  },
  { "show_only_unread", DT_BOOL, false, 0, NULL,
    "(nntp) Only show subscribed newsgroups with unread articles"
  },
  { "sort_browser", DT_SORT|DT_SORT_REVERSE, SORT_ALPHA, IP SortBrowserMethods, NULL,
    "Sort method for the browser"
  },
  { "browser_sort_dirs_first", DT_BOOL, false, 0, NULL,
    "Group directories before files in the browser"
  },
  { NULL },
  // clang-format on
};

/**
 * config_init_browser - Register browser config variables - Implements ::module_init_config_t - @ingroup cfg_module_api
 */
bool config_init_browser(struct ConfigSet *cs)
{
  return cs_register_variables(cs, BrowserVars, DT_NO_FLAGS);
}
