/**
 * @file
 * Compose Envelope
 *
 * @authors
 * Copyright (C) 2021 Richard Russon <rich@flatcap.org>
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
 * @page compose_envelope Envelope Window
 *
 * The Compose Envelope Window displays the header fields of an email.
 *
 * ## Windows
 *
 * | Name                    | Type      | See Also          |
 * | :---------------------- | :-------- | :---------------- |
 * | Compose Envelope Window | WT_CUSTOM | compose_env_new() |
 *
 * **Parent**
 * - @ref compose_dialog
 *
 * **Children**
 *
 * None.
 *
 * ## Data
 * - #ComposeEnvelopeData
 *
 * The Compose Envelope Window stores its data (#ComposeEnvelopeData) in
 * MuttWindow::wdata.
 *
 * ## Events
 *
 * Once constructed, it is controlled by the following events:
 *
 * | Event Type            | Handler                |
 * | :-------------------- | :--------------------- |
 * | #NT_COLOR             | env_color_observer()   |
 * | #NT_COMPOSE           | env_compose_observer() |
 * | #NT_CONFIG            | env_config_observer()  |
 * | #NT_HEADER            | env_header_observer()  |
 * | #NT_WINDOW            | env_window_observer()  |
 * | MuttWindow::recalc()  | env_recalc()           |
 * | MuttWindow::repaint() | env_repaint()          |
 */

#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include "private.h"
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "color/lib.h"
#include "ncrypt/lib.h"
#include "env_data.h"
#include "options.h"
#include "shared_data.h"
#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/// Maximum number of rows to use for the To:, Cc:, Bcc: fields
#define MAX_ADDR_ROWS 5

/// Maximum number of rows to use for the Headers: field
#define MAX_USER_HDR_ROWS 5

#ifdef USE_AUTOCRYPT
static const char *const AutocryptRecUiFlags[] = {
  /* L10N: Autocrypt recommendation flag: off.
     This is displayed when Autocrypt is turned off. */
  N_("Off"),
  /* L10N: Autocrypt recommendation flag: no.
     This is displayed when Autocrypt cannot encrypt to the recipients. */
  N_("No"),
  /* L10N: Autocrypt recommendation flag: discouraged.
     This is displayed when Autocrypt believes encryption should not be used.
     This might occur if one of the recipient Autocrypt Keys has not been
     used recently, or if the only key available is a Gossip Header key. */
  N_("Discouraged"),
  /* L10N: Autocrypt recommendation flag: available.
     This is displayed when Autocrypt believes encryption is possible, but
     leaves enabling it up to the sender.  Probably because "prefer encrypt"
     is not set in both the sender and recipient keys. */
  N_("Available"),
  /* L10N: Autocrypt recommendation flag: yes.
     This is displayed when Autocrypt would normally enable encryption
     automatically. */
  N_("Yes"),
};
#endif

/**
 * calc_address - Calculate how many rows an AddressList will need
 * @param[in]  al    Address List
 * @param[out] slist String list
 * @param[in]  cols Screen columns available
 * @param[out] srows Rows needed
 * @retval num Rows needed
 *
 * @note Number of rows is capped at #MAX_ADDR_ROWS
 */
static int calc_address(struct AddressList *al, struct ListHead *slist,
                        short cols, short *srows)
{
  mutt_list_free(slist);
  mutt_addrlist_write_list(al, slist);

  int rows = 1;
  int addr_len;
  int width_left = cols;
  struct ListNode *next = NULL;
  struct ListNode *np = NULL;
  STAILQ_FOREACH(np, slist, entries)
  {
    next = STAILQ_NEXT(np, entries);
    addr_len = mutt_strwidth(np->data);
    if (next)
      addr_len += 2; // ", "

  try_again:
    if (addr_len >= width_left)
    {
      if (width_left == cols)
        break;

      rows++;
      width_left = cols;
      goto try_again;
    }

    if (addr_len < width_left)
      width_left -= addr_len;
  }

  *srows = MIN(rows, MAX_ADDR_ROWS);
  return *srows;
}

