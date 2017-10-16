/**
 * @file
 * Config/command parsing
 *
 * @authors
 * Copyright (C) 1996-2002,2010,2013,2016 Michael R. Elkins <me@mutt.org>
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

#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <wchar.h>
#include "lib/lib.h"
#include "mutt.h"
#include "init.h"
#include "address.h"
#include "alias.h"
#include "charset.h"
#include "context.h"
#include "envelope.h"
#include "filter.h"
#include "group.h"
#include "hcache/hcache.h"
#include "header.h"
#include "history.h"
#include "keymap.h"
#include "list.h"
#include "mailbox.h"
#include "mbtable.h"
#include "mbyte.h"
#include "mutt_curses.h"
#include "mutt_idna.h"
#include "mutt_menu.h"
#include "mutt_regex.h"
#include "mx.h"
#include "myvar.h"
#include "ncrypt/ncrypt.h"
#include "options.h"
#include "pattern.h"
#include "rfc822.h"
#include "sidebar.h"
#include "version.h"
#ifdef USE_NOTMUCH
#include "mutt_notmuch.h"
#endif
#ifdef USE_IMAP
#include "imap/imap.h" /* for imap_subscribe() */
#endif

#define CHECK_PAGER                                                                  \
  if ((CurrentMenu == MENU_PAGER) && (idx >= 0) && (MuttVars[idx].flags & R_RESORT)) \
  {                                                                                  \
    snprintf(err->data, err->dsize, "%s", _("Not available in this menu."));         \
    return -1;                                                                       \
  }

/**
 * struct MyVar - A user-set variable
 */
struct MyVar
{
  char *name;
  char *value;
  struct MyVar *next;
};

static struct MyVar *MyVars;

static void myvar_set(const char *var, const char *val)
{
  struct MyVar **cur = NULL;

  for (cur = &MyVars; *cur; cur = &((*cur)->next))
    if (mutt_strcmp((*cur)->name, var) == 0)
      break;

  if (!*cur)
    *cur = safe_calloc(1, sizeof(struct MyVar));

  if (!(*cur)->name)
    (*cur)->name = safe_strdup(var);

  mutt_str_replace(&(*cur)->value, val);
}

static void myvar_del(const char *var)
{
  struct MyVar **cur = NULL;
  struct MyVar *tmp = NULL;

  for (cur = &MyVars; *cur; cur = &((*cur)->next))
    if (mutt_strcmp((*cur)->name, var) == 0)
      break;

  if (*cur)
  {
    tmp = (*cur)->next;
    FREE(&(*cur)->name);
    FREE(&(*cur)->value);
    FREE(cur);
    *cur = tmp;
  }
}

#ifdef USE_NOTMUCH
/* List of tags found in last call to mutt_nm_query_complete(). */
static char **nm_tags;
#endif

extern char **envlist;

static void toggle_quadoption(int opt)
{
  int n = opt / 4;
  int b = (opt % 4) * 2;

  QuadOptions[n] ^= (1 << b);
}

static int parse_regex(int idx, struct Buffer *tmp, struct Buffer *err)
{
  int e, flags = 0;
  const char *p = NULL;
  regex_t *rx = NULL;
  struct Regex *ptr = (struct Regex *) MuttVars[idx].data;

  if (!ptr->pattern || (mutt_strcmp(ptr->pattern, tmp->data) != 0))
  {
    bool not = false;

    /* $mask is case-sensitive */
    if (mutt_strcmp(MuttVars[idx].option, "mask") != 0)
      flags |= mutt_which_case(tmp->data);

    p = tmp->data;
    if (mutt_strcmp(MuttVars[idx].option, "mask") == 0)
    {
      if (*p == '!')
      {
        not = true;
        p++;
      }
    }

    rx = safe_malloc(sizeof(regex_t));
    e = REGCOMP(rx, p, flags);
    if (e != 0)
    {
      regerror(e, rx, err->data, err->dsize);
      FREE(&rx);
      return 0;
    }

    /* get here only if everything went smoothly */
    if (ptr->pattern)
    {
      FREE(&ptr->pattern);
      regfree((regex_t *) ptr->regex);
      FREE(&ptr->regex);
    }

    ptr->pattern = safe_strdup(tmp->data);
    ptr->regex = rx;
    ptr->not = not;

    return 1;
  }
  return 0;
}

void set_quadoption(int opt, int flag)
{
  int n = opt / 4;
  int b = (opt % 4) * 2;

  QuadOptions[n] &= ~(0x3 << b);
  QuadOptions[n] |= (flag & 0x3) << b;
}

int quadoption(int opt)
{
  int n = opt / 4;
  int b = (opt % 4) * 2;

  return (QuadOptions[n] >> b) & 0x3;
}

int query_quadoption(int opt, const char *prompt)
{
  int v = quadoption(opt);

  switch (v)
  {
    case MUTT_YES:
    case MUTT_NO:
      return v;

    default:
      v = mutt_yesorno(prompt, (v == MUTT_ASKYES));
      mutt_window_clearline(MuttMessageWindow, 0);
      return v;
  }

  /* not reached */
}

/**
 * mutt_option_index - Find the index (in rc_vars) of a variable name
 * @param s Variable name to search for
 * @retval -1 on error
 * @retval >0 on success
 */
int mutt_option_index(const char *s)
{
  for (int i = 0; MuttVars[i].option; i++)
    if (mutt_strcmp(s, MuttVars[i].option) == 0)
      return (MuttVars[i].type == DT_SYNONYM ?
                  mutt_option_index((char *) MuttVars[i].data) :
                  i);
  return -1;
}

#ifdef USE_LUA
int mutt_option_to_string(const struct Option *opt, char *val, size_t len)
{
  mutt_debug(2, " * mutt_option_to_string(%s)\n", NONULL((char *) opt->data));
  int idx = mutt_option_index((const char *) opt->option);
  if (idx != -1)
    return var_to_string(idx, val, len);
  return 0;
}

bool mutt_option_get(const char *s, struct Option *opt)
{
  mutt_debug(2, " * mutt_option_get(%s)\n", s);
  int idx = mutt_option_index(s);
  if (idx != -1)
  {
    if (opt)
      *opt = MuttVars[idx];
    return true;
  }

  if (mutt_strncmp("my_", s, 3) == 0)
  {
    const char *mv = myvar_get(s);
    if (!mv)
      return false;

    if (opt)
    {
      memset(opt, 0, sizeof(*opt));
      opt->option = s;
      opt->type = DT_STRING;
    }
    return true;
  }
  return false;
}
#endif

static void free_mbtable(struct MbTable **t)
{
  if (!t || !*t)
    return;

  FREE(&(*t)->chars);
  FREE(&(*t)->segmented_str);
  FREE(&(*t)->orig_str);
  FREE(t);
}

static struct MbTable *parse_mbtable(const char *s)
{
  struct MbTable *t = NULL;
  size_t slen, k;
  mbstate_t mbstate;
  char *d = NULL;

  t = safe_calloc(1, sizeof(struct MbTable));
  slen = mutt_strlen(s);
  if (!slen)
    return t;

  t->orig_str = safe_strdup(s);
  /* This could be more space efficient.  However, being used on tiny
   * strings (ToChars and StatusChars), the overhead is not great. */
  t->chars = safe_calloc(slen, sizeof(char *));
  d = t->segmented_str = safe_calloc(slen * 2, sizeof(char));

  memset(&mbstate, 0, sizeof(mbstate));
  while (slen && (k = mbrtowc(NULL, s, slen, &mbstate)))
  {
    if (k == (size_t)(-1) || k == (size_t)(-2))
    {
      mutt_debug(1, "parse_mbtable: mbrtowc returned %d converting %s in %s\n",
                 (k == (size_t)(-1)) ? -1 : -2, s, t->orig_str);
      if (k == (size_t)(-1))
        memset(&mbstate, 0, sizeof(mbstate));
      k = (k == (size_t)(-1)) ? 1 : slen;
    }

    slen -= k;
    t->chars[t->len++] = d;
    while (k--)
      *d++ = *s++;
    *d++ = '\0';
  }

  return t;
}

static int parse_sort(short *val, const char *s, const struct Mapping *map, struct Buffer *err)
{
  int i, flags = 0;

  if (mutt_strncmp("reverse-", s, 8) == 0)
  {
    s += 8;
    flags = SORT_REVERSE;
  }

  if (mutt_strncmp("last-", s, 5) == 0)
  {
    s += 5;
    flags |= SORT_LAST;
  }

  i = mutt_getvaluebyname(s, map);
  if (i == -1)
  {
    snprintf(err->data, err->dsize, _("%s: unknown sorting method"), s);
    return -1;
  }

  *val = i | flags;

  return 0;
}

#ifdef USE_LUA
int mutt_option_set(const struct Option *val, struct Buffer *err)
{
  mutt_debug(2, " * mutt_option_set()\n");
  int idx = mutt_option_index(val->option);
  if (idx != -1)
  {
    switch (DTYPE(MuttVars[idx].type))
    {
      case DT_REGEX:
      {
        char err_str[LONG_STRING] = "";
        struct Buffer err2;
        err2.data = err_str;
        err2.dsize = sizeof(err_str);

        struct Buffer tmp;
        tmp.data = (char *) val->data;
        tmp.dsize = strlen((char *) val->data);

        if (parse_regex(idx, &tmp, &err2))
        {
          /* $reply_regexp and $alternates require special treatment */
          if (Context && Context->msgcount &&
              (mutt_strcmp(MuttVars[idx].option, "reply_regexp") == 0))
          {
            regmatch_t pmatch[1];

            for (int i = 0; i < Context->msgcount; i++)
            {
              struct Envelope *e = Context->hdrs[i]->env;
              if (e && e->subject)
              {
                e->real_subj =
                    (regexec(ReplyRegexp.regex, e->subject, 1, pmatch, 0)) ?
                        e->subject :
                        e->subject + pmatch[0].rm_eo;
              }
            }
          }
        }
        else
        {
          snprintf(err->data, err->dsize, _("%s: Unknown type."), MuttVars[idx].option);
          return -1;
        }
        break;
      }
      case DT_SORT:
      {
        const struct Mapping *map = NULL;

        switch (MuttVars[idx].type & DT_SUBTYPE_MASK)
        {
          case DT_SORT_ALIAS:
            map = SortAliasMethods;
            break;
          case DT_SORT_BROWSER:
            map = SortBrowserMethods;
            break;
          case DT_SORT_KEYS:
            if ((WithCrypto & APPLICATION_PGP))
              map = SortKeyMethods;
            break;
          case DT_SORT_AUX:
            map = SortAuxMethods;
            break;
          case DT_SORT_SIDEBAR:
            map = SortSidebarMethods;
            break;
          default:
            map = SortMethods;
            break;
        }

        if (!map)
        {
          snprintf(err->data, err->dsize, _("%s: Unknown type."), MuttVars[idx].option);
          return -1;
        }

        if (parse_sort((short *) MuttVars[idx].data, (const char *) val->data, map, err) == -1)
        {
          return -1;
        }
      }
      break;
      case DT_MBTABLE:
      {
        struct MbTable **tbl = (struct MbTable **) MuttVars[idx].data;
        free_mbtable(tbl);
        *tbl = parse_mbtable((const char *) val->data);
      }
      break;
      case DT_ADDRESS:
        rfc822_free_address((struct Address **) MuttVars[idx].data);
        *((struct Address **) MuttVars[idx].data) =
            rfc822_parse_adrlist(NULL, (const char *) val->data);
        break;
      case DT_PATH:
      {
        char scratch[LONG_STRING];
        strfcpy(scratch, NONULL((const char *) val->data), sizeof(scratch));
        mutt_expand_path(scratch, sizeof(scratch));
        /* MuttVars[idx].data is already 'char**' (or some 'void**') or...
        * so cast to 'void*' is okay */
        FREE((void *) MuttVars[idx].data);
        *((char **) MuttVars[idx].data) = safe_strdup(scratch);
        break;
      }
      case DT_STRING:
      {
        /* MuttVars[idx].data is already 'char**' (or some 'void**') or...
          * so cast to 'void*' is okay */
        FREE((void *) MuttVars[idx].data);
        *((char **) MuttVars[idx].data) = safe_strdup((char *) val->data);
      }
      break;
      case DT_BOOL:
        if (val->data)
          set_option(MuttVars[idx].data);
        else
          unset_option(MuttVars[idx].data);
        break;
      case DT_QUAD:
        set_quadoption(MuttVars[idx].data, val->data);
        break;
      case DT_NUMBER:
        *((short *) MuttVars[idx].data) = val->data;
        break;
      default:
        return -1;
    }
  }
  /* set the string as a myvar if it's one */
  if (mutt_strncmp("my_", val->option, 3) == 0)
  {
    myvar_set(val->option, (const char *) val->data);
  }
  return 0;
}
#endif

int mutt_extract_token(struct Buffer *dest, struct Buffer *tok, int flags)
{
  if (!dest || !tok)
    return -1;

  char ch;
  char qc = 0; /* quote char */
  char *pc = NULL;

  /* reset the destination pointer to the beginning of the buffer */
  dest->dptr = dest->data;

  SKIPWS(tok->dptr);
  while ((ch = *tok->dptr))
  {
    if (!qc)
    {
      if ((ISSPACE(ch) && !(flags & MUTT_TOKEN_SPACE)) ||
          (ch == '#' && !(flags & MUTT_TOKEN_COMMENT)) ||
          (ch == '=' && (flags & MUTT_TOKEN_EQUAL)) ||
          (ch == ';' && !(flags & MUTT_TOKEN_SEMICOLON)) ||
          ((flags & MUTT_TOKEN_PATTERN) && strchr("~%=!|", ch)))
        break;
    }

    tok->dptr++;

    if (ch == qc)
      qc = 0; /* end of quote */
    else if (!qc && (ch == '\'' || ch == '"') && !(flags & MUTT_TOKEN_QUOTE))
      qc = ch;
    else if (ch == '\\' && qc != '\'')
    {
      if (!*tok->dptr)
        return -1; /* premature end of token */
      switch (ch = *tok->dptr++)
      {
        case 'c':
        case 'C':
          if (!*tok->dptr)
            return -1; /* premature end of token */
          mutt_buffer_addch(dest, (toupper((unsigned char) *tok->dptr) - '@') & 0x7f);
          tok->dptr++;
          break;
        case 'r':
          mutt_buffer_addch(dest, '\r');
          break;
        case 'n':
          mutt_buffer_addch(dest, '\n');
          break;
        case 't':
          mutt_buffer_addch(dest, '\t');
          break;
        case 'f':
          mutt_buffer_addch(dest, '\f');
          break;
        case 'e':
          mutt_buffer_addch(dest, '\033');
          break;
        default:
          if (isdigit((unsigned char) ch) && isdigit((unsigned char) *tok->dptr) &&
              isdigit((unsigned char) *(tok->dptr + 1)))
          {
            mutt_buffer_addch(dest, (ch << 6) + (*tok->dptr << 3) + *(tok->dptr + 1) - 3504);
            tok->dptr += 2;
          }
          else
            mutt_buffer_addch(dest, ch);
      }
    }
    else if (ch == '^' && (flags & MUTT_TOKEN_CONDENSE))
    {
      if (!*tok->dptr)
        return -1; /* premature end of token */
      ch = *tok->dptr++;
      if (ch == '^')
        mutt_buffer_addch(dest, ch);
      else if (ch == '[')
        mutt_buffer_addch(dest, '\033');
      else if (isalpha((unsigned char) ch))
        mutt_buffer_addch(dest, toupper((unsigned char) ch) - '@');
      else
      {
        mutt_buffer_addch(dest, '^');
        mutt_buffer_addch(dest, ch);
      }
    }
    else if (ch == '`' && (!qc || qc == '"'))
    {
      FILE *fp = NULL;
      pid_t pid;
      char *cmd = NULL, *ptr = NULL;
      size_t expnlen;
      struct Buffer expn;
      int line = 0;

      pc = tok->dptr;
      do
      {
        if ((pc = strpbrk(pc, "\\`")))
        {
          /* skip any quoted chars */
          if (*pc == '\\')
            pc += 2;
        }
      } while (pc && *pc != '`');
      if (!pc)
      {
        mutt_debug(1, "mutt_get_token: mismatched backticks\n");
        return -1;
      }
      cmd = mutt_substrdup(tok->dptr, pc);
      if ((pid = mutt_create_filter(cmd, NULL, &fp, NULL)) < 0)
      {
        mutt_debug(1, "mutt_get_token: unable to fork command: %s\n", cmd);
        FREE(&cmd);
        return -1;
      }
      FREE(&cmd);

      tok->dptr = pc + 1;

      /* read line */
      mutt_buffer_init(&expn);
      expn.data = mutt_read_line(NULL, &expn.dsize, fp, &line, 0);
      safe_fclose(&fp);
      mutt_wait_filter(pid);

      /* if we got output, make a new string consisting of the shell output
         plus whatever else was left on the original line */
      /* BUT: If this is inside a quoted string, directly add output to
       * the token */
      if (expn.data && qc)
      {
        mutt_buffer_addstr(dest, expn.data);
        FREE(&expn.data);
      }
      else if (expn.data)
      {
        expnlen = mutt_strlen(expn.data);
        tok->dsize = expnlen + mutt_strlen(tok->dptr) + 1;
        ptr = safe_malloc(tok->dsize);
        memcpy(ptr, expn.data, expnlen);
        strcpy(ptr + expnlen, tok->dptr);
        if (tok->destroy)
          FREE(&tok->data);
        tok->data = ptr;
        tok->dptr = ptr;
        tok->destroy = 1; /* mark that the caller should destroy this data */
        ptr = NULL;
        FREE(&expn.data);
      }
    }
    else if (ch == '$' && (!qc || qc == '"') &&
             (*tok->dptr == '{' || isalpha((unsigned char) *tok->dptr)))
    {
      const char *env = NULL;
      char *var = NULL;
      int idx;

      if (*tok->dptr == '{')
      {
        tok->dptr++;
        if ((pc = strchr(tok->dptr, '}')))
        {
          var = mutt_substrdup(tok->dptr, pc);
          tok->dptr = pc + 1;
        }
      }
      else
      {
        for (pc = tok->dptr; isalnum((unsigned char) *pc) || *pc == '_'; pc++)
          ;
        var = mutt_substrdup(tok->dptr, pc);
        tok->dptr = pc;
      }
      if (var)
      {
        if ((env = getenv(var)) || (env = myvar_get(var)))
          mutt_buffer_addstr(dest, env);
        else if ((idx = mutt_option_index(var)) != -1)
        {
          /* expand settable neomutt variables */
          char val[LONG_STRING];

          if (var_to_string(idx, val, sizeof(val)))
            mutt_buffer_addstr(dest, val);
        }
        FREE(&var);
      }
    }
    else
      mutt_buffer_addch(dest, ch);
  }
  mutt_buffer_addch(dest, 0); /* terminate the string */
  SKIPWS(tok->dptr);
  return 0;
}

