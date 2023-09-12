/**
 * @file
 * PGP Key Selection Dialog
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
 * @page crypt_dlg_pgp PGP Key Selection Dialog
 *
 * The PGP Key Selection Dialog lets the user select a PGP key.
 *
 * This is a @ref gui_simple
 *
 * ## Windows
 *
 * | Name                     | Type       | See Also  |
 * | :----------------------- | :--------- | :-------- |
 * | PGP Key Selection Dialog | WT_DLG_PGP | dlg_pgp() |
 *
 * **Parent**
 * - @ref gui_dialog
 *
 * **Children**
 * - See: @ref gui_simple
 *
 * ## Data
 * - #Menu
 * - #Menu::mdata
 * - #PgpUid
 *
 * The @ref gui_simple holds a Menu.  The PGP Key Selection Dialog stores its
 * data (#PgpUid) in Menu::mdata.
 *
 * ## Events
 *
 * Once constructed, it is controlled by the following events:
 *
 * | Event Type  | Handler                   |
 * | :---------- | :------------------------ |
 * | #NT_CONFIG  | pgp_key_config_observer() |
 * | #NT_WINDOW  | pgp_key_window_observer() |
 *
 * The PGP Key Selection Dialog doesn't have any specific colours, so it
 * doesn't need to support #NT_COLOR.
 *
 * The PGP Key Selection Dialog does not implement MuttWindow::recalc() or
 * MuttWindow::repaint().
 *
 * Some other events are handled by the @ref gui_simple.
 */

#include "config.h"
#include <ctype.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "private.h"
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "lib.h"
#include "expando/lib.h"
#include "menu/lib.h"
#include "format_flags.h"
#include "keymap.h"
#include "mutt_logging.h"
#include "muttlib.h"
#include "opcodes.h"
#include "pgp.h"
#include "pgp_functions.h"
#include "pgpkey.h"
#include "pgplib.h"
#include "sort.h"

/// Help Bar for the PGP key selection dialog
static const struct Mapping PgpHelp[] = {
  // clang-format off
  { N_("Exit"),      OP_EXIT },
  { N_("Select"),    OP_GENERIC_SELECT_ENTRY },
  { N_("Check key"), OP_VERIFY_KEY },
  { N_("Help"),      OP_HELP },
  { NULL, 0 },
  // clang-format on
};

/**
 * pgp_sort_address - Compare two keys by their addresses - Implements ::sort_t - @ingroup sort_api
 */
static int pgp_sort_address(const void *a, const void *b, void *sdata)
{
  struct PgpUid const *s = *(struct PgpUid const *const *) a;
  struct PgpUid const *t = *(struct PgpUid const *const *) b;
  const bool sort_reverse = *(bool *) sdata;

  int rc = mutt_istr_cmp(s->addr, t->addr);
  if (rc != 0)
    goto done;

  rc = mutt_istr_cmp(pgp_fpr_or_lkeyid(s->parent), pgp_fpr_or_lkeyid(t->parent));

done:
  return sort_reverse ? -rc : rc;
}

/**
 * pgp_sort_date - Compare two keys by their dates - Implements ::sort_t - @ingroup sort_api
 */
static int pgp_sort_date(const void *a, const void *b, void *sdata)
{
  struct PgpUid const *s = *(struct PgpUid const *const *) a;
  struct PgpUid const *t = *(struct PgpUid const *const *) b;
  const bool sort_reverse = *(bool *) sdata;

  int rc = mutt_numeric_cmp(s->parent->gen_time, t->parent->gen_time);
  if (rc != 0)
    goto done;

  rc = mutt_istr_cmp(s->addr, t->addr);

done:
  return sort_reverse ? -rc : rc;
}

/**
 * pgp_sort_keyid - Compare two keys by their IDs - Implements ::sort_t - @ingroup sort_api
 */
static int pgp_sort_keyid(const void *a, const void *b, void *sdata)
{
  struct PgpUid const *s = *(struct PgpUid const *const *) a;
  struct PgpUid const *t = *(struct PgpUid const *const *) b;
  const bool sort_reverse = *(bool *) sdata;

  int rc = mutt_istr_cmp(pgp_fpr_or_lkeyid(s->parent), pgp_fpr_or_lkeyid(t->parent));
  if (rc != 0)
    goto done;

  rc = mutt_istr_cmp(s->addr, t->addr);

done:
  return sort_reverse ? -rc : rc;
}

/**
 * pgp_sort_trust - Compare two keys by their trust levels - Implements ::sort_t - @ingroup sort_api
 */