/**
 * calc_security - Calculate how many rows the security info will need
 * @param e    Email
 * @param rows Rows needed
 * @param sub  ConfigSubset
 * @retval num Rows needed
 */
static int calc_security(struct Email *e, short *rows, const struct ConfigSubset *sub)
{
  if ((WithCrypto & (APPLICATION_PGP | APPLICATION_SMIME)) == 0)
    *rows = 0; // Neither PGP nor SMIME are built into NeoMutt
  else if ((e->security & (SEC_ENCRYPT | SEC_SIGN)) != 0)
    *rows = 2; // 'Security:' and 'Sign as:'
  else
    *rows = 1; // Just 'Security:'

#ifdef USE_AUTOCRYPT
  const bool c_autocrypt = cs_subset_bool(sub, "autocrypt");
  if (c_autocrypt)
    *rows += 1;
#endif

  return *rows;
}

/**
 * calc_user_hdrs - Calculate how many rows are needed for user-defined headers
 * @param hdrs Header List
 * @retval num Rows needed, limited to #MAX_USER_HDR_ROWS
 */
static int calc_user_hdrs(const struct ListHead *hdrs)
{
  int rows = 0; /* Don't print at all if no custom headers*/
  struct ListNode *np = NULL;
  STAILQ_FOREACH(np, hdrs, entries)
  {
    if (rows == MAX_USER_HDR_ROWS)
      break;
    rows++;
  }
  return rows;
}

/**
 * calc_envelope - Calculate how many rows the envelope will need
 * @param win    Window to draw on
 * @param shared Shared compose data
 * @param edata  Envelope data
 * @retval num Rows needed
 */
static int calc_envelope(struct MuttWindow *win, struct ComposeSharedData *shared,
                         struct ComposeEnvelopeData *edata)
{
  int rows = 4; // 'From:', 'Subject:', 'Reply-To:', 'Fcc:'

  struct Email *e = shared->email;
  struct Envelope *env = e->env;
  const int cols = win->state.cols - MaxHeaderWidth;

#ifdef USE_NNTP
  if (OptNewsSend)
  {
    rows += 2; // 'Newsgroups:' and 'Followup-To:'
    const bool c_x_comment_to = cs_subset_bool(shared->sub, "x_comment_to");
    if (c_x_comment_to)
      rows++;
  }
  else
#endif
  {
    rows += calc_address(&env->to, &edata->to_list, cols, &edata->to_rows);
    rows += calc_address(&env->cc, &edata->cc_list, cols, &edata->cc_rows);
    rows += calc_address(&env->bcc, &edata->bcc_list, cols, &edata->bcc_rows);
  }
  rows += calc_security(e, &edata->sec_rows, shared->sub);
  const bool c_compose_show_user_headers =
      cs_subset_bool(shared->sub, "compose_show_user_headers");
  if (c_compose_show_user_headers)
    rows += calc_user_hdrs(&env->userhdrs);

  return rows;
}

/**
 * draw_floating - Draw a floating label
 * @param win  Window to draw on
 * @param col  Column to draw at
 * @param row  Row to draw at
 * @param text Text to display
 */
static void draw_floating(struct MuttWindow *win, int col, int row, const char *text)
{
  mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_HEADER);
  mutt_window_mvprintw(win, col, row, "%s", text);
  mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
}

/**
 * draw_header - Draw an aligned label
 * @param win   Window to draw on
 * @param row   Row to draw at
 * @param field Field to display, e.g. #HDR_FROM
 */
static void draw_header(struct MuttWindow *win, int row, enum HeaderField field)
{
  mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_HEADER);
  mutt_window_mvprintw(win, 0, row, "%*s", HeaderPadding[field], _(Prompts[field]));
  mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
}

/**
 * draw_header_content - Draw content on a separate line aligned to header prompt
 * @param win     Window to draw on
 * @param row     Row to draw at
 * @param field   Field to display, e.g. #HDR_FROM
 * @param content Text to display
 *
 * Content will be truncated if it is wider than the window.
 */
static void draw_header_content(struct MuttWindow *win, int row,
                                enum HeaderField field, const char *content)
{
  mutt_window_move(win, HeaderPadding[field], row);
  mutt_paddstr(win, win->state.cols - HeaderPadding[field], content);
}