static void free_opt(struct Option *p)
{
  struct Regex *pp = NULL;

  switch (DTYPE(p->type))
  {
    case DT_ADDRESS:
      rfc822_free_address((struct Address **) p->data);
      break;
    case DT_REGEX:
      pp = (struct Regex *) p->data;
      FREE(&pp->pattern);
      if (pp->regex)
      {
        regfree(pp->regex);
        FREE(&pp->regex);
      }
      break;
    case DT_PATH:
    case DT_STRING:
      FREE((char **) p->data);
      break;
  }
}

/**
 * mutt_free_opts - clean up before quitting
 */
void mutt_free_opts(void)
{
  for (int i = 0; MuttVars[i].option; i++)
    free_opt(MuttVars + i);

  mutt_free_regex_list(&Alternates);
  mutt_free_regex_list(&UnAlternates);
  mutt_free_regex_list(&MailLists);
  mutt_free_regex_list(&UnMailLists);
  mutt_free_regex_list(&SubscribedLists);
  mutt_free_regex_list(&UnSubscribedLists);
  mutt_free_regex_list(&NoSpamList);
}

static void add_to_stailq(struct ListHead *head, const char *str)
{
  /* don't add a NULL or empty string to the list */
  if (!str || *str == '\0')
    return;

  /* check to make sure the item is not already on this list */
  struct ListNode *np;
  STAILQ_FOREACH(np, head, entries)
  {
    if (mutt_strcasecmp(str, np->data) == 0)
    {
      return;
    }
  }
  mutt_list_insert_tail(head, safe_strdup(str));
}

static struct RegexList *new_regex_list(void)
{
  return safe_calloc(1, sizeof(struct RegexList));
}

int mutt_add_to_regex_list(struct RegexList **list, const char *s, int flags,
                           struct Buffer *err)
{
  struct RegexList *t = NULL, *last = NULL;
  struct Regex *rx = NULL;

  if (!s || !*s)
    return 0;

  rx = mutt_compile_regex(s, flags);
  if (!rx)
  {
    snprintf(err->data, err->dsize, "Bad regex: %s\n", s);
    return -1;
  }

  /* check to make sure the item is not already on this list */
  for (last = *list; last; last = last->next)
  {
    if (mutt_strcasecmp(rx->pattern, last->regex->pattern) == 0)
    {
      /* already on the list, so just ignore it */
      last = NULL;
      break;
    }
    if (!last->next)
      break;
  }

  if (!*list || last)
  {
    t = new_regex_list();
    t->regex = rx;
    if (last)
    {
      last->next = t;
      last = last->next;
    }
    else
      *list = last = t;
  }
  else /* duplicate */
    mutt_free_regex(&rx);

  return 0;
}

static int remove_from_replace_list(struct ReplaceList **list, const char *pat)
{
  struct ReplaceList *cur = NULL, *prev = NULL;
  int nremoved = 0;

  /* Being first is a special case. */
  cur = *list;
  if (!cur)
    return 0;
  if (cur->regex && (mutt_strcmp(cur->regex->pattern, pat) == 0))
  {
    *list = cur->next;
    mutt_free_regex(&cur->regex);
    FREE(&cur->template);
    FREE(&cur);
    return 1;
  }

  prev = cur;
  for (cur = prev->next; cur;)
  {
    if (mutt_strcmp(cur->regex->pattern, pat) == 0)
    {
      prev->next = cur->next;
      mutt_free_regex(&cur->regex);
      FREE(&cur->template);
      FREE(&cur);
      cur = prev->next;
      nremoved++;
    }
    else
      cur = cur->next;
  }

  return nremoved;
}

static struct ReplaceList *new_replace_list(void)
{
  return safe_calloc(1, sizeof(struct ReplaceList));
}

static int add_to_replace_list(struct ReplaceList **list, const char *pat,
                               const char *templ, struct Buffer *err)
{
  struct ReplaceList *t = NULL, *last = NULL;
  struct Regex *rx = NULL;
  int n;
  const char *p = NULL;

  if (!pat || !*pat || !templ)
    return 0;

  rx = mutt_compile_regex(pat, REG_ICASE);
  if (!rx)
  {
    snprintf(err->data, err->dsize, _("Bad regex: %s"), pat);
    return -1;
  }

  /* check to make sure the item is not already on this list */
  for (last = *list; last; last = last->next)
  {
    if (mutt_strcasecmp(rx->pattern, last->regex->pattern) == 0)
    {
      /* Already on the list. Formerly we just skipped this case, but
       * now we're supporting removals, which means we're supporting
       * re-adds conceptually. So we probably want this to imply a
       * removal, then do an add. We can achieve the removal by freeing
       * the template, and leaving t pointed at the current item.
       */
      t = last;
      FREE(&t->template);
      break;
    }
    if (!last->next)
      break;
  }

  /* If t is set, it's pointing into an extant ReplaceList* that we want to
   * update. Otherwise we want to make a new one to link at the list's end.
   */
  if (!t)
  {
    t = new_replace_list();
    t->regex = rx;
    rx = NULL;
    if (last)
      last->next = t;
    else
      *list = t;
  }
  else
    mutt_free_regex(&rx);

  /* Now t is the ReplaceList* that we want to modify. It is prepared. */
  t->template = safe_strdup(templ);

  /* Find highest match number in template string */
  t->nmatch = 0;
  for (p = templ; *p;)
  {
    if (*p == '%')
    {
      n = atoi(++p);
      if (n > t->nmatch)
        t->nmatch = n;
      while (*p && isdigit((int) *p))
        p++;
    }
    else
      p++;
  }

  if (t->nmatch > t->regex->regex->re_nsub)
  {
    snprintf(err->data, err->dsize, "%s", _("Not enough subexpressions for "
                                            "template"));
    remove_from_replace_list(list, pat);
    return -1;
  }

  t->nmatch++; /* match 0 is always the whole expr */

  return 0;
}

/**
 * finish_source - 'finish' command: stop processing current config file
 * @param tmp  Temporary space shared by all command handlers
 * @param s    Current line of the config file
 * @param data data field from init.h:struct Command
 * @param err  Buffer for any error message
 * @retval  1 Stop processing the current file
 * @retval -1 Failed
 *
 * If the 'finish' command is found, we should stop reading the current file.
 */
static int finish_source(struct Buffer *tmp, struct Buffer *s,
                         unsigned long data, struct Buffer *err)
{
  if (MoreArgs(s))
  {
    snprintf(err->data, err->dsize, _("finish: too many arguments"));
    return -1;
  }

  return 1;
}

/**
 * parse_ifdef - 'ifdef' command: conditional config
 * @param tmp  Temporary space shared by all command handlers
 * @param s    Current line of the config file
 * @param data data field from init.h:struct Command
 * @param err  Buffer for any error message
 * @retval  0 Success
 * @retval -1 Failed
 *
 * The 'ifdef' command allows conditional elements in the config file.
 * If a given variable, function, command or compile-time symbol exists, then
 * read the rest of the line of config commands.
 * e.g.
 *      ifdef sidebar source ~/.neomutt/sidebar.rc
 *
 * If (data == 1) then it means use the 'ifndef' (if-not-defined) command.
 * e.g.
 *      ifndef imap finish
 */
static int parse_ifdef(struct Buffer *tmp, struct Buffer *s, unsigned long data,
                       struct Buffer *err)
{
  bool res = 0;
  struct Buffer token;

  memset(&token, 0, sizeof(token));
  mutt_extract_token(tmp, s, 0);

  /* is the item defined as a variable? */
  res = (mutt_option_index(tmp->data) != -1);

  /* is the item a compiled-in feature? */
  if (!res)
  {
    res = feature_enabled(tmp->data);
  }

  /* or a function? */
  if (!res)
  {
    for (int i = 0; !res && (i < MENU_MAX); i++)
    {
      const struct Binding *b = km_get_table(Menus[i].value);
      if (!b)
        continue;

      for (int j = 0; b[j].name; j++)
      {
        if (mutt_strcmp(tmp->data, b[j].name) == 0)
        {
          res = true;
          break;
        }
      }
    }
  }

  /* or a command? */
  if (!res)
  {
    for (int i = 0; Commands[i].name; i++)
    {
      if (mutt_strcmp(tmp->data, Commands[i].name) == 0)
      {
        res = true;
        break;
      }
    }
  }

  if (!MoreArgs(s))
  {
    snprintf(err->data, err->dsize, _("%s: too few arguments"),
             (data ? "ifndef" : "ifdef"));
    return -1;
  }
  mutt_extract_token(tmp, s, MUTT_TOKEN_SPACE);

  /* ifdef KNOWN_SYMBOL or ifndef UNKNOWN_SYMBOL */
  if ((res && (data == 0)) || (!res && (data == 1)))
  {
    int rc = mutt_parse_rc_line(tmp->data, &token, err);
    if (rc == -1)
    {
      mutt_error("Error: %s", err->data);
      FREE(&token.data);
      return -1;
    }
    FREE(&token.data);
    return rc;
  }
  return 0;
}

static void remove_from_stailq(struct ListHead *head, const char *str)
{
  if (mutt_strcmp("*", str) == 0)
    mutt_list_free(head); /* ``unCMD *'' means delete all current entries */
  else
  {
    struct ListNode *np, *tmp;
    STAILQ_FOREACH_SAFE(np, head, entries, tmp)
    {
      if (mutt_strcasecmp(str, np->data) == 0)
      {
        STAILQ_REMOVE(head, np, ListNode, entries);
        FREE(&np->data);
        FREE(&np);
        break;
      }
    }
  }
}

static int parse_unignore(struct Buffer *buf, struct Buffer *s,
                          unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);

    /* don't add "*" to the unignore list */
    if (strcmp(buf->data, "*") != 0)
      add_to_stailq(&UnIgnore, buf->data);

    remove_from_stailq(&Ignore, buf->data);
  } while (MoreArgs(s));

  return 0;
}

static int parse_ignore(struct Buffer *buf, struct Buffer *s,
                        unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);
    remove_from_stailq(&UnIgnore, buf->data);
    add_to_stailq(&Ignore, buf->data);
  } while (MoreArgs(s));

  return 0;
}

static int parse_stailq(struct Buffer *buf, struct Buffer *s,
                        unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);
    add_to_stailq((struct ListHead *) data, buf->data);
  } while (MoreArgs(s));

  return 0;
}

static int parse_unstailq(struct Buffer *buf, struct Buffer *s,
                          unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);
    /*
     * Check for deletion of entire list
     */
    if (mutt_strcmp(buf->data, "*") == 0)
    {
      mutt_list_free((struct ListHead *) data);
      break;
    }
    remove_from_stailq((struct ListHead *) data, buf->data);
  } while (MoreArgs(s));

  return 0;
}

static void _alternates_clean(void)
{
  if (Context && Context->msgcount)
  {
    for (int i = 0; i < Context->msgcount; i++)
      Context->hdrs[i]->recip_valid = false;
  }
}

static int parse_alternates(struct Buffer *buf, struct Buffer *s,
                            unsigned long data, struct Buffer *err)
{
  struct GroupContext *gc = NULL;

  _alternates_clean();

  do
  {
    mutt_extract_token(buf, s, 0);

    if (parse_group_context(&gc, buf, s, data, err) == -1)
      goto bail;

    mutt_remove_from_regex_list(&UnAlternates, buf->data);

    if (mutt_add_to_regex_list(&Alternates, buf->data, REG_ICASE, err) != 0)
      goto bail;

    if (mutt_group_context_add_regex(gc, buf->data, REG_ICASE, err) != 0)
      goto bail;
  } while (MoreArgs(s));

  mutt_group_context_destroy(&gc);
  return 0;

bail:
  mutt_group_context_destroy(&gc);
  return -1;
}

static int parse_unalternates(struct Buffer *buf, struct Buffer *s,
                              unsigned long data, struct Buffer *err)
{
  _alternates_clean();
  do
  {
    mutt_extract_token(buf, s, 0);
    mutt_remove_from_regex_list(&Alternates, buf->data);

    if ((mutt_strcmp(buf->data, "*") != 0) &&
        mutt_add_to_regex_list(&UnAlternates, buf->data, REG_ICASE, err) != 0)
      return -1;

  } while (MoreArgs(s));

  return 0;
}

static int parse_replace_list(struct Buffer *buf, struct Buffer *s,
                              unsigned long data, struct Buffer *err)
{
  struct ReplaceList **list = (struct ReplaceList **) data;
  struct Buffer templ;

  memset(&templ, 0, sizeof(templ));

  /* First token is a regex. */
  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("not enough arguments"), err->dsize);
    return -1;
  }
  mutt_extract_token(buf, s, 0);

  /* Second token is a replacement template */
  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("not enough arguments"), err->dsize);
    return -1;
  }
  mutt_extract_token(&templ, s, 0);

  if (add_to_replace_list(list, buf->data, templ.data, err) != 0)
  {
    FREE(&templ.data);
    return -1;
  }
  FREE(&templ.data);

  return 0;
}

static int parse_unreplace_list(struct Buffer *buf, struct Buffer *s,
                                unsigned long data, struct Buffer *err)
{
  struct ReplaceList **list = (struct ReplaceList **) data;

  /* First token is a regex. */
  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("not enough arguments"), err->dsize);
    return -1;
  }

  mutt_extract_token(buf, s, 0);

  /* "*" is a special case. */
  if (mutt_strcmp(buf->data, "*") == 0)
  {
    mutt_free_replace_list(list);
    return 0;
  }

  remove_from_replace_list(list, buf->data);
  return 0;
}

static void clear_subject_mods(void)
{
  if (Context && Context->msgcount)
  {
    for (int i = 0; i < Context->msgcount; i++)
      FREE(&Context->hdrs[i]->env->disp_subj);
  }
}

static int parse_subjectrx_list(struct Buffer *buf, struct Buffer *s,
                                unsigned long data, struct Buffer *err)
{
  int rc;

  rc = parse_replace_list(buf, s, data, err);
  if (rc == 0)
    clear_subject_mods();
  return rc;
}

static int parse_unsubjectrx_list(struct Buffer *buf, struct Buffer *s,
                                  unsigned long data, struct Buffer *err)
{
  int rc;

  rc = parse_unreplace_list(buf, s, data, err);
  if (rc == 0)
    clear_subject_mods();
  return rc;
}