static int pgp_sort_trust(const void *a, const void *b, void *sdata)
{
  struct PgpUid const *s = *(struct PgpUid const *const *) a;
  struct PgpUid const *t = *(struct PgpUid const *const *) b;
  const bool sort_reverse = *(bool *) sdata;

  int rc = mutt_numeric_cmp(s->parent->flags & KEYFLAG_RESTRICTIONS,
                            t->parent->flags & KEYFLAG_RESTRICTIONS);
  if (rc != 0)
    goto done;

  // Note: reversed
  rc = mutt_numeric_cmp(t->trust, s->trust);
  if (rc != 0)
    goto done;

  // Note: reversed
  rc = mutt_numeric_cmp(t->parent->keylen, s->parent->keylen);
  if (rc != 0)
    goto done;

  // Note: reversed
  rc = mutt_numeric_cmp(t->parent->gen_time, s->parent->gen_time);
  if (rc != 0)
    goto done;

  rc = mutt_istr_cmp(s->addr, t->addr);
  if (rc != 0)
    goto done;

  rc = mutt_istr_cmp(pgp_fpr_or_lkeyid(s->parent), pgp_fpr_or_lkeyid(t->parent));

done:
  return sort_reverse ? -rc : rc;
}

/**
 * pgp_make_entry - Format a menu item for the pgp key list - Implements Menu::make_entry() - @ingroup menu_make_entry
 *
 * @sa $pgp_entry_format, pgp_entry_format_str()
 */
static void pgp_make_entry(struct Menu *menu, char *buf, size_t buflen, int line)
{
  struct PgpUid **key_table = menu->mdata;

  struct PgpEntry entry = { 0 };
  entry.uid = key_table[line];
  entry.num = line + 1;

  const struct ExpandoRecord *c_pgp_entry_format = cs_subset_expando(NeoMutt->sub, "pgp_entry_format");

  /* $pgp_entry_format is used in ncrypt/dlg_gpgme.c with differnet callbacks 
     so we need to check if it is validated for ncrypt/dlg_pgp.c */
  const bool ok = expando_revalidate((struct ExpandoRecord *) c_pgp_entry_format,
                                     EFMTI_PGP_ENTRY_FORMAT_DLG_PGP);

  if (ok)
  {
    mutt_expando_format_2gmb(buf, buflen, 0, menu->win->state.cols,
                             &c_pgp_entry_format->tree, (intptr_t) &entry,
                             MUTT_FORMAT_ARROWCURSOR);
  }
}

/**
 * pgp_key_table_free - Free the key table - Implements Menu::mdata_free() - @ingroup menu_mdata_free
 *
 * @note The keys are owned by the caller of the dialog
 */
static void pgp_key_table_free(struct Menu *menu, void **ptr)
{
  FREE(ptr);
}

/**
 * pgp_key_config_observer - Notification that a Config Variable has changed - Implements ::observer_t - @ingroup observer_api
 */
static int pgp_key_config_observer(struct NotifyCallback *nc)
{
  if (nc->event_type != NT_CONFIG)
    return 0;
  if (!nc->global_data || !nc->event_data)
    return -1;

  struct EventConfig *ev_c = nc->event_data;

  if (!mutt_str_equal(ev_c->name, "pgp_entry_format") &&
      !mutt_str_equal(ev_c->name, "pgp_sort_keys"))
  {
    return 0;
  }

  struct Menu *menu = nc->global_data;
  menu_queue_redraw(menu, MENU_REDRAW_FULL);
  mutt_debug(LL_DEBUG5, "config done, request WA_RECALC, MENU_REDRAW_FULL\n");

  return 0;
}

/**
 * pgp_key_window_observer - Notification that a Window has changed - Implements ::observer_t - @ingroup observer_api
 *
 * This function is triggered by changes to the windows.
 *
 * - Delete (this window): clean up the resources held by the Help Bar
 */
static int pgp_key_window_observer(struct NotifyCallback *nc)
{
  if (nc->event_type != NT_WINDOW)
    return 0;
  if (!nc->global_data || !nc->event_data)
    return -1;
  if (nc->event_subtype != NT_WINDOW_DELETE)
    return 0;

  struct MuttWindow *win_menu = nc->global_data;
  struct EventWindow *ev_w = nc->event_data;
  if (ev_w->win != win_menu)
    return 0;

  struct Menu *menu = win_menu->wdata;

  notify_observer_remove(NeoMutt->sub->notify, pgp_key_config_observer, menu);
  notify_observer_remove(win_menu->notify, pgp_key_window_observer, win_menu);

  mutt_debug(LL_DEBUG5, "window delete done\n");
  return 0;
}

