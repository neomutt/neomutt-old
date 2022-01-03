/**
 * @file
 * Automate the testing of the Compose Dialog
 *
 * @authors
 * Copyright (C) 2018-2020 Richard Russon <rich@flatcap.org>
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
 * @page debug_compose Compose testing
 *
 * Automate the testing of the Compose Dialog
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "email/lib.h"
#include "gui/lib.h"
#include "lib.h"
#include "keymap.h"

void push_actions(struct Buffer *actions)
{
  if (mutt_buffer_is_empty(actions))
    return;

  struct Buffer *tmp = mutt_buffer_pool_get();

  mutt_buffer_seek(actions, 0);
  mutt_debug(LL_DEBUG1, "ACTION: %s\n", mutt_buffer_string(actions));
  mutt_parse_push(tmp, actions, 0, NULL);

  mutt_buffer_pool_release(&tmp);
}

void add_random_file(struct Buffer *actions)
{
  int letter = (rand() % 26) + 'A';

  mutt_buffer_add_printf(actions, "<attach-file>attachments/%c.txt<enter>", letter);
}

void select_attachment(struct Buffer *actions, int index)
{
  mutt_buffer_add_printf(actions, "%d<enter>", index);
}

void select_random_attachment(struct Buffer *actions, struct AttachCtx *actx)
{
  int index = (rand() % actx->idxlen) + 1;
  select_attachment(actions, index);
}

void tag_attachment(struct Buffer *actions)
{
  mutt_buffer_add_printf(actions, "<tag-entry>");
}

void tag_random_attachment(struct Buffer *actions, struct AttachCtx *actx)
{
  select_random_attachment(actions, actx);
  tag_attachment(actions);
}

void group_attachments(struct Buffer *actions)
{
  mutt_buffer_add_printf(actions, "<group-alternatives>");
}

void group_random_attachments(struct Buffer *actions, struct AttachCtx *actx)
{
  int num_group = (rand() % 4) + 2;

  for (int i = 0; i < num_group; i++)
  {
    tag_random_attachment(actions, actx);
  }
  group_attachments(actions);
}

void untag_all(struct Buffer *actions)
{
  mutt_buffer_add_printf(actions, "<tag-prefix-cond><tag-entry><end-cond>");
}

void move_attachment(struct Buffer *actions, struct AttachCtx *actx)
{
  select_random_attachment(actions, actx);

  const int places = (rand() % 4) + 1;
  const char *direction = (rand() % 2) ? "<move-up>" : "<move-down>";

  for (int i = 0; i < places; i++)
  {
    mutt_buffer_addstr(actions, direction);
  }
}

void ungroup_attachment(struct Buffer *actions, struct AttachCtx *actx)
{
  select_random_attachment(actions, actx);
  mutt_buffer_addstr(actions, "<ungroup-attachment>");
}

void delete_random_attachment(struct Buffer *actions, struct AttachCtx *actx)
{
  select_random_attachment(actions, actx);
  mutt_buffer_addstr(actions, "<detach-file>");
}

void compose_automate(struct AttachCtx *actx, int *action_num)
{
  static int runs = 0;
  // mutt_debug(LL_DEBUG1, "ACTION: runs=%d, action_num=%d\n", runs, *action_num);

  if (!actx)
    return;
  struct Buffer *actions = NULL;

  if (!keyboard_buffer_is_empty())
  {
    // mutt_debug(LL_DEBUG1, "ACTION: buffer not empty %d\n", *action_num);
    return;
  }

  (*action_num)++;

  actions = mutt_buffer_pool_get();

  if (*action_num > 100)
  {
    runs++;
    mutt_debug(LL_DEBUG1, "ACTION: runs=%d, action_num=%d\n", runs, *action_num);
    const int c_compose_autorun =
        cs_subset_number(NeoMutt->sub, "compose_autorun");
    if ((c_compose_autorun == 0) || (runs < c_compose_autorun))
    {
      // Restart Compose
      // mutt_buffer_add_printf(actions, "<exit><mail>john.doe@example.com<enter>test %d<enter>", runs);
      mutt_buffer_add_printf(actions, "<exit><F1>", runs);
    }
    else
    {
      // Quit NeoMutt
      mutt_buffer_addstr(actions, "<exit><exit>");
    }
    goto done;
  }

  if (*action_num == 1)
  {
    const int num = (rand() % 35) + 6;
    for (int i = 0; i < num; i++)
    {
      add_random_file(actions);
      push_actions(actions);
      mutt_buffer_reset(actions);
    }
    goto done;
  }

  switch (rand() % 10)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      group_random_attachments(actions, actx);
      untag_all(actions);
      break;

    case 4:
    case 5:
      move_attachment(actions, actx);
      break;

    case 7:
    case 6:
      ungroup_attachment(actions, actx);
      break;

    case 8:
      delete_random_attachment(actions, actx);
      break;

    case 9:
      add_random_file(actions);
  }

done:
  push_actions(actions);
  mutt_buffer_pool_release(&actions);
}

char body_name(const struct Body *b)
{
  if (!b)
    return '!';

  if (b->type == TYPE_MULTIPART)
    return '&';

  if (b->description)
    return b->description[0];

  if (b->filename)
  {
    const char *base = basename(b->filename);
    if (mutt_str_startswith(base, "neomutt-"))
      return '0';

    return base[0];
  }

  return '!';
}

void dump_body_next(struct Buffer *buf, const struct Body *b)
{
  if (!b)
    return;

  mutt_buffer_addstr(buf, "<");
  for (; b; b = b->next)
  {
    mutt_buffer_add_printf(buf, "%c", body_name(b));
    dump_body_next(buf, b->parts);
    if (b->next)
      mutt_buffer_addch(buf, ',');
  }
  mutt_buffer_addstr(buf, ">");
}

void dump_body_one_line(const struct Body *b)
{
  if (!b)
    return;

  struct Buffer *buf = mutt_buffer_pool_get();
  mutt_buffer_addstr(buf, "Body layout: ");
  dump_body_next(buf, b);

  mutt_message(mutt_buffer_string(buf));
  mutt_buffer_pool_release(&buf);
}