static int parse_spam_list(struct Buffer *buf, struct Buffer *s,
                           unsigned long data, struct Buffer *err)
{
  struct Buffer templ;

  mutt_buffer_init(&templ);

  /* Insist on at least one parameter */
  if (!MoreArgs(s))
  {
    if (data == MUTT_SPAM)
      strfcpy(err->data, _("spam: no matching pattern"), err->dsize);
    else
      strfcpy(err->data, _("nospam: no matching pattern"), err->dsize);
    return -1;
  }

  /* Extract the first token, a regex */
  mutt_extract_token(buf, s, 0);

  /* data should be either MUTT_SPAM or MUTT_NOSPAM. MUTT_SPAM is for spam commands. */
  if (data == MUTT_SPAM)
  {
    /* If there's a second parameter, it's a template for the spam tag. */
    if (MoreArgs(s))
    {
      mutt_extract_token(&templ, s, 0);

      /* Add to the spam list. */
      if (add_to_replace_list(&SpamList, buf->data, templ.data, err) != 0)
      {
        FREE(&templ.data);
        return -1;
      }
      FREE(&templ.data);
    }

    /* If not, try to remove from the nospam list. */
    else
    {
      mutt_remove_from_regex_list(&NoSpamList, buf->data);
    }

    return 0;
  }

  /* MUTT_NOSPAM is for nospam commands. */
  else if (data == MUTT_NOSPAM)
  {
    /* nospam only ever has one parameter. */

    /* "*" is a special case. */
    if (mutt_strcmp(buf->data, "*") == 0)
    {
      mutt_free_replace_list(&SpamList);
      mutt_free_regex_list(&NoSpamList);
      return 0;
    }

    /* If it's on the spam list, just remove it. */
    if (remove_from_replace_list(&SpamList, buf->data) != 0)
      return 0;

    /* Otherwise, add it to the nospam list. */
    if (mutt_add_to_regex_list(&NoSpamList, buf->data, REG_ICASE, err) != 0)
      return -1;

    return 0;
  }

  /* This should not happen. */
  strfcpy(err->data, "This is no good at all.", err->dsize);
  return -1;
}

#ifdef USE_SIDEBAR
static int parse_path_list(struct Buffer *buf, struct Buffer *s,
                           unsigned long data, struct Buffer *err)
{
  char path[_POSIX_PATH_MAX];

  do
  {
    mutt_extract_token(buf, s, 0);
    strfcpy(path, buf->data, sizeof(path));
    mutt_expand_path(path, sizeof(path));
    add_to_stailq((struct ListHead *) data, path);
  } while (MoreArgs(s));

  return 0;
}

static int parse_path_unlist(struct Buffer *buf, struct Buffer *s,
                             unsigned long data, struct Buffer *err)
{
  char path[_POSIX_PATH_MAX];

  do
  {
    mutt_extract_token(buf, s, 0);
    /*
     * Check for deletion of entire list
     */
    if (mutt_strcmp(buf->data, "*") == 0)
    {
      mutt_list_free((struct ListHead *) data);
      break;
    }
    strfcpy(path, buf->data, sizeof(path));
    mutt_expand_path(path, sizeof(path));
    remove_from_stailq((struct ListHead *) data, path);
  } while (MoreArgs(s));

  return 0;
}
#endif

static int parse_lists(struct Buffer *buf, struct Buffer *s, unsigned long data,
                       struct Buffer *err)
{
  struct GroupContext *gc = NULL;

  do
  {
    mutt_extract_token(buf, s, 0);

    if (parse_group_context(&gc, buf, s, data, err) == -1)
      goto bail;

    mutt_remove_from_regex_list(&UnMailLists, buf->data);

    if (mutt_add_to_regex_list(&MailLists, buf->data, REG_ICASE, err) != 0)
      goto bail;

    if (mutt_group_context_add_regex(gc, buf->data, REG_ICASE, err) != 0)
      goto bail;
  } while (MoreArgs(s));

  mutt_group_context_destroy(&gc);
  return 0;

bail:
  mutt_group_context_destroy(&gc);
  return -1;
}

/**
 * enum GroupState - Type of email address group
 */
enum GroupState
{
  GS_NONE,
  GS_RX,
  GS_ADDR
};

static int parse_group(struct Buffer *buf, struct Buffer *s, unsigned long data,
                       struct Buffer *err)
{
  struct GroupContext *gc = NULL;
  enum GroupState state = GS_NONE;
  struct Address *addr = NULL;
  char *estr = NULL;

  do
  {
    mutt_extract_token(buf, s, 0);
    if (parse_group_context(&gc, buf, s, data, err) == -1)
      goto bail;

    if (data == MUTT_UNGROUP && (mutt_strcasecmp(buf->data, "*") == 0))
    {
      if (mutt_group_context_clear(&gc) < 0)
        goto bail;
      goto out;
    }

    if (mutt_strcasecmp(buf->data, "-rx") == 0)
      state = GS_RX;
    else if (mutt_strcasecmp(buf->data, "-addr") == 0)
      state = GS_ADDR;
    else
    {
      switch (state)
      {
        case GS_NONE:
          snprintf(err->data, err->dsize, _("%sgroup: missing -rx or -addr."),
                   data == MUTT_UNGROUP ? "un" : "");
          goto bail;

        case GS_RX:
          if (data == MUTT_GROUP &&
              mutt_group_context_add_regex(gc, buf->data, REG_ICASE, err) != 0)
            goto bail;
          else if (data == MUTT_UNGROUP &&
                   mutt_group_context_remove_regex(gc, buf->data) < 0)
            goto bail;
          break;

        case GS_ADDR:
          addr = mutt_parse_adrlist(NULL, buf->data);
          if (!addr)
            goto bail;
          if (mutt_addrlist_to_intl(addr, &estr))
          {
            snprintf(err->data, err->dsize,
                     _("%sgroup: warning: bad IDN '%s'.\n"), data == 1 ? "un" : "", estr);
            rfc822_free_address(&addr);
            FREE(&estr);
            goto bail;
          }
          if (data == MUTT_GROUP)
            mutt_group_context_add_adrlist(gc, addr);
          else if (data == MUTT_UNGROUP)
            mutt_group_context_remove_adrlist(gc, addr);
          rfc822_free_address(&addr);
          break;
      }
    }
  } while (MoreArgs(s));

out:
  mutt_group_context_destroy(&gc);
  return 0;

bail:
  mutt_group_context_destroy(&gc);
  return -1;
}

/**
 * _attachments_clean - always wise to do what someone else did before
 */
static void _attachments_clean(void)
{
  if (Context && Context->msgcount)
  {
    for (int i = 0; i < Context->msgcount; i++)
      Context->hdrs[i]->attach_valid = false;
  }
}

static int parse_attach_list(struct Buffer *buf, struct Buffer *s,
                             struct ListHead *head, struct Buffer *err)
{
  struct AttachMatch *a = NULL;
  char *p = NULL;
  char *tmpminor = NULL;
  int len;
  int ret;

  do
  {
    mutt_extract_token(buf, s, 0);

    if (!buf->data || *buf->data == '\0')
      continue;

    a = safe_malloc(sizeof(struct AttachMatch));

    /* some cheap hacks that I expect to remove */
    if (mutt_strcasecmp(buf->data, "any") == 0)
      a->major = safe_strdup("*/.*");
    else if (mutt_strcasecmp(buf->data, "none") == 0)
      a->major = safe_strdup("cheap_hack/this_should_never_match");
    else
      a->major = safe_strdup(buf->data);

    if ((p = strchr(a->major, '/')))
    {
      *p = '\0';
      p++;
      a->minor = p;
    }
    else
    {
      a->minor = "unknown";
    }

    len = strlen(a->minor);
    tmpminor = safe_malloc(len + 3);
    strcpy(&tmpminor[1], a->minor);
    tmpminor[0] = '^';
    tmpminor[len + 1] = '$';
    tmpminor[len + 2] = '\0';

    a->major_int = mutt_check_mime_type(a->major);
    ret = REGCOMP(&a->minor_regex, tmpminor, REG_ICASE);

    FREE(&tmpminor);

    if (ret)
    {
      regerror(ret, &a->minor_regex, err->data, err->dsize);
      FREE(&a->major);
      FREE(&a);
      return -1;
    }

    mutt_debug(5, "parse_attach_list: added %s/%s [%d]\n", a->major, a->minor, a->major_int);

    mutt_list_insert_tail(head, (char *) a);
  } while (MoreArgs(s));

  _attachments_clean();
  return 0;
}

static int parse_unattach_list(struct Buffer *buf, struct Buffer *s,
                               struct ListHead *head, struct Buffer *err)
{
  struct AttachMatch *a = NULL;
  char *tmp = NULL;
  int major;
  char *minor = NULL;

  do
  {
    mutt_extract_token(buf, s, 0);
    FREE(&tmp);

    if (mutt_strcasecmp(buf->data, "any") == 0)
      tmp = safe_strdup("*/.*");
    else if (mutt_strcasecmp(buf->data, "none") == 0)
      tmp = safe_strdup("cheap_hack/this_should_never_match");
    else
      tmp = safe_strdup(buf->data);

    if ((minor = strchr(tmp, '/')))
    {
      *minor = '\0';
      minor++;
    }
    else
    {
      minor = "unknown";
    }
    major = mutt_check_mime_type(tmp);

    struct ListNode *np, *tmp2;
    STAILQ_FOREACH_SAFE(np, head, entries, tmp2)
    {
      a = (struct AttachMatch *) np->data;
      mutt_debug(5, "parse_unattach_list: check %s/%s [%d] : %s/%s [%d]\n",
                 a->major, a->minor, a->major_int, tmp, minor, major);
      if (a->major_int == major && (mutt_strcasecmp(minor, a->minor) == 0))
      {
        mutt_debug(5, "parse_unattach_list: removed %s/%s [%d]\n", a->major,
                   a->minor, a->major_int);
        regfree(&a->minor_regex);
        FREE(&a->major);
        STAILQ_REMOVE(head, np, ListNode, entries);
        FREE(&np->data);
        FREE(&np);
      }
    }

  } while (MoreArgs(s));

  FREE(&tmp);
  _attachments_clean();
  return 0;
}

static int print_attach_list(struct ListHead *h, char op, char *name)
{
  struct ListNode *np;
  STAILQ_FOREACH(np, h, entries)
  {
    printf("attachments %c%s %s/%s\n", op, name,
           ((struct AttachMatch *) np->data)->major,
           ((struct AttachMatch *) np->data)->minor);
  }

  return 0;
}

static int parse_attachments(struct Buffer *buf, struct Buffer *s,
                             unsigned long data, struct Buffer *err)
{
  char op, *category = NULL;
  struct ListHead *head = NULL;

  mutt_extract_token(buf, s, 0);
  if (!buf->data || *buf->data == '\0')
  {
    strfcpy(err->data, _("attachments: no disposition"), err->dsize);
    return -1;
  }

  category = buf->data;
  op = *category++;

  if (op == '?')
  {
    mutt_endwin(NULL);
    fflush(stdout);
    printf(_("\nCurrent attachments settings:\n\n"));
    print_attach_list(&AttachAllow, '+', "A");
    print_attach_list(&AttachExclude, '-', "A");
    print_attach_list(&InlineAllow, '+', "I");
    print_attach_list(&InlineExclude, '-', "I");
    mutt_any_key_to_continue(NULL);
    return 0;
  }

  if (op != '+' && op != '-')
  {
    op = '+';
    category--;
  }
  if (mutt_strncasecmp(category, "attachment", strlen(category)) == 0)
  {
    if (op == '+')
      head = &AttachAllow;
    else
      head = &AttachExclude;
  }
  else if (mutt_strncasecmp(category, "inline", strlen(category)) == 0)
  {
    if (op == '+')
      head = &InlineAllow;
    else
      head = &InlineExclude;
  }
  else
  {
    strfcpy(err->data, _("attachments: invalid disposition"), err->dsize);
    return -1;
  }

  return parse_attach_list(buf, s, head, err);
}

static int parse_unattachments(struct Buffer *buf, struct Buffer *s,
                               unsigned long data, struct Buffer *err)
{
  char op, *p = NULL;
  struct ListHead *head = NULL;

  mutt_extract_token(buf, s, 0);
  if (!buf->data || *buf->data == '\0')
  {
    strfcpy(err->data, _("unattachments: no disposition"), err->dsize);
    return -1;
  }

  p = buf->data;
  op = *p++;
  if (op != '+' && op != '-')
  {
    op = '+';
    p--;
  }
  if (mutt_strncasecmp(p, "attachment", strlen(p)) == 0)
  {
    if (op == '+')
      head = &AttachAllow;
    else
      head = &AttachExclude;
  }
  else if (mutt_strncasecmp(p, "inline", strlen(p)) == 0)
  {
    if (op == '+')
      head = &InlineAllow;
    else
      head = &InlineExclude;
  }
  else
  {
    strfcpy(err->data, _("unattachments: invalid disposition"), err->dsize);
    return -1;
  }

  return parse_unattach_list(buf, s, head, err);
}

static int parse_unlists(struct Buffer *buf, struct Buffer *s,
                         unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);
    mutt_remove_from_regex_list(&SubscribedLists, buf->data);
    mutt_remove_from_regex_list(&MailLists, buf->data);

    if ((mutt_strcmp(buf->data, "*") != 0) &&
        mutt_add_to_regex_list(&UnMailLists, buf->data, REG_ICASE, err) != 0)
      return -1;
  } while (MoreArgs(s));

  return 0;
}

static int parse_subscribe(struct Buffer *buf, struct Buffer *s,
                           unsigned long data, struct Buffer *err)
{
  struct GroupContext *gc = NULL;

  do
  {
    mutt_extract_token(buf, s, 0);

    if (parse_group_context(&gc, buf, s, data, err) == -1)
      goto bail;

    mutt_remove_from_regex_list(&UnMailLists, buf->data);
    mutt_remove_from_regex_list(&UnSubscribedLists, buf->data);

    if (mutt_add_to_regex_list(&MailLists, buf->data, REG_ICASE, err) != 0)
      goto bail;
    if (mutt_add_to_regex_list(&SubscribedLists, buf->data, REG_ICASE, err) != 0)
      goto bail;
    if (mutt_group_context_add_regex(gc, buf->data, REG_ICASE, err) != 0)
      goto bail;
  } while (MoreArgs(s));

  mutt_group_context_destroy(&gc);
  return 0;

bail:
  mutt_group_context_destroy(&gc);
  return -1;
}

static int parse_unsubscribe(struct Buffer *buf, struct Buffer *s,
                             unsigned long data, struct Buffer *err)
{
  do
  {
    mutt_extract_token(buf, s, 0);
    mutt_remove_from_regex_list(&SubscribedLists, buf->data);

    if ((mutt_strcmp(buf->data, "*") != 0) &&
        mutt_add_to_regex_list(&UnSubscribedLists, buf->data, REG_ICASE, err) != 0)
      return -1;
  } while (MoreArgs(s));

  return 0;
}

static int parse_unalias(struct Buffer *buf, struct Buffer *s,
                         unsigned long data, struct Buffer *err)
{
  struct Alias *tmp = NULL, *last = NULL;

  do
  {
    mutt_extract_token(buf, s, 0);

    if (mutt_strcmp("*", buf->data) == 0)
    {
      if (CurrentMenu == MENU_ALIAS)
      {
        for (tmp = Aliases; tmp; tmp = tmp->next)
          tmp->del = true;
        mutt_set_current_menu_redraw_full();
      }
      else
        mutt_free_alias(&Aliases);
      break;
    }
    else
      for (tmp = Aliases; tmp; tmp = tmp->next)
      {
        if (mutt_strcasecmp(buf->data, tmp->name) == 0)
        {
          if (CurrentMenu == MENU_ALIAS)
          {
            tmp->del = true;
            mutt_set_current_menu_redraw_full();
            break;
          }

          if (last)
            last->next = tmp->next;
          else
            Aliases = tmp->next;
          tmp->next = NULL;
          mutt_free_alias(&tmp);
          break;
        }
        last = tmp;
      }
  } while (MoreArgs(s));
  return 0;
}

