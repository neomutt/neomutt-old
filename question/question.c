/**
 * @file
 * Ask the user a question
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
 * @page question_question Ask the user a question
 *
 * Ask the user a question
 */

#include "config.h"
#include <ctype.h>
#include <langinfo.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "gui/lib.h"
#include "color/lib.h"
#include "globals.h"
#include "keymap.h"
#include "mutt_logging.h"
#include "opcodes.h"
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/**
 * mutt_multi_choice - Offer the user a multiple choice question
 * @param prompt  Message prompt
 * @param letters Allowable selection keys
 * @retval >=1 1-based user selection
 * @retval  -1 Selection aborted
 */
int mutt_multi_choice(const char *prompt, const char *letters)
{
  struct MuttWindow *win = msgwin_get_window();
  if (!win)
    return -1;

  struct KeyEvent ch = { OP_NULL, OP_NULL };
  int choice;
  bool redraw = true;
  int prompt_lines = 1;

  struct AttrColor *ac_opts = NULL;
  if (simple_color_is_set(MT_COLOR_OPTIONS))
  {
    struct AttrColor *ac_base = simple_color_get(MT_COLOR_NORMAL);
    ac_base = merged_color_overlay(ac_base, simple_color_get(MT_COLOR_PROMPT));

    ac_opts = simple_color_get(MT_COLOR_OPTIONS);
    ac_opts = merged_color_overlay(ac_base, ac_opts);
  }

  struct MuttWindow *old_focus = window_set_focus(win);
  enum MuttCursorState cursor = mutt_curses_set_cursor(MUTT_CURSOR_VISIBLE);
  window_redraw(NULL);
  while (true)
  {
    if (redraw || SigWinch)
    {
      redraw = false;
      if (SigWinch)
      {
        SigWinch = false;
        mutt_resize_screen();
        clearok(stdscr, true);
        window_redraw(NULL);
      }
      if (win->state.cols)
      {
        int width = mutt_strwidth(prompt) + 2; // + '?' + space
        /* If we're going to colour the options,
         * make an assumption about the modified prompt size. */
        if (ac_opts)
          width -= 2 * mutt_str_len(letters);

        prompt_lines = (width + win->state.cols - 1) / win->state.cols;
        prompt_lines = MAX(1, MIN(3, prompt_lines));
      }
      if (prompt_lines != win->state.rows)
      {
        msgwin_set_height(prompt_lines);
        window_redraw(NULL);
      }

      mutt_window_move(win, 0, 0);

      if (ac_opts)
      {
        char *cur = NULL;

        while ((cur = strchr(prompt, '(')))
        {
          // write the part between prompt and cur using MT_COLOR_PROMPT
          mutt_curses_set_normal_backed_color_by_id(MT_COLOR_PROMPT);
          mutt_window_addnstr(win, prompt, cur - prompt);

          if (isalnum(cur[1]) && (cur[2] == ')'))
          {
            // we have a single letter within parentheses
            mutt_curses_set_color(ac_opts);
            mutt_window_addch(win, cur[1]);
            prompt = cur + 3;
          }
          else
          {
            // we have a parenthesis followed by something else
            mutt_window_addch(win, cur[0]);
            prompt = cur + 1;
          }
        }
      }

      mutt_curses_set_normal_backed_color_by_id(MT_COLOR_PROMPT);
      mutt_window_addstr(win, prompt);
      mutt_curses_set_color_by_id(MT_COLOR_NORMAL);

      mutt_window_addch(win, ' ');
      mutt_window_clrtoeol(win);
    }

    mutt_refresh();
    /* SigWinch is not processed unless timeout is set */
    ch = mutt_getch_timeout(30 * 1000);
    if (ch.op == OP_TIMEOUT)
      continue;
    if (ch.op == OP_ABORT || CI_is_return(ch.ch))
    {
      choice = -1;
      break;
    }
    else
    {
      char *p = strchr(letters, ch.ch);
      if (p)
      {
        choice = p - letters + 1;
        break;
      }
      else if ((ch.ch <= '9') && (ch.ch > '0'))
      {
        choice = ch.ch - '0';
        if (choice <= mutt_str_len(letters))
          break;
      }
    }
    mutt_beep(false);
  }

  if (win->state.rows == 1)
  {
    mutt_window_clearline(win, 0);
  }
  else
  {
    msgwin_set_height(1);
    window_redraw(NULL);
  }

  mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
  window_set_focus(old_focus);
  mutt_curses_set_cursor(cursor);
  mutt_refresh();
  return choice;
}