/**
 * draw_crypt_lines - Update the encryption info in the compose window
 * @param win    Window to draw on
 * @param shared Shared compose data
 * @param edata  Envelope data
 * @param row    Window row to start drawing
 * @retval num Number of lines used
 */
static int draw_crypt_lines(struct MuttWindow *win, struct ComposeSharedData *shared,
                            struct ComposeEnvelopeData *edata, int row)
{
  struct Email *e = shared->email;

  draw_header(win, row++, HDR_CRYPT);

  if ((WithCrypto & (APPLICATION_PGP | APPLICATION_SMIME)) == 0)
    return 0;

  // We'll probably need two lines for 'Security:' and 'Sign as:'
  int used = 2;
  if ((e->security & (SEC_ENCRYPT | SEC_SIGN)) == (SEC_ENCRYPT | SEC_SIGN))
  {
    mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_BOTH);
    mutt_window_addstr(win, _("Sign, Encrypt"));
  }
  else if (e->security & SEC_ENCRYPT)
  {
    mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_ENCRYPT);
    mutt_window_addstr(win, _("Encrypt"));
  }
  else if (e->security & SEC_SIGN)
  {
    mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_SIGN);
    mutt_window_addstr(win, _("Sign"));
  }
  else
  {
    /* L10N: This refers to the encryption of the email, e.g. "Security: None" */
    mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_NONE);
    mutt_window_addstr(win, _("None"));
    used = 1; // 'Sign as:' won't be needed
  }
  mutt_curses_set_color_by_id(MT_COLOR_NORMAL);

  if ((e->security & (SEC_ENCRYPT | SEC_SIGN)))
  {
    if (((WithCrypto & APPLICATION_PGP) != 0) && (e->security & APPLICATION_PGP))
    {
      if ((e->security & SEC_INLINE))
        mutt_window_addstr(win, _(" (inline PGP)"));
      else
        mutt_window_addstr(win, _(" (PGP/MIME)"));
    }
    else if (((WithCrypto & APPLICATION_SMIME) != 0) && (e->security & APPLICATION_SMIME))
      mutt_window_addstr(win, _(" (S/MIME)"));
  }

  const bool c_crypt_opportunistic_encrypt =
      cs_subset_bool(shared->sub, "crypt_opportunistic_encrypt");
  if (c_crypt_opportunistic_encrypt && (e->security & SEC_OPPENCRYPT))
    mutt_window_addstr(win, _(" (OppEnc mode)"));

  mutt_window_clrtoeol(win);

  if (((WithCrypto & APPLICATION_PGP) != 0) &&
      (e->security & APPLICATION_PGP) && (e->security & SEC_SIGN))
  {
    draw_header(win, row++, HDR_CRYPTINFO);
    const char *const c_pgp_sign_as =
        cs_subset_string(shared->sub, "pgp_sign_as");
    mutt_window_printf(win, "%s", c_pgp_sign_as ? c_pgp_sign_as : _("<default>"));
  }

  if (((WithCrypto & APPLICATION_SMIME) != 0) &&
      (e->security & APPLICATION_SMIME) && (e->security & SEC_SIGN))
  {
    draw_header(win, row++, HDR_CRYPTINFO);
    const char *const c_smime_sign_as =
        cs_subset_string(shared->sub, "pgp_sign_as");
    mutt_window_printf(win, "%s", c_smime_sign_as ? c_smime_sign_as : _("<default>"));
  }

  const char *const c_smime_encrypt_with =
      cs_subset_string(shared->sub, "smime_encrypt_with");
  if (((WithCrypto & APPLICATION_SMIME) != 0) && (e->security & APPLICATION_SMIME) &&
      (e->security & SEC_ENCRYPT) && c_smime_encrypt_with)
  {
    draw_floating(win, 40, row - 1, _("Encrypt with: "));
    mutt_window_printf(win, "%s", NONULL(c_smime_encrypt_with));
  }