static int parse_alias(struct Buffer *buf, struct Buffer *s, unsigned long data,
                       struct Buffer *err)
{
  struct Alias *tmp = Aliases;
  struct Alias *last = NULL;
  char *estr = NULL;
  struct GroupContext *gc = NULL;

  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("alias: no address"), err->dsize);
    return -1;
  }

  mutt_extract_token(buf, s, 0);

  if (parse_group_context(&gc, buf, s, data, err) == -1)
    return -1;

  /* check to see if an alias with this name already exists */
  for (; tmp; tmp = tmp->next)
  {
    if (mutt_strcasecmp(tmp->name, buf->data) == 0)
      break;
    last = tmp;
  }

  if (!tmp)
  {
    /* create a new alias */
    tmp = safe_calloc(1, sizeof(struct Alias));
    tmp->self = tmp;
    tmp->name = safe_strdup(buf->data);
    /* give the main addressbook code a chance */
    if (CurrentMenu == MENU_ALIAS)
      set_option(OPT_MENU_CALLER);
  }
  else
  {
    mutt_alias_delete_reverse(tmp);
    /* override the previous value */
    rfc822_free_address(&tmp->addr);
    if (CurrentMenu == MENU_ALIAS)
      mutt_set_current_menu_redraw_full();
  }

  mutt_extract_token(buf, s, MUTT_TOKEN_QUOTE | MUTT_TOKEN_SPACE | MUTT_TOKEN_SEMICOLON);
  mutt_debug(3, "parse_alias: Second token is '%s'.\n", buf->data);

  tmp->addr = mutt_parse_adrlist(tmp->addr, buf->data);

  if (last)
    last->next = tmp;
  else
    Aliases = tmp;
  if (mutt_addrlist_to_intl(tmp->addr, &estr))
  {
    snprintf(err->data, err->dsize, _("Warning: Bad IDN '%s' in alias '%s'.\n"),
             estr, tmp->name);
    FREE(&estr);
    goto bail;
  }

  mutt_group_context_add_adrlist(gc, tmp->addr);
  mutt_alias_add_reverse(tmp);

#ifdef DEBUG
  if (debuglevel >= 2)
  {
    /* A group is terminated with an empty address, so check a->mailbox */
    for (struct Address *a = tmp->addr; a && a->mailbox; a = a->next)
    {
      if (!a->group)
        mutt_debug(3, "parse_alias:   %s\n", a->mailbox);
      else
        mutt_debug(3, "parse_alias:   Group %s\n", a->mailbox);
    }
  }
#endif
  mutt_group_context_destroy(&gc);
  return 0;

bail:
  mutt_group_context_destroy(&gc);
  return -1;
}

static int parse_unmy_hdr(struct Buffer *buf, struct Buffer *s,
                          unsigned long data, struct Buffer *err)
{
  struct ListNode *np, *tmp;
  size_t l;

  do
  {
    mutt_extract_token(buf, s, 0);
    if (mutt_strcmp("*", buf->data) == 0)
    {
      mutt_list_free(&UserHeader);
      continue;
    }

    l = mutt_strlen(buf->data);
    if (buf->data[l - 1] == ':')
      l--;

    STAILQ_FOREACH_SAFE(np, &UserHeader, entries, tmp)
    {
      if ((mutt_strncasecmp(buf->data, np->data, l) == 0) && np->data[l] == ':')
      {
        STAILQ_REMOVE(&UserHeader, np, ListNode, entries);
        FREE(&np->data);
        FREE(&np);
      }
    }
  } while (MoreArgs(s));
  return 0;
}

static int parse_my_hdr(struct Buffer *buf, struct Buffer *s,
                        unsigned long data, struct Buffer *err)
{
  struct ListNode *n = NULL;
  size_t keylen;
  char *p = NULL;

  mutt_extract_token(buf, s, MUTT_TOKEN_SPACE | MUTT_TOKEN_QUOTE);
  if ((p = strpbrk(buf->data, ": \t")) == NULL || *p != ':')
  {
    strfcpy(err->data, _("invalid header field"), err->dsize);
    return -1;
  }
  keylen = p - buf->data + 1;

  STAILQ_FOREACH(n, &UserHeader, entries)
  {
    /* see if there is already a field by this name */
    if (mutt_strncasecmp(buf->data, n->data, keylen) == 0)
    {
      break;
    }
  }

  if (!n)
  {
    /* not found, allocate memory for a new node and add it to the list */
    n = mutt_list_insert_tail(&UserHeader, NULL);
  }
  else
  {
    /* found, free the existing data */
    FREE(&n->data);
  }

  n->data = buf->data;
  mutt_buffer_init(buf);

  return 0;
}

static void set_default(struct Option *p)
{
  switch (DTYPE(p->type))
  {
    case DT_STRING:
      if (!p->init && *((char **) p->data))
        p->init = (unsigned long) safe_strdup(*((char **) p->data));
      break;
    case DT_PATH:
      if (!p->init && *((char **) p->data))
      {
        char *cp = safe_strdup(*((char **) p->data));
        /* mutt_pretty_mailbox (cp); */
        p->init = (unsigned long) cp;
      }
      break;
    case DT_ADDRESS:
      if (!p->init && *((struct Address **) p->data))
      {
        char tmp[HUGE_STRING];
        *tmp = '\0';
        rfc822_write_address(tmp, sizeof(tmp), *((struct Address **) p->data), 0);
        p->init = (unsigned long) safe_strdup(tmp);
      }
      break;
    case DT_REGEX:
    {
      struct Regex *pp = (struct Regex *) p->data;
      if (!p->init && pp->pattern)
        p->init = (unsigned long) safe_strdup(pp->pattern);
      break;
    }
  }
}

static void restore_default(struct Option *p)
{
  switch (DTYPE(p->type))
  {
    case DT_STRING:
      mutt_str_replace((char **) p->data, (char *) p->init);
      break;
    case DT_MBTABLE:
      free_mbtable((struct MbTable **) p->data);
      *((struct MbTable **) p->data) = parse_mbtable((char *) p->init);
      break;
    case DT_PATH:
      FREE((char **) p->data);
      char *init = NULL;
#ifdef DEBUG
      if (mutt_strcmp(p->option, "debug_file") == 0 && debugfile_cmdline)
        init = debugfile_cmdline;
      else
#endif
        init = (char *) p->init;
      if (init)
      {
        char path[_POSIX_PATH_MAX];
        strfcpy(path, init, sizeof(path));
        mutt_expand_path(path, sizeof(path));
        *((char **) p->data) = safe_strdup(path);
      }
      break;
    case DT_ADDRESS:
      rfc822_free_address((struct Address **) p->data);
      if (p->init)
        *((struct Address **) p->data) = rfc822_parse_adrlist(NULL, (char *) p->init);
      break;
    case DT_BOOL:
      if (p->init)
        set_option(p->data);
      else
        unset_option(p->data);
      break;
    case DT_QUAD:
      set_quadoption(p->data, p->init);
      break;
    case DT_NUMBER:
    case DT_SORT:
    case DT_MAGIC:
#ifdef DEBUG
      if (mutt_strcmp(p->option, "debug_level") == 0 && debuglevel_cmdline)
        *((short *) p->data) = debuglevel_cmdline;
      else
#endif
        *((short *) p->data) = p->init;
      break;
    case DT_REGEX:
    {
      struct Regex *pp = (struct Regex *) p->data;
      int flags = 0;

      FREE(&pp->pattern);
      if (pp->regex)
      {
        regfree(pp->regex);
        FREE(&pp->regex);
      }

      if (p->init)
      {
        int retval;
        char *s = (char *) p->init;

        pp->regex = safe_calloc(1, sizeof(regex_t));
        pp->pattern = safe_strdup((char *) p->init);
        if (mutt_strcmp(p->option, "mask") != 0)
          flags |= mutt_which_case((const char *) p->init);
        if ((mutt_strcmp(p->option, "mask") == 0) && *s == '!')
        {
          s++;
          pp->not = true;
        }
        retval = REGCOMP(pp->regex, s, flags);
        if (retval != 0)
        {
          char msgbuf[STRING];
          regerror(retval, pp->regex, msgbuf, sizeof(msgbuf));
          fprintf(stderr, _("restore_default(%s): error in regex: %s\n"),
                  p->option, pp->pattern);
          fprintf(stderr, "%s\n", msgbuf);
          mutt_sleep(0);
          FREE(&pp->pattern);
          FREE(&pp->regex);
        }
      }
    }
    break;
  }

  if (p->flags & R_INDEX)
    mutt_set_menu_redraw_full(MENU_MAIN);
  if (p->flags & R_PAGER)
    mutt_set_menu_redraw_full(MENU_PAGER);
  if (p->flags & R_PAGER_FLOW)
  {
    mutt_set_menu_redraw_full(MENU_PAGER);
    mutt_set_menu_redraw(MENU_PAGER, REDRAW_FLOW);
  }
  if (p->flags & R_RESORT_SUB)
    set_option(OPT_SORT_SUBTHREADS);
  if (p->flags & R_RESORT)
    set_option(OPT_NEED_RESORT);
  if (p->flags & R_RESORT_INIT)
    set_option(OPT_RESORT_INIT);
  if (p->flags & R_TREE)
    set_option(OPT_REDRAW_TREE);
  if (p->flags & R_REFLOW)
    mutt_reflow_windows();
#ifdef USE_SIDEBAR
  if (p->flags & R_SIDEBAR)
    mutt_set_current_menu_redraw(REDRAW_SIDEBAR);
#endif
  if (p->flags & R_MENU)
    mutt_set_current_menu_redraw_full();
}

static void esc_char(char c, char *p, char *dst, size_t len)
{
  *p++ = '\\';
  if (p - dst < len)
    *p++ = c;
}

static size_t escape_string(char *dst, size_t len, const char *src)
{
  char *p = dst;

  if (!len)
    return 0;
  len--; /* save room for \0 */
  while (p - dst < len && src && *src)
  {
    switch (*src)
    {
      case '\n':
        esc_char('n', p, dst, len);
        break;
      case '\r':
        esc_char('r', p, dst, len);
        break;
      case '\t':
        esc_char('t', p, dst, len);
        break;
      default:
        if ((*src == '\\' || *src == '"') && p - dst < len - 1)
          *p++ = '\\';
        *p++ = *src;
    }
    src++;
  }
  *p = '\0';
  return p - dst;
}

static void pretty_var(char *dst, size_t len, const char *option, const char *val)
{
  char *p = NULL;

  if (!len)
    return;

  strfcpy(dst, option, len);
  len--; /* save room for \0 */
  p = dst + mutt_strlen(dst);

  if (p - dst < len)
    *p++ = '=';
  if (p - dst < len)
    *p++ = '"';
  p += escape_string(p, len - (p - dst) + 1, val); /* \0 terminate it */
  if (p - dst < len)
    *p++ = '"';
  *p = '\0';
}

static int check_charset(struct Option *opt, const char *val)
{
  char *q = NULL, *s = safe_strdup(val);
  int rc = 0;
  bool strict = (strcmp(opt->option, "send_charset") == 0);

  if (!s)
    return rc;

  for (char *p = strtok_r(s, ":", &q); p; p = strtok_r(NULL, ":", &q))
  {
    if (!*p)
      continue;
    if (!mutt_check_charset(p, strict))
    {
      rc = -1;
      break;
    }
  }

  FREE(&s);
  return rc;
}

static bool valid_show_multipart_alternative(const char *val)
{
  return ((mutt_strcmp(val, "inline") == 0) ||
          (mutt_strcmp(val, "info") == 0) || !val || (*val == 0));
}

char **mutt_envlist(void)
{
  return envlist;
}

#ifdef DEBUG
/**
 * start_debug - prepare the debugging file
 *
 * This method prepares and opens a new debug file for mutt_debug.
 */
static void start_debug(void)
{
  if (!DebugFile)
    return;

  char buf[_POSIX_PATH_MAX];

  /* rotate the old debug logs */
  for (int i = 3; i >= 0; i--)
  {
    snprintf(debugfilename, sizeof(debugfilename), "%s%d", DebugFile, i);
    snprintf(buf, sizeof(buf), "%s%d", DebugFile, i + 1);

    mutt_expand_path(debugfilename, sizeof(debugfilename));
    mutt_expand_path(buf, sizeof(buf));
    rename(debugfilename, buf);
  }

  debugfile = safe_fopen(debugfilename, "w");
  if (debugfile)
  {
    setbuf(debugfile, NULL); /* don't buffer the debugging output! */
    mutt_debug(1, "NeoMutt/%s debugging at level %d\n", PACKAGE_VERSION, debuglevel);
  }
}

/**
 * restart_debug - reload the debugging configuration
 *
 * This method closes the old debug file is debug was enabled,
 * then reconfigure the debugging system from the configuration options
 * and start a new debug file if debug is enabled
 */
static void restart_debug(void)
{
  bool disable_debug = (debuglevel > 0 && DebugLevel == 0);
  bool enable_debug = (debuglevel == 0 && DebugLevel > 0);
  bool file_changed =
      ((mutt_strlen(debugfilename) - 1) != mutt_strlen(DebugFile) ||
       mutt_strncmp(debugfilename, DebugFile, mutt_strlen(debugfilename) - 1));

  if (disable_debug || file_changed)
  {
    mutt_debug(1, "NeoMutt/%s stop debugging\n", PACKAGE_VERSION);
    safe_fclose(&debugfile);
  }

  if (!enable_debug && !disable_debug && debuglevel != DebugLevel)
    mutt_debug(1, "NeoMutt/%s debugging at level %d\n", PACKAGE_VERSION, DebugLevel);

  debuglevel = DebugLevel;

  if (enable_debug || (file_changed && debuglevel > 0))
    start_debug();
}
#endif

/* mutt_envlist_set - Helper function for parse_setenv()
 * @param name      Name of the environment variable
 * @param value     Value the envionment variable should have
 * @param overwrite Whether the environment variable should be overwritten
 *
 * It's broken out because some other parts of neomutt (filter.c) need
 * to set/overwrite environment variables in envlist before execing.
 */
void mutt_envlist_set(const char *name, const char *value, bool overwrite)
{
  char **envp = envlist;
  char work[LONG_STRING];
  int count, len;

  len = mutt_strlen(name);

  /* Look for current slot to overwrite */
  count = 0;
  while (envp && *envp)
  {
    if ((mutt_strncmp(name, *envp, len) == 0) && (*envp)[len] == '=')
    {
      if (!overwrite)
        return;
      break;
    }
    envp++;
    count++;
  }

  /* Format var=value string */
  snprintf(work, sizeof(work), "%s=%s", NONULL(name), NONULL(value));

  /* If slot found, overwrite */
  if (envp && *envp)
    mutt_str_replace(envp, work);

  /* If not found, add new slot */
  else
  {
    safe_realloc(&envlist, sizeof(char *) * (count + 2));
    envlist[count] = safe_strdup(work);
    envlist[count + 1] = NULL;
  }
}

static int parse_setenv(struct Buffer *tmp, struct Buffer *s,
                        unsigned long data, struct Buffer *err)
{
  int query, unset, len;
  char *name = NULL, **save = NULL, **envp = envlist;
  int count = 0;

  query = 0;
  unset = data & MUTT_SET_UNSET;

  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("too few arguments"), err->dsize);
    return -1;
  }

  if (*s->dptr == '?')
  {
    query = 1;
    s->dptr++;
  }

  /* get variable name */
  mutt_extract_token(tmp, s, MUTT_TOKEN_EQUAL);
  len = strlen(tmp->data);

  if (query)
  {
    int found = 0;
    while (envp && *envp)
    {
      if (mutt_strncmp(tmp->data, *envp, len) == 0)
      {
        if (!found)
        {
          mutt_endwin(NULL);
          found = 1;
        }
        puts(*envp);
      }
      envp++;
    }

    if (found)
    {
      mutt_any_key_to_continue(NULL);
      return 0;
    }

    snprintf(err->data, err->dsize, _("%s is unset"), tmp->data);
    return -1;
  }

  if (unset)
  {
    count = 0;
    while (envp && *envp)
    {
      if ((mutt_strncmp(tmp->data, *envp, len) == 0) && (*envp)[len] == '=')
      {
        /* shuffle down */
        save = envp++;
        while (*envp)
        {
          *save++ = *envp++;
          count++;
        }
        *save = NULL;
        safe_realloc(&envlist, sizeof(char *) * (count + 1));
        return 0;
      }
      envp++;
      count++;
    }
    return -1;
  }

  /* set variable */

  if (*s->dptr == '=')
  {
    s->dptr++;
    SKIPWS(s->dptr);
  }

  if (!MoreArgs(s))
  {
    strfcpy(err->data, _("too few arguments"), err->dsize);
    return -1;
  }

  name = safe_strdup(tmp->data);
  mutt_extract_token(tmp, s, 0);
  mutt_envlist_set(name, tmp->data, true);
  FREE(&name);

  return 0;
}

