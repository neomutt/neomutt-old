/**
 * @file
 * GUI miscellaneous curses (window drawing) routines
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_CURS_LIB_H
#define MUTT_CURS_LIB_H

#include <stdbool.h>
#include <wchar.h> // IWYU pragma: keep
#include "mutt.h"
#include "browser/lib.h"
#include "keymap.h"

struct Buffer;
struct Mailbox;
struct MuttWindow;

extern int MuttGetchTimeout; ///< Timeout in ms for mutt_getch()

/**
 * enum FormatJustify - Alignment for mutt_simple_format()
 */
enum FormatJustify
{
  JUSTIFY_LEFT = -1,  ///< Left justify the text
  JUSTIFY_CENTER = 0, ///< Centre the text
  JUSTIFY_RIGHT = 1,  ///< Right justify the text
};

int          mutt_addwch(struct MuttWindow *win, wchar_t wc);
int          mutt_any_key_to_continue(const char *s);
void         mutt_beep(bool force);
int          mutt_buffer_enter_fname(const char *prompt, struct Buffer *fname, bool mailbox, struct Mailbox *m, bool multiple, char ***files, int *numfiles, SelectFileFlags flags);
int          mutt_buffer_get_field(const char *field, struct Buffer *buf, CompletionFlags complete, bool multiple, struct Mailbox *m, char ***files, int *numfiles);
void         mutt_edit_file(const char *editor, const char *file);
void         mutt_endwin(void);
void         mutt_flushinp(void);
void         mutt_flush_macro_to_endcond(void);
void         mutt_flush_unget_to_endcond(void);
void         mutt_format_s(char *buf, size_t buflen, const char *prec, const char *s);
void         mutt_format_s_tree(char *buf, size_t buflen, const char *prec, const char *s);
void         mutt_format_s_x(char *buf, size_t buflen, const char *prec, const char *s, bool arboreal);
void         mutt_getch_timeout(int delay);
struct KeyEvent mutt_getch(void);
int          mutt_get_field_unbuffered(const char *msg, struct Buffer *buf, CompletionFlags flags);
void         mutt_need_hard_redraw(void);
void         mutt_paddstr(struct MuttWindow *win, int n, const char *s);
void         mutt_perror_debug(const char *s);
void         mutt_push_macro_event(int ch, int op);
void         mutt_query_exit(void);
void         mutt_refresh(void);
void         mutt_simple_format(char *buf, size_t buflen, int min_width, int max_width, enum FormatJustify justify, char pad_char, const char *s, size_t n, bool arboreal);
int          mutt_strwidth(const char *s);
int          mutt_strnwidth(const char *s, size_t len);
void         mutt_unget_event(int ch, int op);
void         mutt_unget_string(const char *s);
size_t       mutt_wstr_trunc(const char *src, size_t maxlen, size_t maxwid, size_t *width);
bool         keyboard_buffer_is_empty(void);

#endif /* MUTT_CURS_LIB_H */
