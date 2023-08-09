#include <assert.h>
#include <locale.h>
#include "email/lib.h"
#include "core/neomutt.h"
#include "alias/lib.h"
#include "gui/curs_lib.h"
#include "color/color.h"
#include "hdrline.h"
#include "helpers.h"
#include "index_format_callbacks.h"
#include "maillist.h"
#include "mutt_thread.h"
#include "parser.h"
#include "sort.h"

// TODO(g0mb4): see if it can be used for all formats
// NOTE(g0mb4): copy of hdrline.c's add_index_color()
static size_t add_index_color_2gmb(char *buf, size_t buflen,
                                   MuttFormatFlags flags, enum ColorId color)
{
  /* only add color markers if we are operating on main index entries. */
  if (!(flags & MUTT_FORMAT_INDEX))
    return 0;

  /* this item is going to be passed to an external filter */
  if (flags & MUTT_FORMAT_NOFILTER)
    return 0;

  if (color == MT_COLOR_INDEX)
  { /* buf might be uninitialized other cases */
    const size_t len = mutt_str_len(buf);
    buf += len;
    buflen -= len;
  }

  if (buflen <= 2)
    return 0;

  buf[0] = MUTT_SPECIAL_INDEX;
  buf[1] = color;
  buf[2] = '\0';

  return 2;
}

static bool thread_is_new_2gmb(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 1);
}

/**
 * thread_is_old - Does the email thread contain any unread emails?
 * @param e Email
 * @retval true Thread contains unread mail
 */
static bool thread_is_old_2gmb(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 2);
}

static bool user_in_addr_2gmb(struct AddressList *al)
{
  struct Address *a = NULL;
  TAILQ_FOREACH(a, al, entries)
  if (mutt_addr_is_user(a))
    return true;
  return false;
}

static enum ToChars user_is_recipient_2gmb(struct Email *e)
{
  if (!e || !e->env)
    return FLAG_CHAR_TO_NOT_IN_THE_LIST;

  struct Envelope *env = e->env;