/**
 * mutt_yesorno - Ask the user a Yes/No question
 * @param msg Prompt
 * @param def Default answer, #MUTT_YES or #MUTT_NO (see #QuadOption)
 * @retval num Selection made, see #QuadOption
 */
enum QuadOption mutt_yesorno(const char *msg, enum QuadOption def)
{
  struct MuttWindow *win = msgwin_get_window();
  if (!win)
    return MUTT_ABORT;

  struct KeyEvent ch = { OP_NULL, OP_NULL };
  char *answer_string = NULL;
  int answer_string_wid, msg_wid;
  size_t trunc_msg_len;
  bool redraw = true;
  int prompt_lines = 1;
  char answer[7] = { 0 };
  char *panswer = NULL;
  char *codeset = NULL;
  bool input_is_utf = false;

  char *yes = N_("yes");
  char *no = N_("no");
  char *trans_yes = _(yes);
  char *trans_no = _(no);

  regex_t reyes = { 0 };
  regex_t reno = { 0 };

  bool reyes_ok = false;
  bool reno_ok = false;

#ifdef OpenBSD
  /* OpenBSD only supports locale C and UTF-8
   * so there is no suitable base system's locale identification
   * Remove this code immediately if this situation changes! */
  char rexyes[16] = "^[+1YyYy]";
  rexyes[6] = toupper(trans_yes[0]);
  rexyes[7] = tolower(trans_yes[0]);

  char rexno[16] = "^[-0NnNn]";
  rexno[6] = toupper(trans_no[0]);
  rexno[7] = tolower(trans_no[0]);

  if (REG_COMP(&reyes, rexyes, REG_NOSUB) == 0)
    reyes_ok = true;

  if (REG_COMP(&reno, rexno, REG_NOSUB) == 0)
    reno_ok = true;

#else
  char *expr = NULL;
  reyes_ok = (expr = nl_langinfo(YESEXPR)) && (expr[0] == '^') &&
             (REG_COMP(&reyes, expr, REG_NOSUB) == 0);
  reno_ok = (expr = nl_langinfo(NOEXPR)) && (expr[0] == '^') &&
            (REG_COMP(&reno, expr, REG_NOSUB) == 0);
#endif

  if ((yes != trans_yes) && (no != trans_no) && reyes_ok && reno_ok)
  {
    // If all parts of the translation succeeded...
    yes = trans_yes;
    no = trans_no;
  }
  else
  {
    // otherwise, fallback to English
    if (reyes_ok)
    {
      regfree(&reyes);
      reyes_ok = false;
    }
    if (reno_ok)
    {
      regfree(&reno);
      reno_ok = false;
    }
  }

  /* In order to prevent the default answer to the question to wrapped
   * around the screen in the event the question is wider than the screen,
   * ensure there is enough room for the answer and truncate the question
   * to fit.  */
  mutt_str_asprintf(&answer_string, " ([%s]/%s): ", (def == MUTT_YES) ? yes : no,
                    (def == MUTT_YES) ? no : yes);
  answer_string_wid = mutt_strwidth(answer_string);
  msg_wid = mutt_strwidth(msg);

  struct MuttWindow *old_focus = window_set_focus(win);

