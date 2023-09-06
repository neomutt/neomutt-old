/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023 Tóth János <gomba007@gmail.com>
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
 * @page expando_index_format XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include "mutt/lib.h"
#include "expando/lib.h"
#include "ncrypt/lib.h"
#include "format_flags.h"
#include "locale.h"
#include "ncrypt/crypt_gpgme.h"
#include "ncrypt/pgp.h"
#include "ncrypt/pgpkey.h"
#include "ncrypt/pgplib.h"

/**
 * pgp_key_abilities - Turn PGP key abilities into a string
 * @param flags Flags, see #KeyFlags
 * @retval ptr Abilities string
 *
 * @note This returns a pointer to a static buffer
 */
static char *pgp_key_abilities(KeyFlags flags)
{
  static char buf[3];

  if (!(flags & KEYFLAG_CANENCRYPT))
    buf[0] = '-';
  else if (flags & KEYFLAG_PREFER_SIGNING)
    buf[0] = '.';
  else
    buf[0] = 'e';

  if (!(flags & KEYFLAG_CANSIGN))
    buf[1] = '-';
  else if (flags & KEYFLAG_PREFER_ENCRYPTION)
    buf[1] = '.';
  else
    buf[1] = 's';

  buf[2] = '\0';

  return buf;
}

/**
 * pgp_flags - Turn PGP key flags into a string
 * @param flags Flags, see #KeyFlags
 * @retval char Flag character
 */
static char pgp_flags(KeyFlags flags)
{
  if (flags & KEYFLAG_REVOKED)
    return 'R';
  if (flags & KEYFLAG_EXPIRED)
    return 'X';
  if (flags & KEYFLAG_DISABLED)
    return 'd';
  if (flags & KEYFLAG_CRITICAL)
    return 'c';

  return ' ';
}

