/**
 * @file
 * Many unsorted constants and some structs
 *
 * @authors
 * Copyright (C) 1996-2002,2010,2013 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2004 g10 Code GmbH
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

#ifndef _MUTT_H
#define _MUTT_H

#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct ReplaceList;
struct RegexList;
struct State;
struct ListHead;
struct TagHead;
struct Mapping;

/* On OS X 10.5.x, wide char functions are inlined by default breaking
 * --without-wc-funcs compilation
 */
#ifdef __APPLE_CC__
#define _DONT_USE_CTYPE_INLINE_
#endif

/* PATH_MAX is undefined on the hurd */
#if !defined(PATH_MAX) && defined(_POSIX_PATH_MAX)
#define PATH_MAX _POSIX_PATH_MAX
#endif

#ifdef HAVE_FGETS_UNLOCKED
#define fgets fgets_unlocked
#endif

#ifdef HAVE_FGETC_UNLOCKED
#define fgetc fgetc_unlocked
#endif

/* flags for mutt_enter_string() */
#define MUTT_ALIAS    (1 << 0)  /**< do alias "completion" by calling up the alias-menu */
#define MUTT_FILE     (1 << 1)  /**< do file completion */
#define MUTT_EFILE    (1 << 2)  /**< do file completion, plus incoming folders */
#define MUTT_CMD      (1 << 3)  /**< do completion on previous word */
#define MUTT_PASS     (1 << 4)  /**< password mode (no echo) */
#define MUTT_CLEAR    (1 << 5)  /**< clear input if printable character is pressed */
#define MUTT_COMMAND  (1 << 6)  /**< do command completion */
#define MUTT_PATTERN  (1 << 7)  /**< pattern mode - only used for history classes */
#define MUTT_LABEL    (1 << 8)  /**< do label completion */
#ifdef USE_NOTMUCH
#define MUTT_NM_QUERY (1 << 9)  /**< Notmuch query mode. */
#define MUTT_NM_TAG   (1 << 10) /**< Notmuch tag +/- mode. */
#endif
#define MUTT_MATCHER (1 << 11)

/* flags for mutt_get_token() */
#define MUTT_TOKEN_EQUAL      1       /* treat '=' as a special */
#define MUTT_TOKEN_CONDENSE   (1<<1)  /* ^(char) to control chars (macros) */
#define MUTT_TOKEN_SPACE      (1<<2)  /* don't treat whitespace as a term */
#define MUTT_TOKEN_QUOTE      (1<<3)  /* don't interpret quotes */
#define MUTT_TOKEN_PATTERN    (1<<4)  /* !)|~ are terms (for patterns) */
#define MUTT_TOKEN_COMMENT    (1<<5)  /* don't reap comments */
#define MUTT_TOKEN_SEMICOLON  (1<<6)  /* don't treat ; as special */

/* types for mutt_add_hook() */
#define MUTT_FOLDERHOOK   (1 << 0)
#define MUTT_MBOXHOOK     (1 << 1)
#define MUTT_SENDHOOK     (1 << 2)
#define MUTT_FCCHOOK      (1 << 3)
#define MUTT_SAVEHOOK     (1 << 4)
#define MUTT_CHARSETHOOK  (1 << 5)
#define MUTT_ICONVHOOK    (1 << 6)
#define MUTT_MESSAGEHOOK  (1 << 7)
#define MUTT_CRYPTHOOK    (1 << 8)
#define MUTT_ACCOUNTHOOK  (1 << 9)
#define MUTT_REPLYHOOK    (1 << 10)
#define MUTT_SEND2HOOK    (1 << 11)
#ifdef USE_COMPRESSED
#define MUTT_OPENHOOK     (1 << 12)
#define MUTT_APPENDHOOK   (1 << 13)
#define MUTT_CLOSEHOOK    (1 << 14)
#endif
#define MUTT_TIMEOUTHOOK  (1 << 15)
#define MUTT_STARTUPHOOK  (1 << 16)
#define MUTT_SHUTDOWNHOOK (1 << 17)
#define MUTT_GLOBALHOOK   (1 << 18)