/**
 * dlg_pgp - Let the user select a key to use - @ingroup gui_dlg
 * @param keys List of PGP keys
 * @param p    Address to match
 * @param s    String to match
 * @retval ptr Selected PGP key
 *
 * The Select PGP Key Dialog lets the user select an PGP Key to use.
 */
struct PgpKeyInfo *dlg_pgp(struct PgpKeyInfo *keys, struct Address *p, const char *s)
{
  struct PgpUid **key_table = NULL;
  struct Menu *menu = NULL;
  char buf[1024] = { 0 };
  struct PgpUid *a = NULL;
  bool unusable = false;
  int keymax = 0;

  const bool c_pgp_show_unusable = cs_subset_bool(NeoMutt->sub, "pgp_show_unusable");
  int i = 0;
  for (struct PgpKeyInfo *kp = keys; kp; kp = kp->next)
  {
    if (!c_pgp_show_unusable && (kp->flags & KEYFLAG_CANTUSE))
    {
      unusable = true;
      continue;
    }

    for (a = kp->address; a; a = a->next)
    {
      if (!c_pgp_show_unusable && (a->flags & KEYFLAG_CANTUSE))
      {
        unusable = true;
        continue;
      }

      if (i == keymax)
      {
        keymax += 5;
        mutt_mem_realloc(&key_table, sizeof(struct PgpUid *) * keymax);
      }

      key_table[i++] = a;
    }
  }

  if ((i == 0) && unusable)
  {
    mutt_error(_("All matching keys are expired, revoked, or disabled"));
    return NULL;
  }

  sort_t f = NULL;
  short c_pgp_sort_keys = cs_subset_sort(NeoMutt->sub, "pgp_sort_keys");
  switch (c_pgp_sort_keys & SORT_MASK)
  {
    case SORT_ADDRESS:
      f = pgp_sort_address;
      break;
    case SORT_DATE:
      f = pgp_sort_date;
      break;
    case SORT_KEYID:
      f = pgp_sort_keyid;
      break;
    case SORT_TRUST:
    default:
      f = pgp_sort_trust;
      break;
  }

  if (key_table)
  {
    bool sort_reverse = c_pgp_sort_keys & SORT_REVERSE;
    mutt_qsort_r(key_table, i, sizeof(struct PgpUid *), f, &sort_reverse);
  }

  struct MuttWindow *dlg = simple_dialog_new(MENU_PGP, WT_DLG_PGP, PgpHelp);

  menu = dlg->wdata;
  menu->max = i;
  menu->make_entry = pgp_make_entry;
  menu->mdata = key_table;
  menu->mdata_free = pgp_key_table_free;

  struct PgpData pd = { false, menu, key_table, NULL };
  dlg->wdata = &pd;

  // NT_COLOR is handled by the SimpleDialog
  notify_observer_add(NeoMutt->sub->notify, NT_CONFIG, pgp_key_config_observer, menu);
  notify_observer_add(menu->win->notify, NT_WINDOW, pgp_key_window_observer, menu->win);

  if (p)
    snprintf(buf, sizeof(buf), _("PGP keys matching <%s>"), buf_string(p->mailbox));
  else
    snprintf(buf, sizeof(buf), _("PGP keys matching \"%s\""), s);

  struct MuttWindow *sbar = window_find_child(dlg, WT_STATUS_BAR);
  sbar_set_title(sbar, buf);

  mutt_clear_error();

  // ---------------------------------------------------------------------------
  // Event Loop
  int op = OP_NULL;
  do
  {
    menu_tagging_dispatcher(menu->win, op);
    window_redraw(NULL);

    op = km_dokey(MENU_PGP, GETCH_NO_FLAGS);
    mutt_debug(LL_DEBUG1, "Got op %s (%d)\n", opcodes_get_name(op), op);
    if (op < 0)
      continue;
    if (op == OP_NULL)
    {
      km_error_key(MENU_PGP);
      continue;
    }
    mutt_clear_error();

    int rc = pgp_function_dispatcher(dlg, op);

    if (rc == FR_UNKNOWN)
      rc = menu_function_dispatcher(menu->win, op);
    if (rc == FR_UNKNOWN)
      rc = global_function_dispatcher(NULL, op);
  } while (!pd.done);
  // ---------------------------------------------------------------------------

  simple_dialog_free(&dlg);
  return pd.key;
}
