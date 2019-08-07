/**
 * @file
 * Routines for querying and external address book
 *
 * @authors
 * Copyright (C) 1996-2000,2003,2013 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
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
 * @page query Routines for querying and external address book
 *
 * Routines for querying and external address book
 */

#include "config.h"
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "mutt/mutt.h"
#include "address/lib.h"
#include "email/lib.h"
#include "mutt.h"
#include "alias.h"
#include "curs_lib.h"
#include "filter.h"
#include "format_flags.h"
#include "globals.h"
#include "keymap.h"
#include "mutt_logging.h"
#include "mutt_menu.h"
#include "mutt_window.h"
#include "muttlib.h"
#include "opcodes.h"
#include "send.h"
#include "rxi-vec/vec.h"

/* These Config Variables are only used in query.c */
char *C_QueryCommand; ///< Config: External command to query and external address book
char *C_QueryFormat; ///< Config: printf-like format string for the query menu (address book)

/**
 * struct Query - An entry in the menu, populated by an external address-book
 */
struct Query
{
  bool tagged;
  int num;
  struct AddressList addr;
  char *name;
  char *other;
};

typedef vec_t(struct Query*) QueryVector;


static const struct Mapping QueryHelp[] = {
  { N_("Exit"), OP_EXIT },
  { N_("Mail"), OP_MAIL },
  { N_("New Query"), OP_QUERY },
  { N_("Make Alias"), OP_CREATE_ALIAS },
  { N_("Search"), OP_SEARCH },
  { N_("Help"), OP_HELP },
  { NULL, 0 },
};

/**
 * result_to_addr - Turn a Query into an AddressList
 * @param al AddressList to fill (must be empty)
 * @param r Query to use
 * @retval bool True on success
 */
static bool result_to_addr(struct AddressList *al, struct Query *r)
{
  if (!al || !TAILQ_EMPTY(al) || !r)
    return false;

  mutt_addrlist_copy(al, &r->addr, false);
  if (!TAILQ_EMPTY(al))
  {
    struct Address *first = TAILQ_FIRST(al);
    struct Address *second = TAILQ_NEXT(first, entries);
    if (!second && !first->personal)
      first->personal = mutt_str_strdup(r->name);

    mutt_addrlist_to_intl(al, NULL);
  }

  return true;
}

/**
 * query_new - Create a new query
 * @retval A newly allocated query
 */
static struct Query *query_new(void)
{
  struct Query *query = mutt_mem_calloc(1, sizeof(struct Query));
  TAILQ_INIT(&query->addr);
  return query;
}

/**
 * query_free - Free one query
 * @param[out] qp Query to free
 */
static void query_free(struct Query **qp)
{
  if (!qp || !*qp)
    return;

  struct Query *q = *qp;
  mutt_addrlist_clear(&q->addr);
  FREE(&q->name);
  FREE(&q->other);
  FREE(qp);
}

/**
 * query_free - Free all elements of a QueryVector an reinitialize it
 * @param[out] vector QueryVector to free
 */
static void query_freevector(QueryVector *vector)
{
  struct Query *q;
  int iter;
  vec_foreach(vector, q, iter)
  {
    query_free(&q);
  }
  vec_deinit(vector);
}

/**
 * run_query - Run an external program to find Addresses
 * @param queries QueryVector where results are appended
 * @param s      String to match
 * @param quiet  If true, don't print progress messages
 */