static int parse_set(struct Buffer *tmp, struct Buffer *s, unsigned long data,
                     struct Buffer *err)
{
  int query, unset, inv, reset, r = 0;
  int idx = -1;
  const char *p = NULL;
  char scratch[_POSIX_PATH_MAX];
  char *myvar = NULL;

  while (MoreArgs(s))
  {
    /* reset state variables */
    query = 0;
    unset = data & MUTT_SET_UNSET;
    inv = data & MUTT_SET_INV;
    reset = data & MUTT_SET_RESET;
    myvar = NULL;

    if (*s->dptr == '?')
    {
      query = 1;
      s->dptr++;
    }
    else if (mutt_strncmp("no", s->dptr, 2) == 0)
    {
      s->dptr += 2;
      unset = !unset;
    }
    else if (mutt_strncmp("inv", s->dptr, 3) == 0)
    {
      s->dptr += 3;
      inv = !inv;
    }
    else if (*s->dptr == '&')
    {
      reset = 1;
      s->dptr++;
    }

    /* get the variable name */
    mutt_extract_token(tmp, s, MUTT_TOKEN_EQUAL);

    if (mutt_strncmp("my_", tmp->data, 3) == 0)
      myvar = tmp->data;
    else if ((idx = mutt_option_index(tmp->data)) == -1 &&
             !(reset && (mutt_strcmp("all", tmp->data) == 0)))
    {
      snprintf(err->data, err->dsize, _("%s: unknown variable"), tmp->data);
      return -1;
    }
    SKIPWS(s->dptr);

    if (reset)
    {
      if (query || unset || inv)
      {
        snprintf(err->data, err->dsize, "%s",
                 _("prefix is illegal with reset"));
        return -1;
      }

      if (*s->dptr == '=')
      {
        snprintf(err->data, err->dsize, "%s", _("value is illegal with reset"));
        return -1;
      }

      if (mutt_strcmp("all", tmp->data) == 0)
      {
        if (CurrentMenu == MENU_PAGER)
        {
          snprintf(err->data, err->dsize, "%s",
                   _("Not available in this menu."));
          return -1;
        }
        for (idx = 0; MuttVars[idx].option; idx++)
          restore_default(&MuttVars[idx]);
        mutt_set_current_menu_redraw_full();
        set_option(OPT_SORT_SUBTHREADS);
        set_option(OPT_NEED_RESORT);
        set_option(OPT_RESORT_INIT);
        set_option(OPT_REDRAW_TREE);
        return 0;
      }
      else
      {
        CHECK_PAGER;
        if (myvar)
          myvar_del(myvar);
        else
          restore_default(&MuttVars[idx]);
      }
    }
    else if (!myvar && DTYPE(MuttVars[idx].type) == DT_BOOL)
    {
      if (*s->dptr == '=')
      {
        if (unset || inv || query)
        {
          snprintf(err->data, err->dsize, "%s",
                   _("Usage: set variable=yes|no"));
          return -1;
        }

        s->dptr++;
        mutt_extract_token(tmp, s, 0);
        if (mutt_strcasecmp("yes", tmp->data) == 0)
          unset = inv = 0;
        else if (mutt_strcasecmp("no", tmp->data) == 0)
          unset = 1;
        else
        {
          snprintf(err->data, err->dsize, "%s",
                   _("Usage: set variable=yes|no"));
          return -1;
        }
      }

      if (query)
      {
        snprintf(err->data, err->dsize,
                 option(MuttVars[idx].data) ? _("%s is set") : _("%s is unset"),
                 tmp->data);
        return 0;
      }

      CHECK_PAGER;
      if (unset)
        unset_option(MuttVars[idx].data);
      else if (inv)
        toggle_option(MuttVars[idx].data);
      else
        set_option(MuttVars[idx].data);
    }
    else if (myvar || DTYPE(MuttVars[idx].type) == DT_STRING ||
             DTYPE(MuttVars[idx].type) == DT_PATH || DTYPE(MuttVars[idx].type) == DT_ADDRESS ||
             DTYPE(MuttVars[idx].type) == DT_MBTABLE)
    {
      if (unset)
      {
        CHECK_PAGER;
        if (myvar)
          myvar_del(myvar);
        else if (DTYPE(MuttVars[idx].type) == DT_ADDRESS)
          rfc822_free_address((struct Address **) MuttVars[idx].data);
        else if (DTYPE(MuttVars[idx].type) == DT_MBTABLE)
          free_mbtable((struct MbTable **) MuttVars[idx].data);
        else
          /* MuttVars[idx].data is already 'char**' (or some 'void**') or...
           * so cast to 'void*' is okay */
          FREE((void *) MuttVars[idx].data);
      }
      else if (query || *s->dptr != '=')
      {
        char _tmp[LONG_STRING];
        const char *val = NULL;

        if (myvar)
        {
          if ((val = myvar_get(myvar)))
          {
            pretty_var(err->data, err->dsize, myvar, val);
            break;
          }
          else
          {
            snprintf(err->data, err->dsize, _("%s: unknown variable"), myvar);
            return -1;
          }
        }
        else if (DTYPE(MuttVars[idx].type) == DT_ADDRESS)
        {
          _tmp[0] = '\0';
          rfc822_write_address(_tmp, sizeof(_tmp),
                               *((struct Address **) MuttVars[idx].data), 0);
          val = _tmp;
        }
        else if (DTYPE(MuttVars[idx].type) == DT_PATH)
        {
          _tmp[0] = '\0';
          strfcpy(_tmp, NONULL(*((char **) MuttVars[idx].data)), sizeof(_tmp));
          mutt_pretty_mailbox(_tmp, sizeof(_tmp));
          val = _tmp;
        }
        else if (DTYPE(MuttVars[idx].type) == DT_MBTABLE)
        {
          struct MbTable *mbt = (*((struct MbTable **) MuttVars[idx].data));
          val = mbt ? NONULL(mbt->orig_str) : "";
        }
        else
          val = *((char **) MuttVars[idx].data);

        /* user requested the value of this variable */
        pretty_var(err->data, err->dsize, MuttVars[idx].option, NONULL(val));
        break;
      }
      else
      {
        CHECK_PAGER;
        s->dptr++;

        if (myvar)
        {
          /* myvar is a pointer to tmp and will be lost on extract_token */
          myvar = safe_strdup(myvar);
          myvar_del(myvar);
        }

        mutt_extract_token(tmp, s, 0);

        if (myvar)
        {
          myvar_set(myvar, tmp->data);
          FREE(&myvar);
          myvar = "don't resort";
        }
        else if (DTYPE(MuttVars[idx].type) == DT_PATH)
        {
#ifdef DEBUG
          if (mutt_strcmp(MuttVars[idx].option, "debug_file") == 0 && debugfile_cmdline)
          {
            mutt_message(_("set debug_file ignored, it have been overridden "
                           "with cmdline"));
            break;
          }
#endif
          /* MuttVars[idx].data is already 'char**' (or some 'void**') or...
           * so cast to 'void*' is okay */
          FREE((void *) MuttVars[idx].data);

          strfcpy(scratch, tmp->data, sizeof(scratch));
          mutt_expand_path(scratch, sizeof(scratch));
          *((char **) MuttVars[idx].data) = safe_strdup(scratch);
#ifdef DEBUG
          if (mutt_strcmp(MuttVars[idx].option, "debug_file") == 0)
            restart_debug();
#endif
        }
        else if (DTYPE(MuttVars[idx].type) == DT_STRING)
        {
          if ((strstr(MuttVars[idx].option, "charset") &&
               check_charset(&MuttVars[idx], tmp->data) < 0) |
              /* $charset can't be empty, others can */
              ((strcmp(MuttVars[idx].option, "charset") == 0) && !*tmp->data))
          {
            snprintf(err->data, err->dsize,
                     _("Invalid value for option %s: \"%s\""),
                     MuttVars[idx].option, tmp->data);
            return -1;
          }

          FREE((void *) MuttVars[idx].data);
          *((char **) MuttVars[idx].data) = safe_strdup(tmp->data);
          if (mutt_strcmp(MuttVars[idx].option, "charset") == 0)
            mutt_set_charset(Charset);

          if ((mutt_strcmp(MuttVars[idx].option,
                           "show_multipart_alternative") == 0) &&
              !valid_show_multipart_alternative(tmp->data))
          {
            snprintf(err->data, err->dsize,
                     _("Invalid value for option %s: \"%s\""),
                     MuttVars[idx].option, tmp->data);
            return -1;
          }
        }
        else if (DTYPE(MuttVars[idx].type) == DT_MBTABLE)
        {
          free_mbtable((struct MbTable **) MuttVars[idx].data);
          *((struct MbTable **) MuttVars[idx].data) = parse_mbtable(tmp->data);
        }
        else
        {
          rfc822_free_address((struct Address **) MuttVars[idx].data);
          *((struct Address **) MuttVars[idx].data) =
              rfc822_parse_adrlist(NULL, tmp->data);
        }
      }
    }
    else if (DTYPE(MuttVars[idx].type) == DT_REGEX)
    {
      if (query || *s->dptr != '=')
      {
        /* user requested the value of this variable */
        struct Regex *ptr = (struct Regex *) MuttVars[idx].data;
        pretty_var(err->data, err->dsize, MuttVars[idx].option, NONULL(ptr->pattern));
        break;
      }

      if (option(OPT_ATTACH_MSG) &&
          (mutt_strcmp(MuttVars[idx].option, "reply_regexp") == 0))
      {
        snprintf(err->data, err->dsize,
                 "Operation not permitted when in attach-message mode.");
        r = -1;
        break;
      }

      CHECK_PAGER;
      s->dptr++;

      /* copy the value of the string */
      mutt_extract_token(tmp, s, 0);

      if (parse_regex(idx, tmp, err))
        /* $reply_regexp and $alternates require special treatment */
        if (Context && Context->msgcount &&
            (mutt_strcmp(MuttVars[idx].option, "reply_regexp") == 0))
        {
          regmatch_t pmatch[1];

          for (int i = 0; i < Context->msgcount; i++)
          {
            struct Envelope *e = Context->hdrs[i]->env;
            if (e && e->subject)
            {
              e->real_subj =
                  (regexec(ReplyRegexp.regex, e->subject, 1, pmatch, 0)) ?
                      e->subject :
                      e->subject + pmatch[0].rm_eo;
            }
          }
        }
    }
    else if (DTYPE(MuttVars[idx].type) == DT_MAGIC)
    {
      if (query || *s->dptr != '=')
      {
        switch (MboxType)
        {
          case MUTT_MBOX:
            p = "mbox";
            break;
          case MUTT_MMDF:
            p = "MMDF";
            break;
          case MUTT_MH:
            p = "MH";
            break;
          case MUTT_MAILDIR:
            p = "Maildir";
            break;
          default:
            p = "unknown";
            break;
        }
        snprintf(err->data, err->dsize, "%s=%s", MuttVars[idx].option, p);
        break;
      }

      CHECK_PAGER;
      s->dptr++;

      /* copy the value of the string */
      mutt_extract_token(tmp, s, 0);
      if (mx_set_magic(tmp->data))
      {
        snprintf(err->data, err->dsize, _("%s: invalid mailbox type"), tmp->data);
        r = -1;
        break;
      }
    }
    else if (DTYPE(MuttVars[idx].type) == DT_NUMBER)
    {
      short *ptr = (short *) MuttVars[idx].data;
      short val;
      int rc;

      if (query || *s->dptr != '=')
      {
        val = *ptr;
        /* compatibility alias */
        if (mutt_strcmp(MuttVars[idx].option, "wrapmargin") == 0)
          val = *ptr < 0 ? -*ptr : 0;

        /* user requested the value of this variable */
        snprintf(err->data, err->dsize, "%s=%d", MuttVars[idx].option, val);
        break;
      }

      CHECK_PAGER;
      s->dptr++;

      mutt_extract_token(tmp, s, 0);
      rc = mutt_atos(tmp->data, (short *) &val);

      if (rc < 0 || !*tmp->data)
      {
        snprintf(err->data, err->dsize, _("%s: invalid value (%s)"), tmp->data,
                 rc == -1 ? _("format error") : _("number overflow"));
        r = -1;
        break;
      }
#ifdef DEBUG
      else if (mutt_strcmp(MuttVars[idx].option, "debug_level") == 0 && debuglevel_cmdline)
      {
        mutt_message(
            _("set debug_level ignored, it have been overridden with cmdline"));
        break;
      }
#endif
      else
        *ptr = val;

      /* these ones need a sanity check */
      if (mutt_strcmp(MuttVars[idx].option, "history") == 0)
      {
        if (*ptr < 0)
          *ptr = 0;
        mutt_init_history();
      }
#ifdef DEBUG
      else if (mutt_strcmp(MuttVars[idx].option, "debug_level") == 0)
      {
        if (*ptr < 0)
          *ptr = 0;
        restart_debug();
      }
#endif
      else if (mutt_strcmp(MuttVars[idx].option, "pager_index_lines") == 0)
      {
        if (*ptr < 0)
          *ptr = 0;
      }
      else if (mutt_strcmp(MuttVars[idx].option, "wrapmargin") == 0)
      {
        if (*ptr < 0)
          *ptr = 0;
        else
          *ptr = -*ptr;
      }
#ifdef USE_IMAP
      else if (mutt_strcmp(MuttVars[idx].option, "imap_pipeline_depth") == 0)
      {
        if (*ptr < 0)
          *ptr = 0;
      }
#endif
    }
    else if (DTYPE(MuttVars[idx].type) == DT_QUAD)
    {
      if (query)
      {
        static const char *const vals[] = { "no", "yes", "ask-no", "ask-yes" };

        snprintf(err->data, err->dsize, "%s=%s", MuttVars[idx].option,
                 vals[quadoption(MuttVars[idx].data)]);
        break;
      }

      CHECK_PAGER;
      if (*s->dptr == '=')
      {
        s->dptr++;
        mutt_extract_token(tmp, s, 0);
        if (mutt_strcasecmp("yes", tmp->data) == 0)
          set_quadoption(MuttVars[idx].data, MUTT_YES);
        else if (mutt_strcasecmp("no", tmp->data) == 0)
          set_quadoption(MuttVars[idx].data, MUTT_NO);
        else if (mutt_strcasecmp("ask-yes", tmp->data) == 0)
          set_quadoption(MuttVars[idx].data, MUTT_ASKYES);
        else if (mutt_strcasecmp("ask-no", tmp->data) == 0)
          set_quadoption(MuttVars[idx].data, MUTT_ASKNO);
        else
        {
          snprintf(err->data, err->dsize, _("%s: invalid value"), tmp->data);
          r = -1;
          break;
        }
      }
      else
      {
        if (inv)
          toggle_quadoption(MuttVars[idx].data);
        else if (unset)
          set_quadoption(MuttVars[idx].data, MUTT_NO);
        else
          set_quadoption(MuttVars[idx].data, MUTT_YES);
      }
    }
    else if (DTYPE(MuttVars[idx].type) == DT_SORT)
    {
      const struct Mapping *map = NULL;

      switch (MuttVars[idx].type & DT_SUBTYPE_MASK)
      {
        case DT_SORT_ALIAS:
          map = SortAliasMethods;
          break;
        case DT_SORT_BROWSER:
          map = SortBrowserMethods;
          break;
        case DT_SORT_KEYS:
          if ((WithCrypto & APPLICATION_PGP))
            map = SortKeyMethods;
          break;
        case DT_SORT_AUX:
          map = SortAuxMethods;
          break;
        case DT_SORT_SIDEBAR:
          map = SortSidebarMethods;
          break;
        default:
          map = SortMethods;
          break;
      }

      if (!map)
      {
        snprintf(err->data, err->dsize, _("%s: Unknown type."), MuttVars[idx].option);
        r = -1;
        break;
      }

      if (query || *s->dptr != '=')
      {
        p = mutt_getnamebyvalue(*((short *) MuttVars[idx].data) & SORT_MASK, map);

        snprintf(err->data, err->dsize, "%s=%s%s%s", MuttVars[idx].option,
                 (*((short *) MuttVars[idx].data) & SORT_REVERSE) ? "reverse-" : "",
                 (*((short *) MuttVars[idx].data) & SORT_LAST) ? "last-" : "", p);
        return 0;
      }
      CHECK_PAGER;
      s->dptr++;
      mutt_extract_token(tmp, s, 0);

      if (parse_sort((short *) MuttVars[idx].data, tmp->data, map, err) == -1)
      {
        r = -1;
        break;
      }
    }
#ifdef USE_HCACHE
    else if (DTYPE(MuttVars[idx].type) == DT_HCACHE)
    {
      if (query || (*s->dptr != '='))
      {
        pretty_var(err->data, err->dsize, MuttVars[idx].option,
                   NONULL((*(char **) MuttVars[idx].data)));
        break;
      }

      CHECK_PAGER;
      s->dptr++;

      /* copy the value of the string */
      mutt_extract_token(tmp, s, 0);
      if (mutt_hcache_is_valid_backend(tmp->data))
      {
        FREE((void *) MuttVars[idx].data);
        *(char **) (MuttVars[idx].data) = safe_strdup(tmp->data);
      }
      else
      {
        snprintf(err->data, err->dsize, _("%s: invalid backend"), tmp->data);
        r = -1;
        break;
      }
    }
#endif
    else
    {
      snprintf(err->data, err->dsize, _("%s: unknown type"), MuttVars[idx].option);
      r = -1;
      break;
    }

    if (!myvar)
    {
      if (MuttVars[idx].flags & R_INDEX)
        mutt_set_menu_redraw_full(MENU_MAIN);
      if (MuttVars[idx].flags & R_PAGER)
        mutt_set_menu_redraw_full(MENU_PAGER);
      if (MuttVars[idx].flags & R_PAGER_FLOW)
      {
        mutt_set_menu_redraw_full(MENU_PAGER);
        mutt_set_menu_redraw(MENU_PAGER, REDRAW_FLOW);
      }
      if (MuttVars[idx].flags & R_RESORT_SUB)
        set_option(OPT_SORT_SUBTHREADS);
      if (MuttVars[idx].flags & R_RESORT)
        set_option(OPT_NEED_RESORT);
      if (MuttVars[idx].flags & R_RESORT_INIT)
        set_option(OPT_RESORT_INIT);
      if (MuttVars[idx].flags & R_TREE)
        set_option(OPT_REDRAW_TREE);
      if (MuttVars[idx].flags & R_REFLOW)
        mutt_reflow_windows();
#ifdef USE_SIDEBAR
      if (MuttVars[idx].flags & R_SIDEBAR)
        mutt_set_current_menu_redraw(REDRAW_SIDEBAR);
#endif
      if (MuttVars[idx].flags & R_MENU)
        mutt_set_current_menu_redraw_full();
    }
  }
  return r;
}