#ifdef USE_AUTOCRYPT
  const bool c_autocrypt = cs_subset_bool(shared->sub, "autocrypt");
  if (c_autocrypt)
  {
    draw_header(win, row, HDR_AUTOCRYPT);
    if (e->security & SEC_AUTOCRYPT)
    {
      mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_ENCRYPT);
      mutt_window_addstr(win, _("Encrypt"));
    }
    else
    {
      mutt_curses_set_color_by_id(MT_COLOR_COMPOSE_SECURITY_NONE);
      mutt_window_addstr(win, _("Off"));
    }

    /* L10N: The autocrypt compose menu Recommendation field.
       Displays the output of the recommendation engine
       (Off, No, Discouraged, Available, Yes) */
    draw_floating(win, 40, row, _("Recommendation: "));
    mutt_window_printf(win, "%s", _(AutocryptRecUiFlags[edata->autocrypt_rec]));

    used++;
  }
#endif
  return used;
}

/**
 * draw_envelope_addr - Write addresses to the compose window
 * @param field     Field to display, e.g. #HDR_FROM
 * @param al        Address list to write
 * @param win       Window
 * @param row       Window row to start drawing
 * @param max_lines How many lines may be used
 * @retval num Lines used
 */
static int draw_envelope_addr(int field, struct AddressList *al,
                              struct MuttWindow *win, int row, size_t max_lines)
{
  draw_header(win, row, field);

  struct ListHead list = STAILQ_HEAD_INITIALIZER(list);
  int count = mutt_addrlist_write_list(al, &list);

  int lines_used = 1;
  int width_left = win->state.cols - MaxHeaderWidth;
  char more[32];
  int more_len = 0;

  char *sep = NULL;
  struct ListNode *next = NULL;
  struct ListNode *np = NULL;
  STAILQ_FOREACH(np, &list, entries)
  {
    next = STAILQ_NEXT(np, entries);
    int addr_len = mutt_strwidth(np->data);
    if (next)
    {
      sep = ", ";
      addr_len += 2;
    }
    else
    {
      sep = "";
    }

    count--;
  try_again:
    more_len = snprintf(more, sizeof(more),
                        ngettext("(+%d more)", "(+%d more)", count), count);
    mutt_debug(LL_DEBUG3, "text: '%s'  len: %d\n", more, more_len);

    int reserve = ((count > 0) && (lines_used == max_lines)) ? more_len : 0;
    mutt_debug(LL_DEBUG3, "processing: %s (al:%ld, wl:%d, r:%d, lu:%ld)\n",
               np->data, addr_len, width_left, reserve, lines_used);
    if (addr_len >= (width_left - reserve))
    {
      mutt_debug(LL_DEBUG3, "not enough space\n");
      if (lines_used == max_lines)
      {
        mutt_debug(LL_DEBUG3, "no more lines\n");
        mutt_debug(LL_DEBUG3, "truncating: %s\n", np->data);
        mutt_paddstr(win, width_left, np->data);
        break;
      }

      if (width_left == (win->state.cols - MaxHeaderWidth))
      {
        mutt_debug(LL_DEBUG3, "couldn't print: %s\n", np->data);
        mutt_paddstr(win, width_left, np->data);
        break;
      }

      mutt_debug(LL_DEBUG3, "start a new line\n");
      mutt_window_clrtoeol(win);
      row++;
      lines_used++;
      width_left = win->state.cols - MaxHeaderWidth;
      mutt_window_move(win, MaxHeaderWidth, row);
      goto try_again;
    }

    if (addr_len < width_left)
    {
      mutt_debug(LL_DEBUG3, "space for: %s\n", np->data);
      mutt_window_addstr(win, np->data);
      mutt_window_addstr(win, sep);
      width_left -= addr_len;
    }
    mutt_debug(LL_DEBUG3, "%ld addresses remaining\n", count);
    mutt_debug(LL_DEBUG3, "%ld lines remaining\n", max_lines - lines_used);
  }
  mutt_list_free(&list);

  if (count > 0)
  {
    mutt_window_move(win, win->state.cols - more_len, row);
    mutt_curses_set_color_by_id(MT_COLOR_BOLD);
    mutt_window_addstr(win, more);
    mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
    mutt_debug(LL_DEBUG3, "%ld more (len %d)\n", count, more_len);
  }
  else
  {
    mutt_window_clrtoeol(win);
  }