int pgp_entry_pgp_date(const struct ExpandoNode *self, char *buf, int buf_len,
                       int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_DATE);
  assert(self->ndata != NULL);

  struct ExpandoDatePrivate *dp = (struct ExpandoDatePrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  const struct PgpKeyInfo *key = uid->parent;

  char tmp[128], fmt[128];
  char datestr[128];

  const int size = self->end - self->start;
  assert(size < sizeof(datestr));
  mutt_strn_copy(datestr, self->start, size, sizeof(datestr));

  if (dp->use_c_locale)
  {
    mutt_date_localtime_format_locale(tmp, sizeof(tmp), datestr, key->gen_time,
                                      NeoMutt->time_c_locale);
  }
  else
  {
    mutt_date_localtime_format(tmp, sizeof(tmp), datestr, key->gen_time);
  }

  format_string_simple(fmt, sizeof(fmt), tmp, NULL);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_n(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;

  char fmt[128];

  const int num = entry->num;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  c
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_p(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_gpgme.c */
  return 0;
}

int pgp_entry_pgp_t(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;

  char tmp[128], fmt[128];

  /// Characters used to show the trust level for PGP keys
  // NOTE(g0mb4): Use $to_chars?
  static const char TrustFlags[] = "?- +";

  snprintf(tmp, sizeof(tmp), "%c", TrustFlags[uid->trust & 0x03]);
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_u(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;

  char fmt[128];

  const char *s = NONULL(uid->addr);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_a(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  const struct PgpKeyInfo *key = uid->parent;

  char fmt[128];

  const char *s = key->algorithm;
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_A(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;
  struct PgpKeyInfo *pkey = pgp_principal_key(key);

  char fmt[128];

  const char *s = pkey->algorithm;
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_c(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  const struct PgpKeyInfo *key = uid->parent;

  const KeyFlags kflags = key->flags | uid->flags;

  char fmt[128];

  const char *s = pgp_key_abilities(kflags);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_C(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;
  struct PgpKeyInfo *pkey = pgp_principal_key(key);

  const KeyFlags kflags = (pkey->flags & KEYFLAG_RESTRICTIONS) | uid->flags;

  char fmt[128];

  const char *s = pgp_key_abilities(kflags);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_f(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  const struct PgpKeyInfo *key = uid->parent;

  const KeyFlags kflags = key->flags | uid->flags;

  char tmp[128], fmt[128];

  snprintf(tmp, sizeof(tmp), "%c", pgp_flags(kflags));
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_F(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;
  struct PgpKeyInfo *pkey = pgp_principal_key(key);

  const KeyFlags kflags = (pkey->flags & KEYFLAG_RESTRICTIONS) | uid->flags;

  char tmp[128], fmt[128];

  snprintf(tmp, sizeof(tmp), "%c", pgp_flags(kflags));
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_i(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_gpgme.c*/
  return 0;
}

int pgp_entry_pgp_I(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_gpgme.c*/
  return 0;
}

int pgp_entry_pgp_k(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;

  char fmt[128];

  const char *s = pgp_this_keyid(key);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_K(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;
  struct PgpKeyInfo *pkey = pgp_principal_key(key);

  char fmt[128];

  const char *s = pgp_this_keyid(pkey);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_l(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  const struct PgpKeyInfo *key = uid->parent;

  char fmt[128];

  const int num = key->keylen;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

int pgp_entry_pgp_L(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PGP
  const struct PgpEntry *entry = (const struct PgpEntry *) data;
  const struct PgpUid *uid = entry->uid;
  struct PgpKeyInfo *key = uid->parent;
  struct PgpKeyInfo *pkey = pgp_principal_key(key);

  char fmt[128];

  const int num = pkey->keylen;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PGP */
  return 0;
#endif /* HAVE_PGP */
}

/* -------------------------------------------------  */

/**
 * crypt_key_abilities - Parse key flags into a string
 * @param flags Flags, see #KeyFlags
 * @retval ptr Flag string
 *
 * @note The string is statically allocated.
 */
static char *crypt_key_abilities(KeyFlags flags)
{
  static char buf[3];

  if (!(flags & KEYFLAG_CANENCRYPT))
    buf[0] = '-';
  else if (flags & KEYFLAG_PREFER_SIGNING)
    buf[0] = '.';
  else
    buf[0] = 'e';

  if (!(flags & KEYFLAG_CANSIGN))
    buf[1] = '-';
  else if (flags & KEYFLAG_PREFER_ENCRYPTION)
    buf[1] = '.';
  else
    buf[1] = 's';

  buf[2] = '\0';

  return buf;
}

/**
 * crypt_flags - Parse the key flags into a single character
 * @param flags Flags, see #KeyFlags
 * @retval char Flag character
 *
 * The returned character describes the most important flag.
 */
static char *crypt_flags(KeyFlags flags)
{
  if (flags & KEYFLAG_REVOKED)
    return "R";
  if (flags & KEYFLAG_EXPIRED)
    return "X";
  if (flags & KEYFLAG_DISABLED)
    return "d";
  if (flags & KEYFLAG_CRITICAL)
    return "c";

  return " ";
}

int pgp_entry_gpgme_date(const struct ExpandoNode *self, char *buf, int buf_len,
                         int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_DATE);
  assert(self->ndata != NULL);

  struct ExpandoDatePrivate *dp = (struct ExpandoDatePrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char tmp[128], fmt[128];
  char datestr[128];

  const int size = self->end - self->start;
  assert(size < sizeof(datestr));
  mutt_strn_copy(datestr, self->start, size, sizeof(datestr));

  struct tm tm = { 0 };
  if (key->kobj->subkeys && (key->kobj->subkeys->timestamp > 0))
  {
    tm = mutt_date_localtime(key->kobj->subkeys->timestamp);
  }
  else
  {
    tm = mutt_date_localtime(0); // Default to 1970-01-01
  }

  if (dp->use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), datestr, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), datestr, &tm);
  }

  format_string_simple(fmt, sizeof(fmt), tmp, NULL);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_n(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;

  char fmt[128];

  const int num = entry->num;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_p(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = gpgme_get_protocol_name(key->kobj->protocol);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_t(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = "";
  if ((key->flags & KEYFLAG_ISX509))
  {
    s = "x";
  }
  else
  {
    switch (key->validity)
    {
      case GPGME_VALIDITY_FULL:
        s = "f";
        break;
      case GPGME_VALIDITY_MARGINAL:
        s = "m";
        break;
      case GPGME_VALIDITY_NEVER:
        s = "n";
        break;
      case GPGME_VALIDITY_ULTIMATE:
        s = "u";
        break;
      case GPGME_VALIDITY_UNDEFINED:
        s = "q";
        break;
      case GPGME_VALIDITY_UNKNOWN:
      default:
        s = "?";
        break;
    }
  }

  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_u(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = key->uid;
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_a(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = NULL;
  if (key->kobj->subkeys)
    s = gpgme_pubkey_algo_name(key->kobj->subkeys->pubkey_algo);
  else
    s = "?";

  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_A(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}

int pgp_entry_gpgme_c(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = crypt_key_abilities(key->flags);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_C(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}

int pgp_entry_gpgme_f(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  const char *s = crypt_flags(key->flags);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_F(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}

int pgp_entry_gpgme_i(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  /* fixme: we need a way to distinguish between main and subkeys.
   * Store the idx in entry? */
  const char *s = crypt_fpr_or_lkeyid(key);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_I(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}

int pgp_entry_gpgme_k(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  struct CryptKeyInfo *key = entry->key;

  char fmt[128];

  /* fixme: we need a way to distinguish between main and subkeys.
   * Store the idx in entry? */
  const char *s = crypt_keyid(key);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_K(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}

int pgp_entry_gpgme_l(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

#ifdef HAVE_PKG_GPGME
  const struct CryptEntry *entry = (const struct CryptEntry *) data;
  const struct CryptKeyInfo *key = entry->key;

  char tmp[128], fmt[128];

  const unsigned long val = key->kobj->subkeys ? key->kobj->subkeys->length : 0;
  snprintf(tmp, sizeof(tmp), "%lu", val);
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_PKG_GPGME */
  return 0;
#endif /* HAVE_PKG_GPGME */
}

int pgp_entry_gpgme_L(const struct ExpandoNode *self, char *buf, int buf_len,
                      int cols_len, intptr_t data, MuttFormatFlags flags)
{
  /* only used in ncrypt/dlg_pgp.c */
  return 0;
}