/* FILO designed to contain the list of config files that have been sourced and
 * avoid cyclic sourcing */
static struct ListHead MuttrcStack = STAILQ_HEAD_INITIALIZER(MuttrcStack);

/**
 * to_absolute_path - Convert relative filepath to an absolute path
 * @param path      Relative path
 * @param reference Absolute path that \a path is relative to
 * @retval true on success
 * @retval false otherwise
 *
 * Use POSIX functions to convert a path to absolute, relatively to another path
 * @note \a path should be at least of PATH_MAX length
 */
static int to_absolute_path(char *path, const char *reference)
{
  char *ref_tmp = NULL, *dirpath = NULL;
  char abs_path[PATH_MAX];
  int path_len;

  /* if path is already absolute, don't do anything */
  if ((strlen(path) > 1) && (path[0] == '/'))
  {
    return true;
  }

  ref_tmp = safe_strdup(reference);
  dirpath = dirname(ref_tmp); /* get directory name of */
  strfcpy(abs_path, dirpath, PATH_MAX);
  safe_strncat(abs_path, sizeof(abs_path), "/", 1); /* append a / at the end of the path */

  FREE(&ref_tmp);
  path_len = PATH_MAX - strlen(path);

  safe_strncat(abs_path, sizeof(abs_path), path, path_len > 0 ? path_len : 0);

  path = realpath(abs_path, path);

  if (!path)
  {
    printf("Error: issue converting path to absolute (%s)", strerror(errno));
    return false;
  }

  return true;
}

#define MAXERRS 128

/**
 * source_rc - Read an initialization file
 * @param rcfile_path Path to initialization file
 * @param err         Buffer for error messages
 * @retval <0 if neomutt should pause to let the user know
 */
static int source_rc(const char *rcfile_path, struct Buffer *err)
{
  FILE *f = NULL;
  int line = 0, rc = 0, conv = 0, line_rc, warnings = 0;
  struct Buffer token;
  char *linebuf = NULL;
  char *currentline = NULL;
  char rcfile[PATH_MAX];
  size_t buflen;
  size_t rcfilelen;
  bool ispipe;

  pid_t pid;

  strfcpy(rcfile, rcfile_path, PATH_MAX);

  rcfilelen = mutt_strlen(rcfile);
  if (rcfilelen == 0)
    return -1;

  ispipe = rcfile[rcfilelen - 1] == '|';

  if (!ispipe)
  {
    struct ListNode *np = STAILQ_FIRST(&MuttrcStack);
    if (!to_absolute_path(rcfile, np ? NONULL(np->data) : ""))
    {
      mutt_error("Error: impossible to build path of '%s'.", rcfile_path);
      return -1;
    }

    STAILQ_FOREACH(np, &MuttrcStack, entries)
    {
      if (mutt_strcmp(np->data, rcfile) == 0)
      {
        break;
      }
    }
    if (!np)
    {
      mutt_list_insert_head(&MuttrcStack, safe_strdup(rcfile));
    }
    else
    {
      mutt_error("Error: Cyclic sourcing of configuration file '%s'.", rcfile);
      return -1;
    }
  }

  mutt_debug(2, "Reading configuration file '%s'.\n", rcfile);

  f = mutt_open_read(rcfile, &pid);
  if (!f)
  {
    snprintf(err->data, err->dsize, "%s: %s", rcfile, strerror(errno));
    return -1;
  }

  mutt_buffer_init(&token);
  while ((linebuf = mutt_read_line(linebuf, &buflen, f, &line, MUTT_CONT)) != NULL)
  {
    conv = ConfigCharset && (*ConfigCharset) && Charset;
    if (conv)
    {
      currentline = safe_strdup(linebuf);
      if (!currentline)
        continue;
      mutt_convert_string(&currentline, ConfigCharset, Charset, 0);
    }
    else
      currentline = linebuf;

    line_rc = mutt_parse_rc_line(currentline, &token, err);
    if (line_rc == -1)
    {
      mutt_error(_("Error in %s, line %d: %s"), rcfile, line, err->data);
      if (--rc < -MAXERRS)
      {
        if (conv)
          FREE(&currentline);
        break;
      }
    }
    else if (line_rc == -2)
    {
      /* Warning */
      mutt_error(_("Warning in %s, line %d: %s"), rcfile, line, err->data);
      warnings++;
    }
    else if (line_rc == 1)
    {
      break; /* Found "finish" command */
    }
    else
    {
      if (rc < 0)
        rc = -1;
    }
    if (conv)
      FREE(&currentline);
  }
  FREE(&token.data);
  FREE(&linebuf);
  safe_fclose(&f);
  if (pid != -1)
    mutt_wait_filter(pid);
  if (rc)
  {
    /* the neomuttrc source keyword */
    snprintf(err->data, err->dsize,
             rc >= -MAXERRS ?
                 _("source: errors in %s") :
                 _("source: reading aborted due to too many errors in %s"),
             rcfile);
    rc = -1;
  }
  else
  {
    /* Don't alias errors with warnings */
    if (warnings > 0)
    {
      snprintf(err->data, err->dsize, _("source: %d warnings in %s"), warnings, rcfile);
      rc = -2;
    }
  }

  if (!ispipe && !STAILQ_EMPTY(&MuttrcStack))
  {
    STAILQ_REMOVE_HEAD(&MuttrcStack, entries);
  }

  return rc;
}

#undef MAXERRS

static int parse_source(struct Buffer *tmp, struct Buffer *token,
                        unsigned long data, struct Buffer *err)
{
  char path[_POSIX_PATH_MAX];

  do
  {
    if (mutt_extract_token(tmp, token, 0) != 0)
    {
      snprintf(err->data, err->dsize, _("source: error at %s"), token->dptr);
      return -1;
    }
    strfcpy(path, tmp->data, sizeof(path));
    mutt_expand_path(path, sizeof(path));

    if (source_rc(path, err) < 0)
    {
      snprintf(err->data, err->dsize,
               _("source: file %s could not be sourced."), path);
      return -1;
    }

  } while (MoreArgs(token));

  return 0;
}

/**
 * mutt_parse_rc_line - Parse a line of user config
 * @param line  config line to read
 * @param token scratch buffer to be used by parser
 * @param err   where to write error messages
 *
 * Caller should free token->data when finished.  the reason for this variable
 * is to avoid having to allocate and deallocate a lot of memory if we are
 * parsing many lines.  the caller can pass in the memory to use, which avoids
 * having to create new space for every call to this function.
 */
int mutt_parse_rc_line(/* const */ char *line, struct Buffer *token, struct Buffer *err)
{
  int i, r = 0;
  struct Buffer expn;

  if (!line || !*line)
    return 0;

  mutt_buffer_init(&expn);
  expn.data = expn.dptr = line;
  expn.dsize = mutt_strlen(line);

  *err->data = 0;

  SKIPWS(expn.dptr);
  while (*expn.dptr)
  {
    if (*expn.dptr == '#')
      break; /* rest of line is a comment */
    if (*expn.dptr == ';')
    {
      expn.dptr++;
      continue;
    }
    mutt_extract_token(token, &expn, 0);
    for (i = 0; Commands[i].name; i++)
    {
      if (mutt_strcmp(token->data, Commands[i].name) == 0)
      {
        r = Commands[i].func(token, &expn, Commands[i].data, err);
        if (r != 0)
        {              /* -1 Error, +1 Finish */
          goto finish; /* Propagate return code */
        }
        break; /* Continue with next command */
      }
    }
    if (!Commands[i].name)
    {
      snprintf(err->data, err->dsize, _("%s: unknown command"), NONULL(token->data));
      r = -1;
      break; /* Ignore the rest of the line */
    }
  }
finish:
  if (expn.destroy)
    FREE(&expn.data);
  return r;
}

#define NUMVARS mutt_array_size(MuttVars)
#define NUMCOMMANDS mutt_array_size(Commands)

/* initial string that starts completion. No telling how much crap
 * the user has typed so far. Allocate LONG_STRING just to be sure! */
static char User_typed[LONG_STRING] = { 0 };

static int Num_matched = 0;            /* Number of matches for completion */
static char Completed[STRING] = { 0 }; /* completed string (command or variable) */
static const char **Matches;
/* this is a lie until mutt_init runs: */
static int Matches_listsize = MAX(NUMVARS, NUMCOMMANDS) + 10;

static void matches_ensure_morespace(int current)
{
  int base_space, extra_space, space;

  if (current > Matches_listsize - 2)
  {
    base_space = MAX(NUMVARS, NUMCOMMANDS) + 1;
    extra_space = Matches_listsize - base_space;
    extra_space *= 2;
    space = base_space + extra_space;
    safe_realloc(&Matches, space * sizeof(char *));
    memset(&Matches[current + 1], 0, space - current);
    Matches_listsize = space;
  }
}

/**
 * candidate - helper function for completion
 * @param dest Completion result gets here
 * @param src  Candidate for completion
 * @param try  User entered data for completion
 * @param len  Length of dest buffer
 *
 * Changes the dest buffer if necessary/possible to aid completion.
*/
static void candidate(char *dest, char *try, const char *src, int len)
{
  if (!dest || !try || !src)
    return;

  if (strstr(src, try) == src)
  {
    matches_ensure_morespace(Num_matched);
    Matches[Num_matched++] = src;
    if (dest[0] == 0)
      strfcpy(dest, src, len);
    else
    {
      int l;
      for (l = 0; src[l] && src[l] == dest[l]; l++)
        ;
      dest[l] = '\0';
    }
  }
}

#ifdef USE_LUA
const struct Command *mutt_command_get(const char *s)
{
  for (int i = 0; Commands[i].name; i++)
    if (mutt_strcmp(s, Commands[i].name) == 0)
      return &Commands[i];
  return NULL;
}
#endif

void mutt_commands_apply(void *data, void (*application)(void *, const struct Command *))
{
  for (int i = 0; Commands[i].name; i++)
    application(data, &Commands[i]);
}

int mutt_command_complete(char *buffer, size_t len, int pos, int numtabs)
{
  char *pt = buffer;
  int num;
  int spaces; /* keep track of the number of leading spaces on the line */
  struct MyVar *myv = NULL;

  SKIPWS(buffer);
  spaces = buffer - pt;

  pt = buffer + pos - spaces;
  while ((pt > buffer) && !isspace((unsigned char) *pt))
    pt--;

  if (pt == buffer) /* complete cmd */
  {
    /* first TAB. Collect all the matches */
    if (numtabs == 1)
    {
      Num_matched = 0;
      strfcpy(User_typed, pt, sizeof(User_typed));
      memset(Matches, 0, Matches_listsize);
      memset(Completed, 0, sizeof(Completed));
      for (num = 0; Commands[num].name; num++)
        candidate(Completed, User_typed, Commands[num].name, sizeof(Completed));
      matches_ensure_morespace(Num_matched);
      Matches[Num_matched++] = User_typed;

      /* All matches are stored. Longest non-ambiguous string is ""
       * i.e. don't change 'buffer'. Fake successful return this time */
      if (User_typed[0] == 0)
        return 1;
    }

    if (Completed[0] == 0 && User_typed[0])
      return 0;

    /* Num_matched will _always_ be at least 1 since the initial
      * user-typed string is always stored */
    if (numtabs == 1 && Num_matched == 2)
      snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
    else if (numtabs > 1 && Num_matched > 2)
      /* cycle thru all the matches */
      snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

    /* return the completed command */
    strncpy(buffer, Completed, len - spaces);
  }
  else if ((mutt_strncmp(buffer, "set", 3) == 0) ||
           (mutt_strncmp(buffer, "unset", 5) == 0) ||
           (mutt_strncmp(buffer, "reset", 5) == 0) ||
           (mutt_strncmp(buffer, "toggle", 6) == 0))
  { /* complete variables */
    static const char *const prefixes[] = { "no", "inv", "?", "&", 0 };

    pt++;
    /* loop through all the possible prefixes (no, inv, ...) */
    if (mutt_strncmp(buffer, "set", 3) == 0)
    {
      for (num = 0; prefixes[num]; num++)
      {
        if (mutt_strncmp(pt, prefixes[num], mutt_strlen(prefixes[num])) == 0)
        {
          pt += mutt_strlen(prefixes[num]);
          break;
        }
      }
    }

    /* first TAB. Collect all the matches */
    if (numtabs == 1)
    {
      Num_matched = 0;
      strfcpy(User_typed, pt, sizeof(User_typed));
      memset(Matches, 0, Matches_listsize);
      memset(Completed, 0, sizeof(Completed));
      for (num = 0; MuttVars[num].option; num++)
        candidate(Completed, User_typed, MuttVars[num].option, sizeof(Completed));
      for (myv = MyVars; myv; myv = myv->next)
        candidate(Completed, User_typed, myv->name, sizeof(Completed));
      matches_ensure_morespace(Num_matched);
      Matches[Num_matched++] = User_typed;

      /* All matches are stored. Longest non-ambiguous string is ""
       * i.e. don't change 'buffer'. Fake successful return this time */
      if (User_typed[0] == 0)
        return 1;
    }

    if (Completed[0] == 0 && User_typed[0])
      return 0;

    /* Num_matched will _always_ be at least 1 since the initial
     * user-typed string is always stored */
    if (numtabs == 1 && Num_matched == 2)
      snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
    else if (numtabs > 1 && Num_matched > 2)
      /* cycle thru all the matches */
      snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

    strncpy(pt, Completed, buffer + len - pt - spaces);
  }
  else if (mutt_strncmp(buffer, "exec", 4) == 0)
  {
    const struct Binding *menu = km_get_table(CurrentMenu);

    if (!menu && CurrentMenu != MENU_PAGER)
      menu = OpGeneric;

    pt++;
    /* first TAB. Collect all the matches */
    if (numtabs == 1)
    {
      Num_matched = 0;
      strfcpy(User_typed, pt, sizeof(User_typed));
      memset(Matches, 0, Matches_listsize);
      memset(Completed, 0, sizeof(Completed));
      for (num = 0; menu[num].name; num++)
        candidate(Completed, User_typed, menu[num].name, sizeof(Completed));
      /* try the generic menu */
      if (Completed[0] == 0 && CurrentMenu != MENU_PAGER)
      {
        menu = OpGeneric;
        for (num = 0; menu[num].name; num++)
          candidate(Completed, User_typed, menu[num].name, sizeof(Completed));
      }
      matches_ensure_morespace(Num_matched);
      Matches[Num_matched++] = User_typed;

      /* All matches are stored. Longest non-ambiguous string is ""
       * i.e. don't change 'buffer'. Fake successful return this time */
      if (User_typed[0] == 0)
        return 1;
    }

    if (Completed[0] == 0 && User_typed[0])
      return 0;

    /* Num_matched will _always_ be at least 1 since the initial
     * user-typed string is always stored */
    if (numtabs == 1 && Num_matched == 2)
      snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
    else if (numtabs > 1 && Num_matched > 2)
      /* cycle thru all the matches */
      snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

    strncpy(pt, Completed, buffer + len - pt - spaces);
  }
  else
    return 0;

  return 1;
}