static void run_query(QueryVector *queries, char *s, int quiet)
{
  FILE *fp = NULL;
  char *buf = NULL;
  size_t buflen;
  int dummy = 0;
  char msg[256];
  char *p = NULL;
  pid_t pid;
  struct Buffer *cmd = mutt_buffer_pool_get();

  mutt_buffer_file_expand_fmt_quote(cmd, C_QueryCommand, s);

  pid = mutt_create_filter(mutt_b2s(cmd), NULL, &fp, NULL);
  if (pid < 0)
  {
    mutt_debug(LL_DEBUG1, "unable to fork command: %s\n", mutt_b2s(cmd));
    mutt_buffer_pool_release(&cmd);
    return;
  }
  mutt_buffer_pool_release(&cmd);

  if (!quiet)
    mutt_message(_("Waiting for response..."));
  fgets(msg, sizeof(msg), fp);
  p = strrchr(msg, '\n');
  if (p)
    *p = '\0';
  while ((buf = mutt_file_read_line(buf, &buflen, fp, &dummy, 0)))
  {
    p = strtok(buf, "\t\n");
    if (p)
    {
      struct Query *q = query_new();
      mutt_addrlist_parse(&q->addr, p);
      p = strtok(NULL, "\t\n");
      if (p)
      {
        q->name = mutt_str_strdup(p);
        p = strtok(NULL, "\t\n");
        if (p)
          q->other = mutt_str_strdup(p);
      }
      vec_push(queries, q);
    }
  }
  FREE(&buf);
  mutt_file_fclose(&fp);
  if (mutt_wait_filter(pid))
  {
    mutt_debug(LL_DEBUG1, "Error: %s\n", msg);
    if (!quiet)
      mutt_error("%s", msg);
  }
  else
  {
    if (!quiet)
      mutt_message("%s", msg);
  }
}

/**
 * query_search - Search a Address menu item - Implements Menu::menu_search()
 *
 * Try to match various Address fields.
 */
static int query_search(struct Menu *menu, regex_t *rx, int line)
{
  struct Query *query = ((QueryVector *)menu->data)->data[line];

  if (query->name && !regexec(rx, query->name, 0, NULL, 0))
    return 0;
  if (query->other && !regexec(rx, query->other, 0, NULL, 0))
    return 0;
  if (!TAILQ_EMPTY(&query->addr))
  {
    struct Address *addr = TAILQ_FIRST(&query->addr);
    if (addr->personal && !regexec(rx, addr->personal, 0, NULL, 0))
    {
      return 0;
    }
    if (addr->mailbox && !regexec(rx, addr->mailbox, 0, NULL, 0))
    {
      return 0;
    }
  }

  return REG_NOMATCH;
}

/**
 * query_format_str - Format a string for the query menu - Implements ::format_t
 *
 * | Expando | Description
 * |:--------|:--------------------------------------------------------
 * | \%a     | Destination address
 * | \%c     | Current entry number
 * | \%e     | Extra information
 * | \%n     | Destination name
 * | \%t     | `*` if current entry is tagged, a space otherwise
 */
static const char *query_format_str(char *buf, size_t buflen, size_t col, int cols,
                                    char op, const char *src, const char *prec,
                                    const char *if_str, const char *else_str,
                                    unsigned long data, MuttFormatFlags flags)
{
  struct Query *query = (struct Query *) data;
  char fmt[128];
  char tmp[256] = { 0 };
  bool optional = (flags & MUTT_FORMAT_OPTIONAL);

  switch (op)
  {
    case 'a':
      mutt_addrlist_write(tmp, sizeof(tmp), &query->addr, true);
      mutt_format_s(buf, buflen, prec, tmp);
      break;
    case 'c':
      snprintf(fmt, sizeof(fmt), "%%%sd", prec);
      snprintf(buf, buflen, fmt, query->num + 1);
      break;
    case 'e':
      if (!optional)
        mutt_format_s(buf, buflen, prec, NONULL(query->other));
      else if (!query->other || !*query->other)
        optional = false;
      break;
    case 'n':
      mutt_format_s(buf, buflen, prec, NONULL(query->name));
      break;
    case 't':
      snprintf(fmt, sizeof(fmt), "%%%sc", prec);
      snprintf(buf, buflen, fmt, query->tagged ? '*' : ' ');
      break;
    default:
      snprintf(fmt, sizeof(fmt), "%%%sc", prec);
      snprintf(buf, buflen, fmt, op);
      break;
  }

  if (optional)
    mutt_expando_format(buf, buflen, col, cols, if_str, query_format_str, data,
                        MUTT_FORMAT_NO_FLAGS);
  else if (flags & MUTT_FORMAT_OPTIONAL)
    mutt_expando_format(buf, buflen, col, cols, else_str, query_format_str,
                        data, MUTT_FORMAT_NO_FLAGS);

  return src;
}

