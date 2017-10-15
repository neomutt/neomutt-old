/**
 * @file
 * GUI display the mailboxes in a side panel
 *
 * @authors
 * Copyright (C) 2004 Justin Hibbits <jrh29@po.cwru.edu>
 * Copyright (C) 2004 Thomer M. Gil <mutt@thomer.com>
 * Copyright (C) 2015-2016 Richard Russon <rich@flatcap.org>
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

#ifndef _MUTT_SIDEBAR_H
#define _MUTT_SIDEBAR_H

struct Context;
struct Buffy;

void mutt_sb_change_mailbox(int op);
void mutt_sb_draw(void);
const char *mutt_sb_get_highlight(void);
void mutt_sb_notify_mailbox(struct Buffy *b, int created);
void mutt_sb_set_buffystats(const struct Context *ctx);
void mutt_sb_set_open_buffy(void);
void mutt_sb_toggle_virtual(void);
struct Buffy *mutt_sb_start_search(void);

#endif /* _MUTT_SIDEBAR_H */