int mutt_var_value_complete(char *buffer, size_t len, int pos)
{
  char var[STRING], *pt = buffer;
  int spaces;

  if (buffer[0] == 0)
    return 0;

  SKIPWS(buffer);
  spaces = buffer - pt;

  pt = buffer + pos - spaces;
  while ((pt > buffer) && !isspace((unsigned char) *pt))
    pt--;
  pt++;           /* move past the space */
  if (*pt == '=') /* abort if no var before the '=' */
    return 0;

  if (mutt_strncmp(buffer, "set", 3) == 0)
  {
    int idx;
    char val[LONG_STRING];
    const char *myvarval = NULL;

    strfcpy(var, pt, sizeof(var));
    /* ignore the trailing '=' when comparing */
    int vlen = mutt_strlen(var);
    if (vlen == 0)
      return 0;

    var[vlen - 1] = '\0';
    idx = mutt_option_index(var);
    if (idx == -1)
    {
      myvarval = myvar_get(var);
      if (myvarval)
      {
        pretty_var(pt, len - (pt - buffer), var, myvarval);
        return 1;
      }
      return 0; /* no such variable. */
    }
    else if (var_to_string(idx, val, sizeof(val)))
    {
      snprintf(pt, len - (pt - buffer), "%s=\"%s\"", var, val);
      return 1;
    }
  }
  return 0;
}

#ifdef USE_NOTMUCH

/**
 * complete_all_nm_tags - Pass a list of notmuch tags to the completion code
 */
static int complete_all_nm_tags(const char *pt)
{
  int tag_count_1 = 0;
  int tag_count_2 = 0;

  Num_matched = 0;
  strfcpy(User_typed, pt, sizeof(User_typed));
  memset(Matches, 0, Matches_listsize);
  memset(Completed, 0, sizeof(Completed));

  nm_longrun_init(Context, false);

  /* Work out how many tags there are. */
  if (nm_get_all_tags(Context, NULL, &tag_count_1) || tag_count_1 == 0)
    goto done;

  /* Free the old list, if any. */
  if (nm_tags)
  {
    for (int i = 0; nm_tags[i] != NULL; i++)
      FREE(&nm_tags[i]);
    FREE(&nm_tags);
  }
  /* Allocate a new list, with sentinel. */
  nm_tags = safe_malloc((tag_count_1 + 1) * sizeof(char *));
  nm_tags[tag_count_1] = NULL;

  /* Get all the tags. */
  if (nm_get_all_tags(Context, nm_tags, &tag_count_2) || tag_count_1 != tag_count_2)
  {
    FREE(&nm_tags);
    nm_tags = NULL;
    nm_longrun_done(Context);
    return -1;
  }

  /* Put them into the completion machinery. */
  for (int num = 0; num < tag_count_1; num++)
  {
    candidate(Completed, User_typed, nm_tags[num], sizeof(Completed));
  }

  matches_ensure_morespace(Num_matched);
  Matches[Num_matched++] = User_typed;

done:
  nm_longrun_done(Context);
  return 0;
}

/**
 * mutt_nm_query_complete - Complete to the nearest notmuch tag
 *
 * Complete the nearest "tag:"-prefixed string previous to pos.
 */
bool mutt_nm_query_complete(char *buffer, size_t len, int pos, int numtabs)
{
  char *pt = buffer;
  int spaces;

  SKIPWS(buffer);
  spaces = buffer - pt;

  pt = (char *) rstrnstr((char *) buffer, pos, "tag:");
  if (pt)
  {
    pt += 4;
    if (numtabs == 1)
    {
      /* First TAB. Collect all the matches */
      complete_all_nm_tags(pt);

      /* All matches are stored. Longest non-ambiguous string is ""
       * i.e. don't change 'buffer'. Fake successful return this time.
       */
      if (User_typed[0] == 0)
        return true;
    }

    if (Completed[0] == 0 && User_typed[0])
      return false;

    /* Num_matched will _always_ be at least 1 since the initial
     * user-typed string is always stored */
    if (numtabs == 1 && Num_matched == 2)
      snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
    else if (numtabs > 1 && Num_matched > 2)
      /* cycle thru all the matches */
      snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

    /* return the completed query */
    strncpy(pt, Completed, buffer + len - pt - spaces);
  }
  else
    return false;

  return true;
}

/**
 * mutt_nm_tag_complete - Complete to the nearest notmuch tag
 *
 * Complete the nearest "+" or "-" -prefixed string previous to pos.
 */
bool mutt_nm_tag_complete(char *buffer, size_t len, int pos, int numtabs)
{
  if (!buffer)
    return false;

  char *pt = buffer;

  /* Only examine the last token */
  char *last_space = strrchr(buffer, ' ');
  if (last_space)
    pt = (last_space + 1);

  /* Skip the +/- */
  if ((pt[0] == '+') || (pt[0] == '-'))
    pt++;

  if (numtabs == 1)
  {
    /* First TAB. Collect all the matches */
    complete_all_nm_tags(pt);

    /* All matches are stored. Longest non-ambiguous string is ""
      * i.e. don't change 'buffer'. Fake successful return this time.
      */
    if (User_typed[0] == 0)
      return true;
  }

  if (Completed[0] == 0 && User_typed[0])
    return false;

  /* Num_matched will _always_ be at least 1 since the initial
    * user-typed string is always stored */
  if (numtabs == 1 && Num_matched == 2)
    snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
  else if (numtabs > 1 && Num_matched > 2)
    /* cycle thru all the matches */
    snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

  /* return the completed query */
  strncpy(pt, Completed, buffer + len - pt);

  return true;
}
#endif

int var_to_string(int idx, char *val, size_t len)
{
  char tmp[LONG_STRING];
  static const char *const vals[] = { "no", "yes", "ask-no", "ask-yes" };

  tmp[0] = '\0';

  if ((DTYPE(MuttVars[idx].type) == DT_STRING) ||
      (DTYPE(MuttVars[idx].type) == DT_PATH) || (DTYPE(MuttVars[idx].type) == DT_REGEX))
  {
    strfcpy(tmp, NONULL(*((char **) MuttVars[idx].data)), sizeof(tmp));
    if (DTYPE(MuttVars[idx].type) == DT_PATH)
      mutt_pretty_mailbox(tmp, sizeof(tmp));
  }
  else if (DTYPE(MuttVars[idx].type) == DT_MBTABLE)
  {
    struct MbTable *mbt = (*((struct MbTable **) MuttVars[idx].data));
    strfcpy(tmp, mbt ? NONULL(mbt->orig_str) : "", sizeof(tmp));
  }
  else if (DTYPE(MuttVars[idx].type) == DT_ADDRESS)
  {
    rfc822_write_address(tmp, sizeof(tmp), *((struct Address **) MuttVars[idx].data), 0);
  }
  else if (DTYPE(MuttVars[idx].type) == DT_QUAD)
    strfcpy(tmp, vals[quadoption(MuttVars[idx].data)], sizeof(tmp));
  else if (DTYPE(MuttVars[idx].type) == DT_NUMBER)
  {
    short sval = *((short *) MuttVars[idx].data);

    /* avert your eyes, gentle reader */
    if (mutt_strcmp(MuttVars[idx].option, "wrapmargin") == 0)
      sval = sval > 0 ? 0 : -sval;

    snprintf(tmp, sizeof(tmp), "%d", sval);
  }
  else if (DTYPE(MuttVars[idx].type) == DT_SORT)
  {
    const struct Mapping *map = NULL;
    const char *p = NULL;

    switch (MuttVars[idx].type & DT_SUBTYPE_MASK)
    {
      case DT_SORT_ALIAS:
        map = SortAliasMethods;
        break;
      case DT_SORT_BROWSER:
        map = SortBrowserMethods;
        break;
      case DT_SORT_KEYS:
        if ((WithCrypto & APPLICATION_PGP))
          map = SortKeyMethods;
        else
          map = SortMethods;
        break;
      default:
        map = SortMethods;
        break;
    }
    p = mutt_getnamebyvalue(*((short *) MuttVars[idx].data) & SORT_MASK, map);
    snprintf(tmp, sizeof(tmp), "%s%s%s",
             (*((short *) MuttVars[idx].data) & SORT_REVERSE) ? "reverse-" : "",
             (*((short *) MuttVars[idx].data) & SORT_LAST) ? "last-" : "", p);
  }
  else if (DTYPE(MuttVars[idx].type) == DT_MAGIC)
  {
    char *p = NULL;

    switch (MboxType)
    {
      case MUTT_MBOX:
        p = "mbox";
        break;
      case MUTT_MMDF:
        p = "MMDF";
        break;
      case MUTT_MH:
        p = "MH";
        break;
      case MUTT_MAILDIR:
        p = "Maildir";
        break;
      default:
        p = "unknown";
    }
    strfcpy(tmp, p, sizeof(tmp));
  }
  else if (DTYPE(MuttVars[idx].type) == DT_BOOL)
    strfcpy(tmp, option(MuttVars[idx].data) ? "yes" : "no", sizeof(tmp));
  else
    return 0;

  escape_string(val, len - 1, tmp);

  return 1;
}

/**
 * mutt_query_variables - Implement the -Q command line flag
 */
int mutt_query_variables(struct ListHead *queries)
{
  char command[STRING];

  struct Buffer err, token;

  mutt_buffer_init(&err);
  mutt_buffer_init(&token);

  err.dsize = STRING;
  err.data = safe_malloc(err.dsize);

  struct ListNode *np;
  STAILQ_FOREACH(np, queries, entries)
  {
    snprintf(command, sizeof(command), "set ?%s\n", np->data);
    if (mutt_parse_rc_line(command, &token, &err) == -1)
    {
      fprintf(stderr, "%s\n", err.data);
      FREE(&token.data);
      FREE(&err.data);

      return 1;
    }
    printf("%s\n", err.data);
  }

  FREE(&token.data);
  FREE(&err.data);

  return 0;
}

/**
 * mutt_dump_variables - Print a list of all variables with their values
 */
int mutt_dump_variables(int hide_sensitive)
{
  char command[STRING];

  struct Buffer err, token;

  mutt_buffer_init(&err);
  mutt_buffer_init(&token);

  err.dsize = STRING;
  err.data = safe_malloc(err.dsize);

  for (int i = 0; MuttVars[i].option; i++)
  {
    if (MuttVars[i].type == DT_SYNONYM)
      continue;

    if (hide_sensitive && IS_SENSITIVE(MuttVars[i]))
    {
      printf("%s='***'\n", MuttVars[i].option);
      continue;
    }
    snprintf(command, sizeof(command), "set ?%s\n", MuttVars[i].option);
    if (mutt_parse_rc_line(command, &token, &err) == -1)
    {
      fprintf(stderr, "%s\n", err.data);
      FREE(&token.data);
      FREE(&err.data);

      return 1;
    }
    printf("%s\n", err.data);
  }

  FREE(&token.data);
  FREE(&err.data);

  return 0;
}

static int execute_commands(struct ListHead *p)
{
  struct Buffer err, token;

  mutt_buffer_init(&err);
  err.dsize = STRING;
  err.data = safe_malloc(err.dsize);
  mutt_buffer_init(&token);
  struct ListNode *np;
  STAILQ_FOREACH(np, p, entries)
  {
    if (mutt_parse_rc_line(np->data, &token, &err) == -1)
    {
      fprintf(stderr, _("Error in command line: %s\n"), err.data);
      FREE(&token.data);
      FREE(&err.data);

      return -1;
    }
  }
  FREE(&token.data);
  FREE(&err.data);

  return 0;
}

static char *find_cfg(const char *home, const char *xdg_cfg_home)
{
  const char *names[] = {
    "neomuttrc", "muttrc", NULL,
  };

  const char *locations[][2] = {
    { xdg_cfg_home, "neomutt/" },
    { xdg_cfg_home, "mutt/" },
    { home, ".neomutt/" },
    { home, ".mutt/" },
    { home, "." },
    { NULL, NULL },
  };

  for (int i = 0; locations[i][0] || locations[i][1]; i++)
  {
    if (!locations[i][0])
      continue;

    for (int j = 0; names[j]; j++)
    {
      char buffer[STRING];

      snprintf(buffer, sizeof(buffer), "%s/%s%s", locations[i][0],
               locations[i][1], names[j]);
      if (access(buffer, F_OK) == 0)
        return safe_strdup(buffer);
    }
  }

  return NULL;
}

int getmailname(char *s, size_t l)
{
  FILE *f;
  char tmp[512];
  char *p = tmp;

  if ((f = fopen("/etc/mailname", "r")) == NULL)
    return (-1);

  if (fgets(tmp, 510, f) != NULL)
  {
    while (*p && !ISSPACE(*p) && l > 0)
    {
      *s++ = *p++;
      l--;
    }
    if (*(s - 1) == '.')
      s--;
    *s = 0;

    fclose(f);
    return 0;
  }
  fclose(f);
  return (-1);
}