  for (int i = lines_used; i < max_lines; i++)
  {
    mutt_window_move(win, 0, row + i);
    mutt_window_clrtoeol(win);
  }

  mutt_debug(LL_DEBUG3, "used %d lines\n", lines_used);
  return lines_used;
}

/**
 * draw_envelope_user_hdrs - Write user-defined headers to the compose window
 * @param win    Window to draw on
 * @param shared Shared compose data
 * @param row    Window row to start drawing from
 * @retval num Rows used
 */
static int draw_envelope_user_hdrs(struct MuttWindow *win,
                                   struct ComposeSharedData *shared, int row)
{
  const char *overflow_text = "...";
  int rows_used = 0;

  struct ListNode *first = STAILQ_FIRST(&shared->email->env->userhdrs);
  if (!first)
    return rows_used;

  /* Draw first entry on same line as prompt */
  draw_header(win, row, HDR_CUSTOM_HEADERS);
  mutt_paddstr(win,
               win->state.cols - (HeaderPadding[HDR_CUSTOM_HEADERS] +
                                  mutt_strwidth(_(Prompts[HDR_CUSTOM_HEADERS]))),
               first->data);
  rows_used++;

  /* Draw any following entries on their own line */
  struct ListNode *np = STAILQ_NEXT(first, entries);
  if (!np)
    return rows_used;

  STAILQ_FOREACH_FROM(np, &shared->email->env->userhdrs, entries)
  {
    if ((rows_used == (MAX_USER_HDR_ROWS - 1)) && STAILQ_NEXT(np, entries))
    {
      draw_header_content(win, row + rows_used, HDR_CUSTOM_HEADERS, overflow_text);
      rows_used++;
      break;
    }
    draw_header_content(win, row + rows_used, HDR_CUSTOM_HEADERS, np->data);
    rows_used++;
  }
  return rows_used;
}

/**
 * draw_envelope - Write the email headers to the compose window
 * @param win    Window to draw on
 * @param shared Shared compose data
 * @param edata  Envelope data
 */
static void draw_envelope(struct MuttWindow *win, struct ComposeSharedData *shared,
                          struct ComposeEnvelopeData *edata)
{
  struct Email *e = shared->email;
  const char *fcc = mutt_buffer_string(edata->fcc);
  const int cols = win->state.cols - MaxHeaderWidth;

  mutt_window_clear(win);
  int row = draw_envelope_addr(HDR_FROM, &e->env->from, win, 0, 1);

#ifdef USE_NNTP
  if (OptNewsSend)
  {
    draw_header(win, row++, HDR_NEWSGROUPS);
    mutt_paddstr(win, cols, NONULL(e->env->newsgroups));

    draw_header(win, row++, HDR_FOLLOWUPTO);
    mutt_paddstr(win, cols, NONULL(e->env->followup_to));

    const bool c_x_comment_to = cs_subset_bool(shared->sub, "x_comment_to");
    if (c_x_comment_to)
    {
      draw_header(win, row++, HDR_XCOMMENTTO);
      mutt_paddstr(win, cols, NONULL(e->env->x_comment_to));
    }
  }
  else
#endif
  {
    row += draw_envelope_addr(HDR_TO, &e->env->to, win, row, edata->to_rows);
    row += draw_envelope_addr(HDR_CC, &e->env->cc, win, row, edata->cc_rows);
    row += draw_envelope_addr(HDR_BCC, &e->env->bcc, win, row, edata->bcc_rows);
  }

  draw_header(win, row++, HDR_SUBJECT);
  mutt_paddstr(win, cols, NONULL(e->env->subject));

  row += draw_envelope_addr(HDR_REPLYTO, &e->env->reply_to, win, row, 1);

  draw_header(win, row++, HDR_FCC);
  mutt_paddstr(win, cols, fcc);

  if (WithCrypto)
    row += draw_crypt_lines(win, shared, edata, row);

