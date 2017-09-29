/**
 * @file
 * API for the notifications infrastructure
 *
 * @authors
 * Copyright (C) 2017 Pietro Cerutti <gahr@gahr.ch>
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

#ifndef _MUTT_NOTIFICATIONS_H
#define _MUTT_NOTIFICATIONS_H

/**
 * mutt_notifications_show - Show the notifications screen, if there is any
 */
void mutt_notifications_show(void);

/**
 * mutt_notifications_add - Add a new notification
 * @param[in] s String to display in the notification screen
 *
 * Notifications are kept in a unique list, so adding the same string multiple
 * times has no effect.
 */
void mutt_notifications_add(const char *s);

/**
 * mutt_has_notifications - Check if there are notifications
 * @return true if there are notifications
 */
bool mutt_has_notifications(void);

#endif /* !_MUTT_NOTIFICATIONS_H */