  if (!e->recip_valid)
  {
    e->recip_valid = true;

    if (mutt_addr_is_user(TAILQ_FIRST(&env->from)))
    {
      e->recipient = FLAG_CHAR_TO_ORIGINATOR;
    }
    else if (user_in_addr_2gmb(&env->to))
    {
      if (TAILQ_NEXT(TAILQ_FIRST(&env->to), entries) || !TAILQ_EMPTY(&env->cc))
        e->recipient = FLAG_CHAR_TO_TO; /* non-unique recipient */
      else
        e->recipient = FLAG_CHAR_TO_UNIQUE; /* unique recipient */
    }
    else if (user_in_addr_2gmb(&env->cc))
    {
      e->recipient = FLAG_CHAR_TO_CC;
    }
    else if (check_for_mailing_list(&env->to, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (check_for_mailing_list(&env->cc, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (user_in_addr_2gmb(&env->reply_to))
    {
      e->recipient = FLAG_CHAR_TO_REPLY_TO;
    }
    else
    {
      e->recipient = FLAG_CHAR_TO_NOT_IN_THE_LIST;
    }
  }

  return e->recipient;
}

static void format_int(char *buf, int buf_len, int number,
                       MuttFormatFlags flags, enum ColorId pre,
                       enum ColorId post, struct ExpandoFormatPrivate *format)
{
  char fmt[32];

  if (format)
  {
    size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    int n = snprintf(fmt, sizeof(fmt), "%d", number);
    mutt_simple_format(buf + colorlen1, buf_len - colorlen1, format->min, format->max,
                       format->justification, format->leader, fmt, n, false);
    add_index_color_2gmb(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
  else
  {
    size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    int n = snprintf(buf + colorlen1, buf_len - colorlen1, "%d", number);
    add_index_color_2gmb(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
}

static void format_string(char *buf, int buf_len, const char *s,
                          MuttFormatFlags flags, enum ColorId pre,
                          enum ColorId post, struct ExpandoFormatPrivate *format)
{
  char fmt[32];

  if (format)
  {
    assert(strlen(s) < sizeof(fmt) - 1);

    size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    int n = snprintf(fmt, sizeof(fmt), "%s", s);
    mutt_simple_format(buf + colorlen1, buf_len - colorlen1, format->min, format->max,
                       format->justification, format->leader, fmt, n, false);
    add_index_color_2gmb(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
  else
  {
    size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    int n = snprintf(buf + colorlen1, buf_len - colorlen1, "%s", s);
    add_index_color_2gmb(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
}

void index_C(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  // TODO(g0mb4): handle *start_col != 0
  char fmt[128];

  format_int(fmt, sizeof(fmt), e->msgno + 1, flags, MT_COLOR_INDEX_NUMBER,
             MT_COLOR_INDEX, format);

  int printed = snprintf(*buffer, *buffer_len, "%s", fmt);

  *start_col += mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}

void index_Z(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;
  const size_t msg_in_pager = hfi->msg_in_pager;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");
  const bool threads = mutt_using_threads();

  // TODO(g0mb4): handle *start_col != 0
  char fmt[128], tmp[128];

  const char *first = NULL;
  if (threads && thread_is_new_2gmb(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW_THREAD);
  }
  else if (threads && thread_is_old_2gmb(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD_THREAD);
  }
  else if (e->read && (msg_in_pager != e->msgno))
  {
    if (e->replied)
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
    else
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_ZEMPTY);
  }
  else
  {
    if (e->old)
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
    else
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);
  }

  /* Marked for deletion; deleted attachments; crypto */
  const char *second = NULL;
  if (e->deleted)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  else if (e->attach_del)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  else if ((WithCrypto != 0) && (e->security & SEC_GOODSIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_GOOD_SIGN);
  else if ((WithCrypto != 0) && (e->security & SEC_ENCRYPT))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_ENCRYPTED);
  else if ((WithCrypto != 0) && (e->security & SEC_SIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_SIGNED);
  else if (((WithCrypto & APPLICATION_PGP) != 0) && (e->security & PGP_KEY))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_CONTAINS_KEY);
  else
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_NO_CRYPTO);

  /* Tagged, flagged and recipient flag */
  const char *third = NULL;
  if (e->tagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  else if (e->flagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  else
    third = mbtable_get_nth_wchar(c_to_chars, user_is_recipient_2gmb(e));

  snprintf(tmp, sizeof(tmp), "%s%s%s", first, second, third);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS, MT_COLOR_INDEX, format);

  int printed = snprintf(*buffer, *buffer_len, "%s", fmt);

  *start_col += mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}

void index_date(const struct ExpandoNode *self, char **buffer, int *buffer_len,
                int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_DATE);
  assert(self->ndata != NULL);

  struct ExpandoDatePrivate *dp = (struct ExpandoDatePrivate *) self->ndata;
  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  struct tm tm = { 0 };
  char fmt[128], tmp[128];

  switch (dp->date_type)
  {
    case DT_LOCAL_SEND_TIME:
      tm = mutt_date_localtime(e->date_sent);
      break;

    case DT_LOCAL_RECIEVE_TIME:
      tm = mutt_date_localtime(e->received);
      break;

    case DT_SENDER_SEND_TIME:
    {
      /* restore sender's time zone */
      time_t now = e->date_sent;
      if (e->zoccident)
        now -= (e->zhours * 3600 + e->zminutes * 60);
      else
        now += (e->zhours * 3600 + e->zminutes * 60);
      tm = mutt_date_gmtime(now);
    }
    break;

    default:
      assert(0 && "Unknown date type.");
  }

  if (dp->ingnore_locale)
  {
    setlocale(LC_TIME, "C");
  }

  strftime_range(tmp, sizeof(tmp), self->start, self->end, &tm);

  if (dp->ingnore_locale)
  {
    setlocale(LC_TIME, "");
  }

  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_DATE, MT_COLOR_INDEX, NULL);

  int printed = snprintf(*buffer, *buffer_len, "%s", fmt);

  *start_col += mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}

/**
 * make_from_prefix - Create a prefix for an author field
 * @param disp   Type of field
 * @retval ptr Prefix string (do not free it)
 *
 * If $from_chars is set, pick an appropriate character from it.
 * If not, use the default prefix: "To", "Cc", etc
 */
static const char *make_from_prefix_2gmb(enum FieldType disp)
{
  /* need 2 bytes at the end, one for the space, another for NUL */
  static char padded[8];
  static const char *long_prefixes[DISP_MAX] = {
    [DISP_TO] = "To ", [DISP_CC] = "Cc ", [DISP_BCC] = "Bcc ",
    [DISP_FROM] = "",  [DISP_PLAIN] = "",
  };

  const struct MbTable *c_from_chars = cs_subset_mbtable(NeoMutt->sub, "from_chars");

  if (!c_from_chars || !c_from_chars->chars || (c_from_chars->len == 0))
    return long_prefixes[disp];

  const char *pchar = mbtable_get_nth_wchar(c_from_chars, disp);
  if (mutt_str_len(pchar) == 0)
    return "";

  snprintf(padded, sizeof(padded), "%s ", pchar);
  return padded;
}

/**
 * make_from - Generate a From: field (with optional prefix)
 * @param env      Envelope of the email
 * @param buf      Buffer to store the result
 * @param buflen   Size of the buffer
 * @param do_lists Should we check for mailing lists?
 * @param flags    Format flags, see #MuttFormatFlags
 *
 * Generate the %F or %L field in $index_format.
 * This is the author, or recipient of the email.
 *
 * The field can optionally be prefixed by a character from $from_chars.
 * If $from_chars is not set, the prefix will be, "To", "Cc", etc
 */
static void make_from_2gmb(struct Envelope *env, char *buf, size_t buflen,
                           bool do_lists, MuttFormatFlags flags)
{
  if (!env || !buf)
    return;

  bool me;
  enum FieldType disp;
  struct AddressList *name = NULL;

  me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list(&env->to, make_from_prefix_2gmb(DISP_TO), buf, buflen))
      return;
    if (check_for_mailing_list(&env->cc, make_from_prefix_2gmb(DISP_CC), buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
  {
    disp = (flags & MUTT_FORMAT_PLAIN) ? DISP_PLAIN : DISP_TO;
    name = &env->to;
  }
  else if (me && !TAILQ_EMPTY(&env->cc))
  {
    disp = DISP_CC;
    name = &env->cc;
  }
  else if (me && !TAILQ_EMPTY(&env->bcc))
  {
    disp = DISP_BCC;
    name = &env->bcc;
  }
  else if (!TAILQ_EMPTY(&env->from))
  {
    disp = DISP_FROM;
    name = &env->from;
  }
  else
  {
    *buf = '\0';
    return;
  }

  snprintf(buf, buflen, "%s%s", make_from_prefix_2gmb(disp),
           mutt_get_name(TAILQ_FIRST(name)));
}

void index_L(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;
  const bool optional = (flags & MUTT_FORMAT_OPTIONAL);

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  char fmt[128], tmp[128];

  if (optional)
  {
    return;
  }

  make_from_2gmb(e->env, tmp, sizeof(tmp), true, flags);

  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_AUTHOR, MT_COLOR_INDEX, format);

  int printed = snprintf(*buffer, *buffer_len, "%s", fmt);

  *start_col += mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}