  const bool c_compose_show_user_headers =
      cs_subset_bool(shared->sub, "compose_show_user_headers");
  if (c_compose_show_user_headers)
    row += draw_envelope_user_hdrs(win, shared, row);

  mutt_curses_set_color_by_id(MT_COLOR_STATUS);
  mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
}

/**
 * env_recalc - Recalculate the Window data - Implements MuttWindow::recalc() - @ingroup window_recalc
 */
static int env_recalc(struct MuttWindow *win)
{
  struct ComposeSharedData *shared = win->parent->wdata;
  struct ComposeEnvelopeData *edata = win->wdata;

  const int cur_rows = win->state.rows;
  const int new_rows = calc_envelope(win, shared, edata);

  if (new_rows != cur_rows)
  {
    win->req_rows = new_rows;
    mutt_window_reflow(win->parent);
  }

  win->actions |= WA_REPAINT;
  mutt_debug(LL_DEBUG5, "recalc done, request WA_REPAINT\n");
  return 0;
}

/**
 * env_repaint - Repaint the Window - Implements MuttWindow::repaint() - @ingroup window_repaint
 */
static int env_repaint(struct MuttWindow *win)
{
  if (!mutt_window_is_visible(win))
    return 0;

  struct ComposeSharedData *shared = win->parent->wdata;
  struct ComposeEnvelopeData *edata = win->wdata;

  draw_envelope(win, shared, edata);
  mutt_debug(LL_DEBUG5, "repaint done\n");
  return 0;
}

/**
 * env_color_observer - Notification that a Color has changed - Implements ::observer_t - @ingroup observer_api
 */
static int env_color_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_COLOR) || !nc->global_data || !nc->event_data)
    return -1;

  struct EventColor *ev_c = nc->event_data;
  struct MuttWindow *win_env = nc->global_data;

  enum ColorId cid = ev_c->cid;

  switch (cid)
  {
    case MT_COLOR_BOLD:
    case MT_COLOR_COMPOSE_HEADER:
    case MT_COLOR_COMPOSE_SECURITY_BOTH:
    case MT_COLOR_COMPOSE_SECURITY_ENCRYPT:
    case MT_COLOR_COMPOSE_SECURITY_NONE:
    case MT_COLOR_COMPOSE_SECURITY_SIGN:
    case MT_COLOR_NORMAL:
    case MT_COLOR_STATUS:
    case MT_COLOR_MAX: // Sent on `uncolor *`
      mutt_debug(LL_DEBUG5, "color done, request WA_REPAINT\n");
      win_env->actions |= WA_REPAINT;
      break;

    default:
      break;
  }
  return 0;
}

/**
 * env_compose_observer - Notification that the Compose data has changed - Implements ::observer_t - @ingroup observer_api
 */
static int env_compose_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_COMPOSE) || !nc->global_data)
    return -1;

  if (nc->event_subtype != NT_COMPOSE_ENVELOPE)
    return 0;

  struct MuttWindow *win_env = nc->global_data;

  win_env->actions |= WA_RECALC;
  mutt_debug(LL_DEBUG5, "compose done, request WA_RECALC\n");

  return 0;
}

/**
 * env_config_observer - Notification that a Config Variable has changed - Implements ::observer_t - @ingroup observer_api
 */
static int env_config_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_CONFIG) || !nc->global_data || !nc->event_data)
    return -1;

  struct EventConfig *ev_c = nc->event_data;
  struct MuttWindow *win_env = nc->global_data;

  switch (ev_c->name[0])
  {
    case 'a':
      if (mutt_str_equal(ev_c->name, "autocrypt"))
        break;
      return 0;
    case 'c':
      if (mutt_str_equal(ev_c->name, "compose_show_user_headers") ||
          mutt_str_equal(ev_c->name, "crypt_opportunistic_encrypt"))
      {
        break;
      }
      return 0;
    case 'p':
      if (mutt_str_equal(ev_c->name, "pgp_sign_as"))
        break;
      return 0;
    case 's':
      if (mutt_str_equal(ev_c->name, "smime_encrypt_with"))
        break;
      return 0;
    case 'x':
      if (mutt_str_equal(ev_c->name, "x_comment_to"))
        break;
      return 0;
    default:
      return 0;
  }

  win_env->actions |= WA_RECALC;
  mutt_debug(LL_DEBUG5, "config done, request WA_RECALC\n");
  return 0;
}

