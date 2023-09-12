/**
 * @file
 * Mixmaster Hosts Window
 *
 * @authors
 * Copyright (C) 2022 Richard Russon <rich@flatcap.org>
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
 * @page mixmaster_win_hosts Mixmaster Hosts Window
 *
 * Display and editable list of selected Remailer Hosts.
 *
 * ## Windows
 *
 * | Name                   | Type    | See Also        |
 * | :--------------------- | :------ | :-------------- |
 * | Mixmaster Hosts Window | WT_MENU | win_hosts_new() |
 *
 * **Parent**
 * - @ref mixmaster_dlg_mixmaster
 *
 * **Children**
 * - None
 *
 * ## Data
 * - RemailerArray
 *
 * The Hosts Window stores its data (RemailerArray) in Menu::mdata.
 */

#include "config.h"
#include <stddef.h>
#include <stdint.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "expando/lib.h"
#include "menu/lib.h"
#include "muttlib.h"
#include "remailer.h" // IWYU pragma: keep

/**
 * mix_make_entry - Format a menu item for the mixmaster chain list - Implements Menu::make_entry() - @ingroup menu_make_entry
 *
 * @sa $mix_entry_format, mix_format_str()
 */
static void mix_make_entry(struct Menu *menu, char *buf, size_t buflen, int num)
{
  struct RemailerArray *ra = menu->mdata;
  struct Remailer **r = ARRAY_GET(ra, num);
  if (!r)
    return;

  const struct ExpandoRecord *c_mix_entry_format = cs_subset_expando(NeoMutt->sub, "mix_entry_format");
  mutt_expando_format(buf, buflen, menu->win->state.cols, c_mix_entry_format,
                      (intptr_t) *r, MUTT_FORMAT_ARROWCURSOR);
}

/**
 * win_hosts_new - Create a new Hosts Window
 * @param ra Array of Remailer hosts
 * @retval ptr New Hosts Window
 */
struct MuttWindow *win_hosts_new(struct RemailerArray *ra)
{
  struct MuttWindow *win_hosts = menu_window_new(MENU_MIX, NeoMutt->sub);
  win_hosts->focus = win_hosts;

  struct Menu *menu = win_hosts->wdata;

  menu->max = ARRAY_SIZE(ra);
  menu->make_entry = mix_make_entry;
  menu->tag = NULL;
  menu->mdata = ra;
  menu->mdata_free = NULL; // Menu doesn't own the data

  return win_hosts;
}

/**
 * win_hosts_get_selection - Get the current selection
 * @param win Hosts Window
 * @retval ptr Currently selected Remailer
 */
struct Remailer *win_hosts_get_selection(struct MuttWindow *win)
{
  if (!win || !win->wdata)
    return NULL;

  struct Menu *menu = win->wdata;
  if (!menu->mdata)
    return NULL;

  struct RemailerArray *ra = menu->mdata;

  const int sel = menu_get_index(menu);
  if (sel < 0)
    return NULL;

  struct Remailer **r = ARRAY_GET(ra, sel);
  if (!r)
    return NULL;

  return *r;
}