  enum MuttCursorState cursor = mutt_curses_set_cursor(MUTT_CURSOR_VISIBLE);
  window_redraw(NULL);
  while (true)
  {
    if (redraw || SigWinch)
    {
      redraw = false;
      if (SigWinch)
      {
        SigWinch = false;
        mutt_resize_screen();
        clearok(stdscr, true);
        window_redraw(NULL);
      }
      if (win->state.cols)
      {
        prompt_lines = (msg_wid + answer_string_wid + win->state.cols - 1) /
                       win->state.cols;
        prompt_lines = MAX(1, MIN(3, prompt_lines));
      }
      if (prompt_lines != win->state.rows)
      {
        msgwin_set_height(prompt_lines);
        window_redraw(NULL);
      }

      /* maxlen here is sort of arbitrary, so pick a reasonable upper bound */
      trunc_msg_len = mutt_wstr_trunc(msg,
                                      (size_t) 4 * prompt_lines * win->state.cols,
                                      ((size_t) prompt_lines * win->state.cols) - answer_string_wid,
                                      NULL);

      mutt_window_move(win, 0, 0);
      mutt_curses_set_normal_backed_color_by_id(MT_COLOR_PROMPT);
      mutt_window_addnstr(win, msg, trunc_msg_len);
      mutt_window_addstr(win, answer_string);
      mutt_curses_set_color_by_id(MT_COLOR_NORMAL);
      mutt_window_clrtoeol(win);
    }

    mutt_refresh();
    /* SigWinch is not processed unless timeout is set */
    ch = mutt_getch_timeout(30 * 1000);
    if (ch.op == OP_TIMEOUT)
      continue;
    if (CI_is_return(ch.ch))
      break;
    if (ch.op == OP_ABORT)
    {
      def = MUTT_ABORT;
      break;
    }

    codeset = nl_langinfo(CODESET);
    input_is_utf = codeset && !strcmp(codeset, "UTF-8");

    if (input_is_utf)
    {
      panswer = answer;
      *panswer = ch.ch;
      int times_to_read = mutt_mb_charlen_first((unsigned char) *panswer);
      panswer++;
      for (int i = 1; i < times_to_read; i++)
      {
        ch = mutt_getch_timeout(0);
        *panswer++ = ch.ch;
      }
      *panswer = 0;
    }
    else
    {
      answer[0] = ch.ch;
      answer[1] = 0;
    }

    if (reyes_ok ? (regexec(&reyes, answer, 0, 0, 0) == 0) : (tolower(ch.ch) == 'y'))
    {
      def = MUTT_YES;
      break;
    }
    else if (reno_ok ? (regexec(&reno, answer, 0, 0, 0) == 0) : (tolower(ch.ch) == 'n'))
    {
      def = MUTT_NO;
      break;
    }
    else
    {
      mutt_beep(false);
    }
  }
  window_set_focus(old_focus);
  mutt_curses_set_cursor(cursor);

  FREE(&answer_string);

  if (reyes_ok)
    regfree(&reyes);
  if (reno_ok)
    regfree(&reno);

  if (win->state.rows == 1)
  {
    mutt_window_clearline(win, 0);
  }
  else
  {
    msgwin_set_height(1);
    window_redraw(NULL);
  }

  if (def == MUTT_ABORT)
  {
    /* when the users cancels with ^G, clear the message stored with
     * mutt_message() so it isn't displayed when the screen is refreshed. */
    mutt_clear_error();
  }
  else
  {
    mutt_window_addstr(win, (char *) ((def == MUTT_YES) ? yes : no));
    mutt_refresh();
  }
  return def;
}

/**
 * query_quadoption - Ask the user a quad-question
 * @param opt    Option to use
 * @param prompt Message to show to the user
 * @retval #QuadOption Result, e.g. #MUTT_NO
 */
enum QuadOption query_quadoption(enum QuadOption opt, const char *prompt)
{
  switch (opt)
  {
    case MUTT_YES:
    case MUTT_NO:
      return opt;

    default:
      opt = mutt_yesorno(prompt, (opt == MUTT_ASKYES) ? MUTT_YES : MUTT_NO);
      msgwin_clear_text();
      return opt;
  }

  /* not reached */
}