/**
 * query_make_entry - Format a menu item for the query list - Implements Menu::menu_make_entry()
 */
static void query_make_entry(char *buf, size_t buflen, struct Menu *menu, int line)
{
  struct Query *query = ((QueryVector*) menu->data)->data[line];

  query->num = line;
  mutt_expando_format(buf, buflen, 0, menu->indexwin->cols, NONULL(C_QueryFormat),
                      query_format_str, (unsigned long) query, MUTT_FORMAT_ARROWCURSOR);
}

/**
 * query_tag - Tag an entry in the Query Menu - Implements Menu::menu_tag()
 */
static int query_tag(struct Menu *menu, int sel, int act)
{
  struct Query *q = ((QueryVector *) menu->data)->data[sel];
  bool ot = q->tagged;

  q->tagged = ((act >= 0) ? act : !q->tagged);
  return q->tagged - ot;
}

/**
 * query_menu - Get the user to enter an Address Query
 * @param buf     Buffer for the query
 * @param buflen  Length of buffer
 * @param queries QueryVector
 * @param retbuf  If true, populate the results
 */
static void query_menu(char *buf, size_t buflen, QueryVector *queries, bool retbuf)
{
  struct Menu *menu = NULL;
  char title[256];

  if (queries->length == 0)
  {
    /* Prompt for Query */
    if ((mutt_get_field(_("Query: "), buf, buflen, 0) == 0) && (buf[0] != '\0'))
    {
      run_query(queries, buf, 0);
    }
  }

  if (queries->length == 0)
    return;

  snprintf(title, sizeof(title), _("Query '%s'"), buf);

  menu = mutt_menu_new(MENU_QUERY);
  menu->menu_make_entry = query_make_entry;
  menu->menu_search = query_search;
  menu->menu_tag = query_tag;
  menu->title = title;
  menu->data = queries;
  menu->max = queries->length;
  char helpstr[1024];
  menu->help = mutt_compile_help(helpstr, sizeof(helpstr), MENU_QUERY, QueryHelp);
  mutt_menu_push_current(menu);

  int done = 0;
  while (done == 0)
  {
    const int op = mutt_menu_loop(menu);
    switch (op)
    {
      case OP_QUERY_APPEND:
      case OP_QUERY:
        if ((mutt_get_field(_("Query: "), buf, buflen, 0) == 0) && (buf[0] != '\0'))
        {
          int prev_length = queries->length;
          run_query(queries, buf, 0);

          menu->redraw = REDRAW_FULL;
          if (queries->length != prev_length)
          {
            snprintf(title, sizeof(title), _("Query '%s'"), buf);

            if (op == OP_QUERY)
            {
              /* remove the old items */
              for (size_t i = 0; i < prev_length; ++i)
              {
                query_free(&queries->data[i]);
              }
              vec_splice(queries, 0, prev_length);
            }
            else
            {
              /* append - run_query appends by default, so nothing to do*/
            }

            menu->max = queries->length;
          }
        }
        break;

      case OP_CREATE_ALIAS:
        if (menu->tagprefix)
        {
          struct AddressList naddr = TAILQ_HEAD_INITIALIZER(naddr);

          for (int i = 0; i < menu->max; i++)
          {
            if (queries->data[i]->tagged)
            {
              struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
              if (result_to_addr(&al, queries->data[i]))
              {
                mutt_addrlist_copy(&naddr, &al, false);
                mutt_addrlist_clear(&al);
              }
            }
          }

          mutt_alias_create(NULL, &naddr);
          mutt_addrlist_clear(&naddr);
        }
        else
        {
          struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
          if (result_to_addr(&al, queries->data[menu->current]))
          {
            mutt_alias_create(NULL, &al);
            mutt_addrlist_clear(&al);
          }
        }
        break;

      case OP_GENERIC_SELECT_ENTRY:
        if (retbuf)
        {
          done = 2;
          break;
        }
      /* fallthrough */
      case OP_MAIL:
      {
        struct Email *e = email_new();
        e->env = mutt_env_new();
        if (!menu->tagprefix)
        {
          struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
          if (result_to_addr(&al, queries->data[menu->current]))
          {
            mutt_addrlist_copy(&e->env->to, &al, false);
            mutt_addrlist_clear(&al);
          }
        }
        else
        {
          for (int i = 0; i < menu->max; i++)
          {
            if (queries->data[i]->tagged)
            {
              struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
              if (result_to_addr(&al, queries->data[i]))
              {
                mutt_addrlist_copy(&e->env->to, &al, false);
                mutt_addrlist_clear(&al);
              }
            }
          }
        }
        ci_send_message(SEND_NO_FLAGS, e, NULL, Context, NULL);
        menu->redraw = REDRAW_FULL;
        break;
      }

      case OP_EXIT:
        done = 1;
        break;
    }
  }

  /* if we need to return the selected entries */
  if (retbuf && (done == 2))
  {
    bool tagged = false;
    size_t curpos = 0;

    memset(buf, 0, buflen);

    /* check for tagged entries */
    for (int i = 0; i < menu->max; i++)
    {
      if (queries->data[i]->tagged)
      {
        if (curpos == 0)
        {
          struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
          if (result_to_addr(&al, queries->data[i]))
          {
            mutt_addrlist_to_local(&al);
            tagged = true;
            mutt_addrlist_write(buf, buflen, &al, false);
            curpos = mutt_str_strlen(buf);
            mutt_addrlist_clear(&al);
          }
        }
        else if (curpos + 2 < buflen)
        {
          struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
          if (result_to_addr(&al, queries->data[i]))
          {
            mutt_addrlist_to_local(&al);
            strcat(buf, ", ");
            mutt_addrlist_write(buf + curpos + 1, buflen - curpos - 1, &al, false);
            curpos = mutt_str_strlen(buf);
            mutt_addrlist_clear(&al);
          }
        }
      }
    }
    /* then enter current message */
    if (!tagged)
    {
      struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
      if (result_to_addr(&al, queries->data[menu->current]))
      {
        mutt_addrlist_to_local(&al);
        mutt_addrlist_write(buf, buflen, &al, false);
        mutt_addrlist_clear(&al);
      }
    }
  }

  query_freevector(queries);
  mutt_menu_pop_current(menu);
  mutt_menu_free(&menu);
}