void mutt_init(int skip_sys_rc, struct ListHead *commands)
{
  struct passwd *pw = NULL;
  struct utsname utsname;
  char *p, buffer[STRING];
  int need_pause = 0;
  struct Buffer err;

  mutt_buffer_init(&err);
  err.dsize = STRING;
  err.data = safe_malloc(err.dsize);
  err.dptr = err.data;

  Groups = hash_create(1031, 0);
  /* reverse alias keys need to be strdup'ed because of idna conversions */
  ReverseAliases =
      hash_create(1031, MUTT_HASH_STRCASECMP | MUTT_HASH_STRDUP_KEYS | MUTT_HASH_ALLOW_DUPS);
  TagTransforms = hash_create(64, 1);
  TagFormats = hash_create(64, 0);

  mutt_menu_init();

  snprintf(AttachmentMarker, sizeof(AttachmentMarker), "\033]9;%" PRIu64 "\a",
           mutt_rand64());

  /* on one of the systems I use, getcwd() does not return the same prefix
     as is listed in the passwd file */
  if ((p = getenv("HOME")))
    HomeDir = safe_strdup(p);

  /* Get some information about the user */
  if ((pw = getpwuid(getuid())))
  {
    char rnbuf[STRING];

    Username = safe_strdup(pw->pw_name);
    if (!HomeDir)
      HomeDir = safe_strdup(pw->pw_dir);

    RealName = safe_strdup(mutt_gecos_name(rnbuf, sizeof(rnbuf), pw));
    Shell = safe_strdup(pw->pw_shell);
    endpwent();
  }
  else
  {
    if (!HomeDir)
    {
      mutt_endwin(NULL);
      fputs(_("unable to determine home directory"), stderr);
      exit(1);
    }
    if ((p = getenv("USER")))
      Username = safe_strdup(p);
    else
    {
      mutt_endwin(NULL);
      fputs(_("unable to determine username"), stderr);
      exit(1);
    }
    Shell = safe_strdup((p = getenv("SHELL")) ? p : "/bin/sh");
  }

#ifdef DEBUG
  /* Start up debugging mode if requested from cmdline */
  if (debuglevel_cmdline > 0)
  {
    debuglevel = debuglevel_cmdline;
    if (debugfile_cmdline)
    {
      DebugFile = safe_strdup(debugfile_cmdline);
    }
    else
    {
      int i = mutt_option_index("debug_file");
      if ((i >= 0) && (MuttVars[i].init != 0))
        DebugFile = safe_strdup((const char *) MuttVars[i].init);
    }
    start_debug();
  }
#endif

  /* And about the host... */

  /*
   * The call to uname() shouldn't fail, but if it does, the system is horribly
   * broken, and the system's networking configuration is in an unreliable
   * state.  We should bail.
   */
  if ((uname(&utsname)) == -1)
  {
    mutt_endwin(NULL);
    perror(_("unable to determine nodename via uname()"));
    exit(1);
  }

  /* some systems report the FQDN instead of just the hostname */
  if ((p = strchr(utsname.nodename, '.')))
    ShortHostname = mutt_substrdup(utsname.nodename, p);
  else
    ShortHostname = safe_strdup(utsname.nodename);

  /* now get FQDN.  Use /etc/mailname first, then configured domain, DNS next, then uname */
  if (getmailname(buffer, sizeof(buffer)) != -1)
    Hostname = safe_strdup(buffer);
#ifdef DOMAIN
  /* we have a compile-time domain name, use that for Hostname */
  if (!Hostname)
  {
    Hostname = safe_malloc(mutt_strlen(DOMAIN) + mutt_strlen(Hostname) + 2);
    sprintf(Hostname, "%s.%s", NONULL(Hostname), DOMAIN); /* __SPRINTF_CHECKED__ */
  }
  else
#else
  if (!(getdnsdomainname(buffer, sizeof(buffer))))
  {
    Hostname = safe_malloc(mutt_strlen(buffer) + mutt_strlen(ShortHostname) + 2);
    sprintf(Hostname, "%s.%s", NONULL(ShortHostname), buffer);
  }
  else
    /*
     * DNS failed, use the nodename.  Whether or not the nodename had a '.' in
     * it, we can use the nodename as the FQDN.  On hosts where DNS is not
     * being used, e.g. small network that relies on hosts files, a short host
     * name is all that is required for SMTP to work correctly.  It could be
     * wrong, but we've done the best we can, at this point the onus is on the
     * user to provide the correct hostname if the nodename won't work in their
     * network.
     */
    Hostname = safe_strdup(utsname.nodename);
#endif

#ifdef USE_NNTP
  if ((p = getenv("NNTPSERVER")))
  {
    FREE(&NewsServer);
    NewsServer = safe_strdup(p);
  }
  else
  {
    FILE *f = NULL;
    char *c = NULL;

    if ((f = safe_fopen(SYSCONFDIR "/nntpserver", "r")))
    {
      if (fgets(buffer, sizeof(buffer), f) == NULL)
        buffer[0] = '\0';
      p = buffer;
      SKIPWS(p);
      c = p;
      while (*c && (*c != ' ') && (*c != '\t') && (*c != '\r') && (*c != '\n'))
        c++;
      *c = '\0';
      NewsServer = safe_strdup(p);
      fclose(f);
    }
  }
#endif

  if ((p = getenv("MAIL")))
    SpoolFile = safe_strdup(p);
  else if ((p = getenv("MAILDIR")))
    SpoolFile = safe_strdup(p);
  else
  {
#ifdef HOMESPOOL
    mutt_concat_path(buffer, NONULL(HomeDir), MAILPATH, sizeof(buffer));
#else
    mutt_concat_path(buffer, MAILPATH, NONULL(Username), sizeof(buffer));
#endif
    SpoolFile = safe_strdup(buffer);
  }

  if ((p = getenv("MAILCAPS")))
    MailcapPath = safe_strdup(p);
  else
  {
    /* Default search path from RFC1524 */
    MailcapPath = safe_strdup(
        "~/.mailcap:" PKGDATADIR "/mailcap:" SYSCONFDIR
        "/mailcap:/etc/mailcap:/usr/etc/mailcap:/usr/local/etc/mailcap");
  }

  Tmpdir = safe_strdup((p = getenv("TMPDIR")) ? p : "/tmp");

  p = getenv("VISUAL");
  if (!p)
  {
    p = getenv("EDITOR");
    if (!p)
      p = "/usr/bin/editor";
  }
  Editor = safe_strdup(p);
  Visual = safe_strdup(p);

  p = getenv("REPLYTO");
  if (p)
  {
    struct Buffer buf, token;

    snprintf(buffer, sizeof(buffer), "Reply-To: %s", p);

    mutt_buffer_init(&buf);
    buf.data = buf.dptr = buffer;
    buf.dsize = mutt_strlen(buffer);

    mutt_buffer_init(&token);
    parse_my_hdr(&token, &buf, 0, &err);
    FREE(&token.data);
  }

  p = getenv("EMAIL");
  if (p)
    From = rfc822_parse_adrlist(NULL, p);

  mutt_set_langinfo_charset();
  mutt_set_charset(Charset);

  Matches = safe_calloc(Matches_listsize, sizeof(char *));

  /* Set standard defaults */
  for (int i = 0; MuttVars[i].option; i++)
  {
    set_default(&MuttVars[i]);
    restore_default(&MuttVars[i]);
  }

  CurrentMenu = MENU_MAIN;

#ifndef LOCALES_HACK
  /* Do we have a locale definition? */
  if (((p = getenv("LC_ALL")) != NULL && p[0]) || ((p = getenv("LANG")) != NULL && p[0]) ||
      ((p = getenv("LC_CTYPE")) != NULL && p[0]))
    set_option(OPT_LOCALES);
#endif

#ifdef HAVE_GETSID
  /* Unset suspend by default if we're the session leader */
  if (getsid(0) == getpid())
    unset_option(OPT_SUSPEND);
#endif

  mutt_init_history();

  /* RFC2368, "4. Unsafe headers"
   * The creator of a mailto URL cannot expect the resolver of a URL to
   * understand more than the "subject" and "body" headers. Clients that
   * resolve mailto URLs into mail messages should be able to correctly
   * create RFC822-compliant mail messages using the "subject" and "body"
   * headers.
   */
  add_to_stailq(&MailToAllow, "body");
  add_to_stailq(&MailToAllow, "subject");
  /* Cc, In-Reply-To, and References help with not breaking threading on
   * mailing lists, see https://github.com/neomutt/neomutt/issues/115 */
  add_to_stailq(&MailToAllow, "cc");
  add_to_stailq(&MailToAllow, "in-reply-to");
  add_to_stailq(&MailToAllow, "references");

  if (STAILQ_EMPTY(&Muttrc))
  {
    char *xdg_cfg_home = getenv("XDG_CONFIG_HOME");

    if (!xdg_cfg_home && HomeDir)
    {
      snprintf(buffer, sizeof(buffer), "%s/.config", HomeDir);
      xdg_cfg_home = buffer;
    }

    char *config = find_cfg(HomeDir, xdg_cfg_home);
    if (config)
    {
      mutt_list_insert_tail(&Muttrc, config);
    }
  }
  else
  {
    struct ListNode *np;
    STAILQ_FOREACH(np, &Muttrc, entries)
    {
      strfcpy(buffer, np->data, sizeof(buffer));
      FREE(&np->data);
      mutt_expand_path(buffer, sizeof(buffer));
      np->data = safe_strdup(buffer);
      if (access(np->data, F_OK))
      {
        snprintf(buffer, sizeof(buffer), "%s: %s", np->data, strerror(errno));
        mutt_endwin(buffer);
        exit(1);
      }
    }
  }

  if (!STAILQ_EMPTY(&Muttrc))
  {
    FREE(&AliasFile);
    AliasFile = safe_strdup(STAILQ_FIRST(&Muttrc)->data);
  }

  /* Process the global rc file if it exists and the user hasn't explicitly
     requested not to via "-n".  */
  if (!skip_sys_rc)
  {
    do
    {
      if (mutt_set_xdg_path(XDG_CONFIG_DIRS, buffer, sizeof(buffer)))
        break;

      snprintf(buffer, sizeof(buffer), "%s/neomuttrc", SYSCONFDIR);
      if (access(buffer, F_OK) == 0)
        break;

      snprintf(buffer, sizeof(buffer), "%s/Muttrc", SYSCONFDIR);
      if (access(buffer, F_OK) == 0)
        break;

      snprintf(buffer, sizeof(buffer), "%s/neomuttrc", PKGDATADIR);
      if (access(buffer, F_OK) == 0)
        break;

      snprintf(buffer, sizeof(buffer), "%s/Muttrc", PKGDATADIR);
    } while (0);
    if (access(buffer, F_OK) == 0)
    {
      if (source_rc(buffer, &err) != 0)
      {
        fputs(err.data, stderr);
        fputc('\n', stderr);
        need_pause = 1;
      }
    }
  }

  /* Read the user's initialization file.  */
  struct ListNode *np;
  STAILQ_FOREACH(np, &Muttrc, entries)
  {
    if (np->data)
    {
      if (!option(OPT_NO_CURSES))
        endwin();
      if (source_rc(np->data, &err) != 0)
      {
        fputs(err.data, stderr);
        fputc('\n', stderr);
        need_pause = 1;
      }
    }
  }

  if (execute_commands(commands) != 0)
    need_pause = 1;

  if (need_pause && !option(OPT_NO_CURSES))
  {
    if (mutt_any_key_to_continue(NULL) == -1)
      mutt_exit(1);
  }

  mutt_mkdir(Tmpdir, S_IRWXU);

  mutt_read_histfile();

#ifdef USE_NOTMUCH
  if (option(OPT_VIRTUAL_SPOOLFILE))
  {
    /* Find the first virtual folder and open it */
    for (struct Buffy *b = Incoming; b; b = b->next)
    {
      if (b->magic == MUTT_NOTMUCH)
      {
        mutt_str_replace(&SpoolFile, b->path);
        mutt_sb_toggle_virtual();
        break;
      }
    }
  }
#endif

  FREE(&err.data);
}

int mutt_get_hook_type(const char *name)
{
  for (const struct Command *c = Commands; c->name; c++)
    if (c->func == mutt_parse_hook && (mutt_strcasecmp(c->name, name) == 0))
      return c->data;
  return 0;
}

static int parse_group_context(struct GroupContext **ctx, struct Buffer *buf,
                               struct Buffer *s, unsigned long data, struct Buffer *err)
{
  while (mutt_strcasecmp(buf->data, "-group") == 0)
  {
    if (!MoreArgs(s))
    {
      strfcpy(err->data, _("-group: no group name"), err->dsize);
      goto bail;
    }

    mutt_extract_token(buf, s, 0);

    mutt_group_context_add(ctx, mutt_pattern_group(buf->data));

    if (!MoreArgs(s))
    {
      strfcpy(err->data, _("out of arguments"), err->dsize);
      goto bail;
    }

    mutt_extract_token(buf, s, 0);
  }

  return 0;

bail:
  mutt_group_context_destroy(ctx);
  return -1;
}

static int parse_tag_transforms(struct Buffer *b, struct Buffer *s,
                                unsigned long data, struct Buffer *err)
{
  if (!b || !s)
    return -1;

  char *tmp = NULL;

  while (MoreArgs(s))
  {
    char *tag = NULL, *transform = NULL;

    mutt_extract_token(b, s, 0);
    if (b->data && *b->data)
      tag = safe_strdup(b->data);
    else
      continue;

    mutt_extract_token(b, s, 0);
    transform = safe_strdup(b->data);

    /* avoid duplicates */
    tmp = hash_find(TagTransforms, tag);
    if (tmp)
    {
      mutt_debug(3, "tag transform '%s' already registered as '%s'\n", tag, tmp);
      FREE(&tag);
      FREE(&transform);
      continue;
    }

    hash_insert(TagTransforms, tag, transform);
  }
  return 0;
}

static int parse_tag_formats(struct Buffer *b, struct Buffer *s,
                             unsigned long data, struct Buffer *err)
{
  if (!b || !s)
    return -1;

  char *tmp = NULL;

  while (MoreArgs(s))
  {
    char *tag = NULL, *format = NULL;

    mutt_extract_token(b, s, 0);
    if (b->data && *b->data)
      tag = safe_strdup(b->data);
    else
      continue;

    mutt_extract_token(b, s, 0);
    format = safe_strdup(b->data);

    /* avoid duplicates */
    tmp = hash_find(TagFormats, format);
    if (tmp)
    {
      mutt_debug(3, "tag format '%s' already registered as '%s'\n", format, tmp);
      FREE(&tag);
      FREE(&format);
      continue;
    }

    hash_insert(TagFormats, format, tag);
  }
  return 0;
}

#ifdef USE_IMAP
/**
 * parse_subscribe_to - 'subscribe-to' command: Add an IMAP subscription.
 * @param b    Buffer space shared by all command handlers
 * @param s    Current line of the config file
 * @param data Data field from init.h:struct Command
 * @param err  Buffer for any error message
 * @retval  0 Success
 * @retval -1 Failed
 *
 * The 'subscribe-to' command allows to subscribe to an IMAP-Mailbox.
 * Patterns are not supported.
 * Use it as follows: subscribe-to =folder
 */
static int parse_subscribe_to(struct Buffer *b, struct Buffer *s,
                              unsigned long data, struct Buffer *err)
{
  if (!b || !s || !err)
    return -1;

  mutt_buffer_reset(err);

  if (MoreArgs(s))
  {
    mutt_extract_token(b, s, 0);

    if (MoreArgs(s))
    {
      mutt_buffer_addstr(err, _("Too many arguments"));
      return -1;
    }

    if (b->data && *b->data)
    {
      /* Expand and subscribe */
      if (imap_subscribe(mutt_expand_path(b->data, b->dsize), 1) != 0)
      {
        mutt_buffer_printf(err, _("Could not subscribe to %s"), b->data);
        return -1;
      }
      else
      {
        mutt_message(_("Subscribed to %s"), b->data);
        return 0;
      }
    }
    else
    {
      mutt_debug(5, "Corrupted buffer");
      return -1;
    }
  }

  mutt_buffer_addstr(err, _("No folder specified"));
  return -1;
}

/**
 * parse_unsubscribe_from - 'unsubscribe-from' command: Cancel an IMAP subscription.
 * @param b    Buffer space shared by all command handlers
 * @param s    Current line of the config file
 * @param data Data field from init.h:struct Command
 * @param err  Buffer for any error message
 * @retval  0 Success
 * @retval -1 Failed
 *
 * The 'unsubscribe-from' command allows to unsubscribe from an IMAP-Mailbox.
 * Patterns are not supported.
 * Use it as follows: unsubscribe-from =folder
 */
static int parse_unsubscribe_from(struct Buffer *b, struct Buffer *s,
                                  unsigned long data, struct Buffer *err)
{
  if (!b || !s || !err)
    return -1;

  if (MoreArgs(s))
  {
    mutt_extract_token(b, s, 0);

    if (MoreArgs(s))
    {
      mutt_buffer_addstr(err, _("Too many arguments"));
      return -1;
    }

    if (b->data && *b->data)
    {
      /* Expand and subscribe */
      if (imap_subscribe(mutt_expand_path(b->data, b->dsize), 0) != 0)
      {
        mutt_buffer_printf(err, _("Could not unsubscribe from %s"), b->data);
        return -1;
      }
      else
      {
        mutt_message(_("Unsubscribed from %s"), b->data);
        return 0;
      }
    }
    else
    {
      mutt_debug(5, "Corrupted buffer");
      return -1;
    }
  }

  mutt_buffer_addstr(err, _("No folder specified"));
  return -1;
}
#endif

const char *myvar_get(const char *var)
{
  struct MyVar *cur = NULL;

  for (cur = MyVars; cur; cur = cur->next)
    if (mutt_strcmp(cur->name, var) == 0)
      return NONULL(cur->value);

  return NULL;
}

int mutt_label_complete(char *buffer, size_t len, int numtabs)
{
  char *pt = buffer;
  int spaces; /* keep track of the number of leading spaces on the line */

  if (!Context || !Context->label_hash)
    return 0;

  SKIPWS(buffer);
  spaces = buffer - pt;

  /* first TAB. Collect all the matches */
  if (numtabs == 1)
  {
    struct HashElem *entry = NULL;
    struct HashWalkState state;

    Num_matched = 0;
    strfcpy(User_typed, buffer, sizeof(User_typed));
    memset(Matches, 0, Matches_listsize);
    memset(Completed, 0, sizeof(Completed));
    memset(&state, 0, sizeof(state));
    while ((entry = hash_walk(Context->label_hash, &state)))
      candidate(Completed, User_typed, entry->key.strkey, sizeof(Completed));
    matches_ensure_morespace(Num_matched);
    qsort(Matches, Num_matched, sizeof(char *), (sort_t *) mutt_strcasecmp);
    Matches[Num_matched++] = User_typed;

    /* All matches are stored. Longest non-ambiguous string is ""
     * i.e. don't change 'buffer'. Fake successful return this time */
    if (User_typed[0] == 0)
      return 1;
  }

  if (Completed[0] == 0 && User_typed[0])
    return 0;

  /* Num_matched will _always_ be at least 1 since the initial
    * user-typed string is always stored */
  if (numtabs == 1 && Num_matched == 2)
    snprintf(Completed, sizeof(Completed), "%s", Matches[0]);
  else if (numtabs > 1 && Num_matched > 2)
    /* cycle thru all the matches */
    snprintf(Completed, sizeof(Completed), "%s", Matches[(numtabs - 2) % Num_matched]);

  /* return the completed label */
  strncpy(buffer, Completed, len - spaces);

  return 1;
}
