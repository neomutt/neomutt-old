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

#include <stdbool.h>
#include <stdint.h>

#ifndef MUTT_MOUSE_H
#define MUTT_MOUSE_H

typedef uint8_t MuttMouseFlags;    ///< Flags mouse events, e.g. #MUTT_MOUSE_CTRL
#define MUTT_MOUSE_NO_FLAGS     0  ///< No flags are set
#define MUTT_MOUSE_CTRL   (1 << 0) ///< Ctrl-Mouse
#define MUTT_MOUSE_SHIFT  (1 << 1) ///< Shift-Mouse
#define MUTT_MOUSE_ALT    (1 << 2) ///< Alt-Mouse

/**
 * MuttMouseButton - Which mouse button?
 */
enum MuttMouseButton
{
  MB_BUTTON1 = 1, ///< Mouse Button 1
  MB_BUTTON2 = 2, ///< Mouse Button 2
  MB_BUTTON3 = 3, ///< Mouse Button 3
  MB_BUTTON4 = 4, ///< Mouse Button 4
  MB_BUTTON5 = 5, ///< Mouse Button 5
};

/**
 * MuttMouseEvent - How many clicks?
 */
enum MuttMouseEvent
{
  ME_CLICK,   ///< Single mouse click
  ME_PRESS,   ///< Mouse button held down
  ME_RELEASE, ///< Mouse button released
  ME_DOUBLE,  ///< Double mouse click
  ME_TRIPLE,  ///< Triple mouse click
};

struct EventMouse
{
  MuttMouseFlags       flags;  ///< XXX
  enum MuttMouseButton button; ///< XXX
  enum MuttMouseEvent  event;  ///< XXX
  int                  col;    ///< XXX
  int                  row;    ///< XXX
};

bool mouse_handle_event(int ch);

#endif /* MUTT_MOUSE_H */