/**
 * mutt_query_complete - Perform auto-complete using an Address Query
 * @param buf    Buffer for completion
 * @param buflen Length of buffer
 * @retval 0 Always
 */
int mutt_query_complete(char *buf, size_t buflen)
{
  if (!C_QueryCommand)
  {
    mutt_error(_("Query command not defined"));
    return 0;
  }

  QueryVector queries;
  vec_init(&queries);
  run_query(&queries, buf, 1);
  if (queries.length)
  {
    /* only one response? */
    if (queries.length == 1)
    {
      struct AddressList al = TAILQ_HEAD_INITIALIZER(al);
      if (result_to_addr(&al, queries.data[0]))
      {
        mutt_addrlist_to_local(&al);
        buf[0] = '\0';
        mutt_addrlist_write(buf, buflen, &al, false);
        mutt_addrlist_clear(&al);
        query_freevector(&queries);
        mutt_clear_error();
      }
      return 0;
    }
    /* multiple results, choose from query menu */
    query_menu(buf, buflen, &queries, true);
  }
  return 0;
}

/**
 * mutt_query_menu - Show the user the results of a Query
 * @param buf    Buffer for the query
 * @param buflen Length of buffer
 */
void mutt_query_menu(char *buf, size_t buflen)
{
  if (!C_QueryCommand)
  {
    mutt_error(_("Query command not defined"));
    return;
  }

  char tmp[256] = { 0 };
  if (!buf)
  {
    buf = tmp;
    buflen = sizeof(tmp);
  }

  QueryVector queries;
  vec_init(&queries);
  query_menu(buf, buflen, &queries, true);
}