/* tree characters for linearize_tree and print_enriched_string */
#define MUTT_TREE_LLCORNER 1
#define MUTT_TREE_ULCORNER 2
#define MUTT_TREE_LTEE     3
#define MUTT_TREE_HLINE    4
#define MUTT_TREE_VLINE    5
#define MUTT_TREE_SPACE    6
#define MUTT_TREE_RARROW   7
#define MUTT_TREE_STAR     8
#define MUTT_TREE_HIDDEN   9
#define MUTT_TREE_EQUALS   10
#define MUTT_TREE_TTEE     11
#define MUTT_TREE_BTEE     12
#define MUTT_TREE_MISSING  13
#define MUTT_TREE_MAX      14

#define MUTT_SPECIAL_INDEX MUTT_TREE_MAX

#define MUTT_THREAD_COLLAPSE    (1 << 0)
#define MUTT_THREAD_UNCOLLAPSE  (1 << 1)
#define MUTT_THREAD_GET_HIDDEN  (1 << 2)
#define MUTT_THREAD_UNREAD      (1 << 3)
#define MUTT_THREAD_NEXT_UNREAD (1 << 4)
#define MUTT_THREAD_FLAGGED     (1 << 5)

/**
 * enum MuttMisc - Unsorted flags
 */
enum MuttMisc
{
  /* modes for mutt_view_attachment() */
  MUTT_REGULAR = 1,
  MUTT_MAILCAP,
  MUTT_AS_TEXT,

  /* action codes used by mutt_set_flag() and mutt_pattern_function() */
  MUTT_ALL,
  MUTT_NONE,
  MUTT_NEW,
  MUTT_OLD,
  MUTT_REPLIED,
  MUTT_READ,
  MUTT_UNREAD,
  MUTT_DELETE,
  MUTT_UNDELETE,
  MUTT_PURGE,
  MUTT_DELETED,
  MUTT_FLAG,
  MUTT_TAG,
  MUTT_UNTAG,
  MUTT_LIMIT,
  MUTT_EXPIRED,
  MUTT_SUPERSEDED,
  MUTT_TRASH,

  /* actions for mutt_pattern_comp/mutt_pattern_exec */
  MUTT_AND,
  MUTT_OR,
  MUTT_THREAD,
  MUTT_PARENT,
  MUTT_CHILDREN,
  MUTT_TO,
  MUTT_CC,
  MUTT_COLLAPSED,
  MUTT_SUBJECT,
  MUTT_FROM,
  MUTT_DATE,
  MUTT_DATE_RECEIVED,
  MUTT_DUPLICATED,
  MUTT_UNREFERENCED,
  MUTT_BROKEN,
  MUTT_ID,
  MUTT_BODY,
  MUTT_HEADER,
  MUTT_HORMEL,
  MUTT_WHOLE_MSG,
  MUTT_SENDER,
  MUTT_MESSAGE,
  MUTT_SCORE,
  MUTT_SIZE,
  MUTT_REFERENCE,
  MUTT_RECIPIENT,
  MUTT_LIST,
  MUTT_SUBSCRIBED_LIST,
  MUTT_PERSONAL_RECIP,
  MUTT_PERSONAL_FROM,
  MUTT_ADDRESS,
  MUTT_CRYPT_SIGN,
  MUTT_CRYPT_VERIFIED,
  MUTT_CRYPT_ENCRYPT,
  MUTT_PGP_KEY,
  MUTT_XLABEL,
  MUTT_SERVERSEARCH,
  MUTT_DRIVER_TAGS,
  MUTT_MIMEATTACH,
#ifdef USE_NNTP
  MUTT_NEWSGROUPS,
#endif

  /* Options for Mailcap lookup */
  MUTT_EDIT,
  MUTT_COMPOSE,
  MUTT_PRINT,
  MUTT_AUTOVIEW,

  /* options for socket code */
  MUTT_NEW_SOCKET,
#ifdef USE_SSL_OPENSSL
  MUTT_NEW_SSL_SOCKET,
#endif

  /* Options for mutt_save_attachment */
  MUTT_SAVE_APPEND,
  MUTT_SAVE_OVERWRITE
};

/**
 * enum QuadOptionResponse - Possible values of a QuadOption
 */
enum QuadOptionResponse
{
  MUTT_ABORT = -1,
  MUTT_NO,
  MUTT_YES,
  MUTT_ASKNO,
  MUTT_ASKYES
};

/**
 * enum QuadOptionVars - Index of all QuadOptions
 */
