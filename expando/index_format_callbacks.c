#include <assert.h>
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

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  // TODO(g0mb4): handle *start_col != 0
  char fmt[128];
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

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

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;
  const size_t msg_in_pager = hfi->msg_in_pager;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");
  const bool threads = mutt_using_threads();

  // TODO(g0mb4): handle *start_col != 0
  char fmt[128], tmp[128];
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

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