/**
 * env_header_observer - Notification that a User Header has changed - Implements ::observer_t - @ingroup observer_api
 */
static int env_header_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_HEADER) || !nc->global_data || !nc->event_data)
    return -1;

  const struct EventHeader *ev_h = nc->event_data;
  struct MuttWindow *win_env = nc->global_data;
  // struct ComposeEnvelopeData *edata = win_env->wdata;
  struct MuttWindow *dlg = win_env->parent;
  struct ComposeSharedData *shared = dlg->wdata;

  struct Envelope *env = shared->email->env;

  if ((nc->event_subtype == NT_HEADER_ADD) || (nc->event_subtype == NT_HEADER_CHANGE))
  {
    header_set(&env->userhdrs, ev_h->header);
    mutt_debug(LL_DEBUG5, "header done, request reflow\n");
    env_recalc(win_env);
    return 0;
  }

  if (nc->event_subtype == NT_HEADER_DELETE)
  {
    struct ListNode *removed = header_find(&env->userhdrs, ev_h->header);
    if (removed)
    {
      header_free(&env->userhdrs, removed);
      mutt_debug(LL_DEBUG5, "header done, request reflow\n");
      env_recalc(win_env);
    }
    return 0;
  }

  return 0;
}

/**
 * env_window_observer - Notification that a Window has changed - Implements ::observer_t - @ingroup observer_api
 */
static int env_window_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_WINDOW) || !nc->global_data || !nc->event_data)
    return -1;

  struct MuttWindow *win_env = nc->global_data;
  struct EventWindow *ev_w = nc->event_data;
  if (ev_w->win != win_env)
    return 0;

  if (nc->event_subtype == NT_WINDOW_STATE)
  {
    win_env->actions |= WA_RECALC;
    mutt_debug(LL_DEBUG5, "window state done, request WA_RECALC\n");
  }
  else if (nc->event_subtype == NT_WINDOW_DELETE)
  {
    struct ComposeSharedData *shared = win_env->parent->wdata;

    notify_observer_remove(NeoMutt->notify, env_color_observer, win_env);
    notify_observer_remove(shared->notify, env_compose_observer, win_env);
    notify_observer_remove(NeoMutt->notify, env_config_observer, win_env);
    notify_observer_remove(NeoMutt->notify, env_header_observer, win_env);
    notify_observer_remove(win_env->notify, env_window_observer, win_env);
    mutt_debug(LL_DEBUG5, "window delete done\n");
  }

  return 0;
}

/**
 * compose_env_new - Create the Envelope Window
 * @param shared Shared compose data
 * @param fcc    Buffer to save FCC
 * @retval ptr New Window
 */
struct MuttWindow *compose_env_new(struct ComposeSharedData *shared, struct Buffer *fcc)
{
  struct MuttWindow *win_env =
      mutt_window_new(WT_CUSTOM, MUTT_WIN_ORIENT_VERTICAL, MUTT_WIN_SIZE_FIXED,
                      MUTT_WIN_SIZE_UNLIMITED, HDR_ATTACH_TITLE - 1);

  notify_observer_add(NeoMutt->notify, NT_COLOR, env_color_observer, win_env);
  notify_observer_add(shared->notify, NT_COMPOSE, env_compose_observer, win_env);
  notify_observer_add(NeoMutt->notify, NT_CONFIG, env_config_observer, win_env);
  notify_observer_add(NeoMutt->notify, NT_HEADER, env_header_observer, win_env);
  notify_observer_add(win_env->notify, NT_WINDOW, env_window_observer, win_env);

  struct ComposeEnvelopeData *env_data = env_data_new();
  env_data->fcc = fcc;

  shared->edata = env_data;

  win_env->wdata = env_data;
  win_env->wdata_free = env_data_free;
  win_env->recalc = env_recalc;
  win_env->repaint = env_repaint;

  return win_env;
}