enum QuadOptionVars
{
  OPT_ABORT_UNMODIFIED,
  OPT_BOUNCE,
  OPT_COPY,
  OPT_DELETE,
  OPT_FORWARD_EDIT,
  OPT_FCC_ATTACH,
  OPT_INCLUDE,
  OPT_HONOR_FOLLOWUP_TO,
  OPT_MIME_FORWARD,
  OPT_MIME_FORWARD_REST,
  OPT_MOVE,
  OPT_PGP_MIME_AUTO, /* ask to revert to PGP/MIME when inline fails */
  OPT_SMIME_ENCRYPT_SELF,
  OPT_PGP_ENCRYPT_SELF,
#ifdef USE_POP
  OPT_POP_DELETE,
  OPT_POP_RECONNECT,
#endif
  OPT_POSTPONE,
  OPT_PRINT,
  OPT_QUIT,
  OPT_REPLY_TO,
  OPT_RECALL,
#ifdef USE_SSL
  OPT_SSL_STARTTLS,
#endif
  OPT_ABORT_NOSUBJECT,
  OPT_CRYPT_VERIFY_SIG, /* verify PGP signatures */
#ifdef USE_NNTP
  OPT_POST_MODERATED,
  OPT_CATCHUP_NEWSGROUP,
  OPT_FOLLOWUP_TO_POSTER,
#endif
  OPT_ABORT_NOATTACH, /* forgotten attachment detector */
  /* THIS MUST BE THE LAST VALUE. */
  OPT_QUAD_MAX,
};

/* flags to ci_send_message() */
#define SENDREPLY        (1 << 0)
#define SENDGROUPREPLY   (1 << 1)
#define SENDLISTREPLY    (1 << 2)
#define SENDFORWARD      (1 << 3)
#define SENDPOSTPONED    (1 << 4)
#define SENDBATCH        (1 << 5)
#define SENDMAILX        (1 << 6)
#define SENDKEY          (1 << 7)
#define SENDRESEND       (1 << 8)
#define SENDPOSTPONEDFCC (1 << 9)  /**< used by mutt_get_postponed() to signal that the x-mutt-fcc header field was present */
#define SENDNOFREEHEADER (1 << 10) /**< Used by the -E flag */
#define SENDDRAFTFILE    (1 << 11) /**< Used by the -H flag */
#define SENDNEWS         (1 << 12)

/* flags for mutt_compose_menu() */
#define MUTT_COMPOSE_NOFREEHEADER (1 << 0)

/* flags to _mutt_select_file() */
#define MUTT_SEL_BUFFY   (1 << 0)
#define MUTT_SEL_MULTI   (1 << 1)
#define MUTT_SEL_FOLDER  (1 << 2)
#define MUTT_SEL_VFOLDER (1 << 3)

/* flags for parse_spam_list */
#define MUTT_SPAM   1
#define MUTT_NOSPAM 2

void mutt_free_regex_list(struct RegexList **list);
void mutt_free_replace_list(struct ReplaceList **list);
bool mutt_matches_ignore(const char *s);

/* add an element to a list */
int mutt_remove_from_regex_list(struct RegexList **l, const char *str);

void mutt_init(int skip_sys_rc, struct ListHead *commands);

/* flag to mutt_pattern_comp() */
#define MUTT_FULL_MSG (1 << 0) /* enable body and header matching */

/**
 * struct AttachMatch - An attachment matching a regex
 *
 * for attachment counter
 */
struct AttachMatch
{
  char *major;
  int major_int;
  char *minor;
  regex_t minor_regex;
};

#define MUTT_PARTS_TOPLEVEL (1 << 0) /* is the top-level part */

#define EXECSHELL "/bin/sh"

/* For mutt_simple_format() justifications */
/* Making left 0 and center -1 is of course completely nonsensical, but
 * it retains compatibility for any patches that call mutt_simple_format.
 * Once patches are updated to use FMT_*, these can be made sane. */
#define FMT_LEFT   0
#define FMT_RIGHT  1
#define FMT_CENTER -1

/* Exit values used in send_msg() */
#define S_ERR 127
#define S_BKG 126

int safe_asprintf(char **, const char *, ...);

int mutt_inbox_cmp(const char *a, const char *b);

const char *mutt_strsysexit(int e);

char *mutt_compile_help(char *buf, size_t buflen, int menu, const struct Mapping *items);

#ifdef DEBUG
extern char debugfilename[_POSIX_PATH_MAX];
extern FILE *debugfile;
extern int debuglevel;
extern char *debugfile_cmdline;
extern int debuglevel_cmdline;
#endif

#endif /* _MUTT_H */
