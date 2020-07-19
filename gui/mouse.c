/**
 * @file
 * Mouse handling
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
 * @page gui_mouse Mouse handling
 *
 * Mouse handling
 */

#include "config.h"
#include <stdbool.h>
#include "mutt/lib.h"
#include "mouse.h"
#include "mutt_curses.h"
#include "mutt_window.h"

/**
 * in_window - XXX
 */
static bool in_window(struct MuttWindow *win, int col, int row)
{
  if (!win)
    return false;
  if (!win->state.visible)
    return false;
  if ((col < win->state.col_offset) || (col >= (win->state.col_offset + win->state.cols)))
    return false;
  if ((row < win->state.row_offset) || (row >= (win->state.row_offset + win->state.rows)))
    return false;
  return true;
}

/**
 * window_by_posn - XXX
 */
static struct MuttWindow *window_by_posn(struct MuttWindow *win, int col, int row)
{
  if (!in_window(win, col, row))
    return NULL;

  struct MuttWindow *np = NULL;
  TAILQ_FOREACH(np, &win->children, entries)
  {
    if (in_window(np, col, row))
      return window_by_posn(np, col, row);
  }

  return win;
}

/**
 * mouse_handle_event - Process mouse events
 * @param ch Character, from getch()
 * @retval true If it is a mouse Event
 */
bool mouse_handle_event(int ch)
{
  if (ch != KEY_MOUSE)
    return false;

  if (!RootWindow)
    return true;

  MEVENT event;
  if (getmouse(&event) != OK)
    return true;

  struct MuttWindow *win = window_by_posn(RootWindow, event.x, event.y);

  for (; win; win = win->parent)
  {
    if (win->mouse)
    {
      struct EventMouse em = { MUTT_MOUSE_NO_FLAGS, MB_BUTTON1, ME_CLICK, event.x - win->state.col_offset, event.y - win->state.row_offset };
      if (win->mouse(win, &em))
        return true;
    }
  }

  // Not handled
  return true;
}
