/**
 * @file
 * Duplicate the structure of an entire email
 *
 * @authors
 * Copyright (C) 1996-2000,2002,2014 Michael R. Elkins <me@mutt.org>
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
 * @page copy Duplicate the structure of an entire email
 *
 * Duplicate the structure of an entire email
 */

#include "config.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "mutt/mutt.h"
#include "mutt.h"
#include "copy.h"
#include "body.h"
#include "context.h"
#include "dump.h"
#include "envelope.h"
#include "globals.h"
#include "header.h"
#include "mailbox.h"
#include "mutt_curses.h"
#include "mutt_window.h"
#include "mx.h"
#include "ncrypt/ncrypt.h"
#include "options.h"
#include "protos.h"
#include "rfc2047.h"
#include "state.h"
#include "tags.h"
#ifdef USE_NOTMUCH
#include "mutt_notmuch.h"
#endif

static int address_header_decode(char **h);
static int copy_delete_attach(struct Body *b, FILE *fpin, FILE *fpout, char *date);

/**
 * mutt_copy_hdr - Copy header from one file to another
 * @param in        FILE pointer to read from
 * @param out       FILE pointer to write to
 * @param off_start Offset to start from
 * @param off_end   Offset to finish at
 * @param flags     Flags (see below)
 * @param prefix    Prefix for quoting headers
 * @retval  0 Success
 * @retval -1 Failure
 *
 * Ok, the only reason for not merging this with mutt_copy_header() below is to
 * avoid creating a Header structure in message_handler().  Also, this one will
 * wrap headers much more aggressively than the other one.
 */
int mutt_copy_hdr(FILE *in, FILE *out, LOFF_T off_start, LOFF_T off_end,
                  int flags, const char *prefix, FILE *out_autocrypt)
{
  bool from = false;
  bool this_is_from = false;
  bool ignore = false;
  bool is_autocrypt = false;
  char buf[LONG_STRING]; /* should be long enough to get most fields in one pass */
  char *nl = NULL;
  char **headers = NULL;
  int hdr_count;
  int x;
  char *this_one = NULL;
  size_t this_one_len = 0;
  int error;

  if (off_start < 0)
    return -1;

  if (ftello(in) != off_start)
    if (fseeko(in, off_start, SEEK_SET) < 0)
      return -1;

  buf[0] = '\n';
  buf[1] = '\0';

  if ((flags & (CH_REORDER | CH_WEED | CH_MIME | CH_DECODE | CH_PREFIX | CH_WEED_DELIVERED)) == 0)
  {
    /* Without these flags to complicate things
     * we can do a more efficient line to line copying
     */
    while (ftello(in) < off_end)
    {
      nl = strchr(buf, '\n');

      if ((fgets(buf, sizeof(buf), in)) == NULL)
        break;

      /* Is it the beginning of a header? */
      if (nl && buf[0] != ' ' && buf[0] != '\t')
      {
        ignore = true;
        if (!from && (mutt_str_strncmp("From ", buf, 5) == 0))
        {
          if ((flags & CH_FROM) == 0)
            continue;
          from = true;
        }
        else if (flags & (CH_NOQFROM) && (mutt_str_strncasecmp(">From ", buf, 6) == 0))
          continue;

        else if (buf[0] == '\n' || (buf[0] == '\r' && buf[1] == '\n'))
          break; /* end of header */

        if ((flags & (CH_UPDATE | CH_XMIT | CH_NOSTATUS)) &&
            ((mutt_str_strncasecmp("Status:", buf, 7) == 0) ||
             (mutt_str_strncasecmp("X-Status:", buf, 9) == 0)))
          continue;
        if ((flags & (CH_UPDATE_LEN | CH_XMIT | CH_NOLEN)) &&
            ((mutt_str_strncasecmp("Content-Length:", buf, 15) == 0) ||
             (mutt_str_strncasecmp("Lines:", buf, 6) == 0)))
          continue;
        if ((flags & CH_UPDATE_REFS) && (mutt_str_strncasecmp("References:", buf, 11) == 0))
          continue;
        if ((flags & CH_UPDATE_IRT) && (mutt_str_strncasecmp("In-Reply-To:", buf, 12) == 0))
          continue;
        if (flags & CH_UPDATE_LABEL && (mutt_str_strncasecmp("X-Label:", buf, 8) == 0))
          continue;

        ignore = false;
      }

      if (!ignore && fputs(buf, out) == EOF)
        return -1;
    }
    return 0;
  }

  hdr_count = 1;
  x = 0;
  error = false;

  /* We are going to read and collect the headers in an array
   * so we are able to do re-ordering.
   * First count the number of entries in the array
   */
  if (flags & CH_REORDER)
  {
    struct ListNode *np;
    STAILQ_FOREACH(np, &HeaderOrderList, entries)
    {
      mutt_debug(3, "Reorder list: %s\n", np->data);
      hdr_count++;
    }
  }

  mutt_debug(1, "WEED is %s\n", (flags & CH_WEED) ? "Set" : "Not");

  headers = mutt_mem_calloc(hdr_count, sizeof(char *));

  /* Read all the headers into the array */
  while (ftello(in) < off_end)
  {
    nl = strchr(buf, '\n');

    /* Read a line */
    if ((fgets(buf, sizeof(buf), in)) == NULL)
      break;

    /* Is it the beginning of a header? */
    if (nl && buf[0] != ' ' && buf[0] != '\t')
    {
      /* Do we have anything pending? */
      if (this_one)
      {
        if (flags & CH_DECODE)
        {
          if (address_header_decode(&this_one) == 0)
            mutt_rfc2047_decode(&this_one);
          this_one_len = mutt_str_strlen(this_one);

          /* Convert CRLF line endings to LF */
          if ((this_one_len > 2) && (this_one[this_one_len - 2] == '\r') &&
              (this_one[this_one_len - 1] == '\n'))
          {
            this_one[this_one_len - 2] = '\n';
            this_one[this_one_len - 1] = '\0';
          }
        }

        if (!headers[x])
          headers[x] = this_one;
        else
        {
          int hlen = mutt_str_strlen(headers[x]);

          mutt_mem_realloc(&headers[x], hlen + this_one_len + sizeof(char));
          strcat(headers[x] + hlen, this_one);
          FREE(&this_one);
        }

        this_one = NULL;
      }

      if (is_autocrypt)
        fputs("\n", out_autocrypt);

      ignore = true;
      is_autocrypt = false;
      this_is_from = false;
      if (!from && (mutt_str_strncmp("From ", buf, 5) == 0))
      {
        if ((flags & CH_FROM) == 0)
          continue;
        this_is_from = from = true;
      }
      else if (buf[0] == '\n' || (buf[0] == '\r' && buf[1] == '\n'))
        break; /* end of header */

      if (out_autocrypt &&
          ((mutt_str_strncasecmp("Autocrypt:", buf, 10) == 0) ||
            (mutt_str_strncasecmp("From:", buf, 5) == 0) ||
            (mutt_str_strncasecmp("Date:", buf, 5) == 0) ||
            (mutt_str_strncasecmp("To:", buf, 3) == 0) ||
            (mutt_str_strncasecmp("Cc:", buf, 3) == 0))) {
        is_autocrypt = true;
      }

      /* note: CH_FROM takes precedence over header weeding. */
      if (!((flags & CH_FROM) && (flags & CH_FORCE_FROM) && this_is_from) &&
          (flags & CH_WEED) && mutt_matches_ignore(buf))
      {
        if (is_autocrypt)
        {
          size_t blen = mutt_str_strlen(buf);
          while (buf[blen -1] == '\n' || buf[blen -1] == '\r')
            blen--;
          fwrite(buf, sizeof(char), blen, out_autocrypt);
        }

        continue;
      }
      if ((flags & CH_WEED_DELIVERED) &&
          (mutt_str_strncasecmp("Delivered-To:", buf, 13) == 0))
      {
        continue;
      }
      if ((flags & (CH_UPDATE | CH_XMIT | CH_NOSTATUS)) &&
          ((mutt_str_strncasecmp("Status:", buf, 7) == 0) ||
           (mutt_str_strncasecmp("X-Status:", buf, 9) == 0)))
      {
        continue;
      }
      if ((flags & (CH_UPDATE_LEN | CH_XMIT | CH_NOLEN)) &&
          ((mutt_str_strncasecmp("Content-Length:", buf, 15) == 0) ||
           (mutt_str_strncasecmp("Lines:", buf, 6) == 0)))
      {
        continue;
      }
      if ((flags & CH_MIME) &&
          (((mutt_str_strncasecmp("content-", buf, 8) == 0) &&
            ((mutt_str_strncasecmp("transfer-encoding:", buf + 8, 18) == 0) ||
             (mutt_str_strncasecmp("type:", buf + 8, 5) == 0))) ||
           (mutt_str_strncasecmp("mime-version:", buf, 13) == 0)))
      {
        continue;
      }
      if ((flags & CH_UPDATE_REFS) && (mutt_str_strncasecmp("References:", buf, 11) == 0))
        continue;
      if ((flags & CH_UPDATE_IRT) && (mutt_str_strncasecmp("In-Reply-To:", buf, 12) == 0))
        continue;

      /* Find x -- the array entry where this header is to be saved */
      if (flags & CH_REORDER)
      {
        struct ListNode *np;
        x = 0;
        STAILQ_FOREACH(np, &HeaderOrderList, entries)
        {
          ++x;
          if (mutt_str_strncasecmp(buf, np->data, mutt_str_strlen(np->data)) == 0)
          {
            mutt_debug(2, "Reorder: %s matches %s", np->data, buf);
            break;
          }
        }
      }

      ignore = false;
    } /* If beginning of header */

    if (is_autocrypt)
    {
      size_t blen = mutt_str_strlen(buf);
      while (buf[blen -1] == '\n' || buf[blen -1] == '\r')
        blen--;
      fwrite(buf, sizeof(char), blen, out_autocrypt);
    }

    if (!ignore)
    {
      mutt_debug(2, "Reorder: x = %d; hdr_count = %d\n", x, hdr_count);
      if (!this_one)
      {
        this_one = mutt_str_strdup(buf);
        this_one_len = mutt_str_strlen(this_one);
      }
      else
      {
        size_t blen = mutt_str_strlen(buf);

        mutt_mem_realloc(&this_one, this_one_len + blen + sizeof(char));
        strcat(this_one + this_one_len, buf);
        this_one_len += blen;
      }
    }
  } /* while (ftello (in) < off_end) */

  /* Do we have anything pending?  -- XXX, same code as in above in the loop. */
  if (this_one)
  {
    if (flags & CH_DECODE)
    {
      if (address_header_decode(&this_one) == 0)
        mutt_rfc2047_decode(&this_one);
      this_one_len = mutt_str_strlen(this_one);
    }

    if (!headers[x])
      headers[x] = this_one;
    else
    {
      int hlen = mutt_str_strlen(headers[x]);

      mutt_mem_realloc(&headers[x], hlen + this_one_len + sizeof(char));
      strcat(headers[x] + hlen, this_one);
      FREE(&this_one);
    }

    this_one = NULL;
  }

  if (is_autocrypt)
    fputs("\n", out_autocrypt);

  /* Now output the headers in order */
  for (x = 0; x < hdr_count; x++)
  {
    if (headers[x])
    {
      /* We couldn't do the prefixing when reading because RFC2047
       * decoding may have concatenated lines.
       */

      if (flags & (CH_DECODE | CH_PREFIX))
      {
        if (mutt_write_one_header(out, 0, headers[x], (flags & CH_PREFIX) ? prefix : 0,
                                  mutt_window_wrap_cols(MuttIndexWindow, Wrap), flags) == -1)
        {
          error = true;
          break;
        }
      }
      else
      {
        if (fputs(headers[x], out) == EOF)
        {
          error = true;
          break;
        }
      }
    }
  }

  /* Free in a separate loop to be sure that all headers are freed
   * in case of error. */
  for (x = 0; x < hdr_count; x++)
    FREE(&headers[x]);
  FREE(&headers);

  if (error)
    return -1;
  return 0;
}

/**
 * mutt_copy_header - Copy email header
 * @param in     FILE pointer to read from
 * @param h      Email Header
 * @param out    FILE pointer to write to
 * @param flags  Flags, see below
 * @param prefix Prefix for quoting headers
 * @retval  0 Success
 * @retval -1 Failure
 *
 * flags:
 * * #CH_DECODE       RFC2047 header decoding
 * * #CH_FROM         retain the "From " message separator
 * * #CH_FORCE_FROM   give CH_FROM precedence over CH_WEED
 * * #CH_MIME         ignore MIME fields
 * * #CH_NOLEN        don't write Content-Length: and Lines:
 * * #CH_NONEWLINE    don't output a newline after the header
 * * #CH_NOSTATUS     ignore the Status: and X-Status:
 * * #CH_PREFIX       quote header with $indent_string
 * * #CH_REORDER      output header in order specified by `hdr_order'
 * * #CH_TXTPLAIN     generate text/plain MIME headers [hack alert.]
 * * #CH_UPDATE       write new Status: and X-Status:
 * * #CH_UPDATE_LEN   write new Content-Length: and Lines:
 * * #CH_XMIT         ignore Lines: and Content-Length:
 * * #CH_WEED         do header weeding
 * * #CH_NOQFROM      ignore ">From " line
 * * #CH_UPDATE_IRT   update the In-Reply-To: header
 * * #CH_UPDATE_REFS  update the References: header
 * * #CH_VIRTUAL      write virtual header lines too
 *
 * prefix
 * * string to use if CH_PREFIX is set
 */
int mutt_copy_header(FILE *in, struct Header *h, FILE *out, int flags, const char *prefix, FILE *out_autocrypt)
{
  if (h->env)
    flags |= (h->env->irt_changed ? CH_UPDATE_IRT : 0) |
             (h->env->refs_changed ? CH_UPDATE_REFS : 0);

  if (mutt_copy_hdr(in, out, h->offset, h->content->offset, flags, prefix, out_autocrypt) == -1)
    return -1;

  if (flags & CH_TXTPLAIN)
  {
    char chsbuf[SHORT_STRING];
    char buffer[SHORT_STRING];
    fputs("MIME-Version: 1.0\n", out);
    fputs("Content-Transfer-Encoding: 8bit\n", out);
    fputs("Content-Type: text/plain; charset=", out);
    mutt_ch_canonical_charset(chsbuf, sizeof(chsbuf), Charset ? Charset : "us-ascii");
    mutt_addr_cat(buffer, sizeof(buffer), chsbuf, MimeSpecials);
    fputs(buffer, out);
    fputc('\n', out);
  }

  if ((flags & CH_UPDATE_IRT) && !STAILQ_EMPTY(&h->env->in_reply_to))
  {
    fputs("In-Reply-To:", out);
    struct ListNode *np;
    STAILQ_FOREACH(np, &h->env->in_reply_to, entries)
    {
      fputc(' ', out);
      fputs(np->data, out);
    }
    fputc('\n', out);
  }

  if ((flags & CH_UPDATE_REFS) && !STAILQ_EMPTY(&h->env->references))
  {
    fputs("References:", out);
    mutt_write_references(&h->env->references, out, 0);
    fputc('\n', out);
  }

  if ((flags & CH_UPDATE) && (flags & CH_NOSTATUS) == 0)
  {
    if (h->old || h->read)
    {
      fputs("Status: ", out);
      if (h->read)
        fputs("RO", out);
      else if (h->old)
        fputc('O', out);
      fputc('\n', out);
    }

    if (h->flagged || h->replied)
    {
      fputs("X-Status: ", out);
      if (h->replied)
        fputc('A', out);
      if (h->flagged)
        fputc('F', out);
      fputc('\n', out);
    }
  }

  if (flags & CH_UPDATE_LEN && (flags & CH_NOLEN) == 0)
  {
    fprintf(out, "Content-Length: " OFF_T_FMT "\n", h->content->length);
    if (h->lines != 0 || h->content->length == 0)
      fprintf(out, "Lines: %d\n", h->lines);
  }

#ifdef USE_NOTMUCH
  if (flags & CH_VIRTUAL)
  {
    /* Add some fake headers based on notmuch data */
    char *folder = nm_header_get_folder(h);
    if (folder && !(Weed && mutt_matches_ignore("folder")))
    {
      char buf[LONG_STRING];
      mutt_str_strfcpy(buf, folder, sizeof(buf));
      mutt_pretty_mailbox(buf, sizeof(buf));

      fputs("Folder: ", out);
      fputs(buf, out);
      fputc('\n', out);
    }
  }
#endif
  char *tags = driver_tags_get(&h->tags);
  if (tags && !(Weed && mutt_matches_ignore("tags")))
  {
    fputs("Tags: ", out);
    fputs(tags, out);
    fputc('\n', out);
  }
  FREE(&tags);

  if (flags & CH_UPDATE_LABEL)
  {
    h->xlabel_changed = false;
    if (h->env->x_label)
      if (fprintf(out, "X-Label: %s\n", h->env->x_label) != 10 + strlen(h->env->x_label))
        return -1;
  }

  if ((flags & CH_NONEWLINE) == 0)
  {
    if (flags & CH_PREFIX)
      fputs(prefix, out);
    fputc('\n', out); /* add header terminator */
  }

  if (ferror(out) || feof(out))
    return -1;

  return 0;
}

/**
 * count_delete_lines - Count lines to be deleted in this email body
 * @param fp      FILE pointer to read from
 * @param b       Email Body
 * @param length  Number of bytes to be deleted
 * @param datelen Length of the date
 * @retval num Number of lines to be deleted
 *
 * Count the number of lines and bytes to be deleted in this body
 */
static int count_delete_lines(FILE *fp, struct Body *b, LOFF_T *length, size_t datelen)
{
  int dellines = 0;

  if (b->deleted)
  {
    fseeko(fp, b->offset, SEEK_SET);
    for (long l = b->length; l; l--)
    {
      const int ch = getc(fp);
      if (ch == EOF)
        break;
      if (ch == '\n')
        dellines++;
    }
    dellines -= 3;
    *length -= b->length - (84 + datelen);
    /* Count the number of digits exceeding the first one to write the size */
    for (long l = 10; b->length >= l; l *= 10)
      (*length)++;
  }
  else
  {
    for (b = b->parts; b; b = b->next)
      dellines += count_delete_lines(fp, b, length, datelen);
  }
  return dellines;
}

/**
 * mutt_copy_message_fp - make a copy of a message from a FILE pointer
 *
 * @param fpout   FILE pointer to write to
 * @param src     Source mailbox
 * @param hdr     Email Header
 * @param flags   Flags, see: mutt_copy_message_fp()
 * @param chflags Header flags, see: mutt_copy_header()
 * @retval  0 Success
 * @retval -1 Failure
 *
 * see mutt_copy_message_fp_autocrypt
 */
int mutt_copy_message_fp(FILE *fpout, FILE *fpin, struct Header *hdr, int flags, int chflags)
{
  return mutt_copy_message_fp_autocrypt(fpout, fpin, hdr, flags, chflags, NULL);
}

/**
 * mutt_copy_message_fp_autocrypt - make a copy of a message from a FILE pointer
 * @param fpout   Where to write output
 * @param fpin    Where to get input
 * @param hdr     Header of message being copied
 * @param flags   See below
 * @param chflags Flags to mutt_copy_header()
 * @param out_autocrypt Where to write autocrypt related headers
 * @retval  0 Success
 * @retval -1 Failure
 *
 * * #MUTT_CM_NOHEADER   don't copy header
 * * #MUTT_CM_PREFIX     quote header and body
 * * #MUTT_CM_DECODE     decode message body to text/plain
 * * #MUTT_CM_DISPLAY    displaying output to the user
 * * #MUTT_CM_PRINTING   printing the message
 * * #MUTT_CM_UPDATE     update structures in memory after syncing
 * * #MUTT_CM_DECODE_PGP used for decoding PGP messages
 * * #MUTT_CM_CHARCONV   perform character set conversion
 */
int mutt_copy_message_fp_autocrypt(FILE *fpout, FILE *fpin, struct Header *hdr, int flags, int chflags, FILE *out_autocrypt)
{
  struct Body *body = hdr->content;
  char prefix[SHORT_STRING];
  struct State s;
  LOFF_T new_offset = -1;
  int rc = 0;

  if (flags & MUTT_CM_PREFIX)
  {
    if (TextFlowed)
      mutt_str_strfcpy(prefix, ">", sizeof(prefix));
    else
      mutt_make_string_flags(prefix, sizeof(prefix), NONULL(IndentString), Context, hdr, 0);
  }

  if (hdr->xlabel_changed)
    chflags |= CH_UPDATE_LABEL;

  if ((flags & MUTT_CM_NOHEADER) == 0)
  {
    if (flags & MUTT_CM_PREFIX)
      chflags |= CH_PREFIX;

    else if (hdr->attach_del && (chflags & CH_UPDATE_LEN))
    {
      int new_lines;
      LOFF_T new_length = body->length;
      char date[SHORT_STRING];

      mutt_date_make_date(date, sizeof(date));
      int dlen = mutt_str_strlen(date);
      if (dlen == 0)
        return -1;

      date[5] = '\"';
      date[dlen - 1] = '\"';

      /* Count the number of lines and bytes to be deleted */
      fseeko(fpin, body->offset, SEEK_SET);
      new_lines = hdr->lines - count_delete_lines(fpin, body, &new_length, dlen);

      /* Copy the headers */
      if (mutt_copy_header(fpin, hdr, fpout, chflags | CH_NOLEN | CH_NONEWLINE, NULL, NULL))
        return -1;
      fprintf(fpout, "Content-Length: " OFF_T_FMT "\n", new_length);
      if (new_lines <= 0)
        new_lines = 0;
      else
        fprintf(fpout, "Lines: %d\n", new_lines);

      putc('\n', fpout);
      if (ferror(fpout) || feof(fpout))
        return -1;
      new_offset = ftello(fpout);

      /* Copy the body */
      if (fseeko(fpin, body->offset, SEEK_SET) < 0)
        return -1;
      if (copy_delete_attach(body, fpin, fpout, date))
        return -1;

      LOFF_T fail = ((ftello(fpout) - new_offset) - new_length);
      if (fail)
      {
        mutt_error("The length calculation was wrong by %ld bytes", fail);
        new_length += fail;
      }

      /* Update original message if we are sync'ing a mailfolder */
      if (flags & MUTT_CM_UPDATE)
      {
        hdr->attach_del = false;
        hdr->lines = new_lines;
        body->offset = new_offset;

        /* update the total size of the mailbox to reflect this deletion */
        Context->size -= body->length - new_length;
        /*
         * if the message is visible, update the visible size of the mailbox
         * as well.
         */
        if (Context->v2r[hdr->msgno] != -1)
          Context->vsize -= body->length - new_length;

        body->length = new_length;
        mutt_body_free(&body->parts);
      }
      return 0;
    }

    if (mutt_copy_header(fpin, hdr, fpout, chflags,
                         (chflags & CH_PREFIX) ? prefix : NULL, out_autocrypt) == -1)
    {
      return -1;
    }

    new_offset = ftello(fpout);
  }

  if (flags & MUTT_CM_DECODE)
  {
    /* now make a text/plain version of the message */
    memset(&s, 0, sizeof(struct State));
    s.fpin = fpin;
    s.fpout = fpout;
    s.fpout_gossip = out_autocrypt;
    if (flags & MUTT_CM_PREFIX)
      s.prefix = prefix;
    if (flags & MUTT_CM_DISPLAY)
      s.flags |= MUTT_DISPLAY;
    if (flags & MUTT_CM_PRINTING)
      s.flags |= MUTT_PRINTING;
    if (flags & MUTT_CM_WEED)
      s.flags |= MUTT_WEED;
    if (flags & MUTT_CM_CHARCONV)
      s.flags |= MUTT_CHARCONV;
    if (flags & MUTT_CM_REPLYING)
      s.flags |= MUTT_REPLYING;

    if ((WithCrypto != 0) && flags & MUTT_CM_VERIFY)
      s.flags |= MUTT_VERIFY;

    rc = mutt_body_handler(body, &s);
  }
  else if ((WithCrypto != 0) && (flags & MUTT_CM_DECODE_CRYPT) && (hdr->security & ENCRYPT))
  {
    struct Body *cur = NULL;
    FILE *fp = NULL;

    if (((WithCrypto & APPLICATION_PGP) != 0) && (flags & MUTT_CM_DECODE_PGP) &&
        (hdr->security & APPLICATION_PGP) && hdr->content->type == TYPEMULTIPART)
    {
      if (crypt_pgp_decrypt_mime(fpin, &fp, hdr->content, &cur))
        return -1;
      fputs("MIME-Version: 1.0\n", fpout);
    }

    if (((WithCrypto & APPLICATION_SMIME) != 0) && (flags & MUTT_CM_DECODE_SMIME) &&
        (hdr->security & APPLICATION_SMIME) && hdr->content->type == TYPEAPPLICATION)
    {
      if (crypt_smime_decrypt_mime(fpin, &fp, hdr->content, &cur))
        return -1;
    }

    if (!cur)
    {
      mutt_error(_("No decryption engine available for message"));
      return -1;
    }

    mutt_write_mime_header(cur, fpout);
    fputc('\n', fpout);

    if (fseeko(fp, cur->offset, SEEK_SET) < 0)
      return -1;
    if (mutt_file_copy_bytes(fp, fpout, cur->length) == -1)
    {
      mutt_file_fclose(&fp);
      mutt_body_free(&cur);
      return -1;
    }
    mutt_body_free(&cur);
    mutt_file_fclose(&fp);
  }
  else
  {
    if (fseeko(fpin, body->offset, SEEK_SET) < 0)
      return -1;
    if (flags & MUTT_CM_PREFIX)
    {
      int c;
      size_t bytes = body->length;

      fputs(prefix, fpout);

      while ((c = fgetc(fpin)) != EOF && bytes--)
      {
        fputc(c, fpout);
        if (c == '\n')
        {
          fputs(prefix, fpout);
        }
      }
    }
    else if (mutt_file_copy_bytes(fpin, fpout, body->length) == -1)
      return -1;
  }

  if ((flags & MUTT_CM_UPDATE) && (flags & MUTT_CM_NOHEADER) == 0 && new_offset != -1)
  {
    body->offset = new_offset;
    mutt_body_free(&body->parts);
  }

  return rc;
}

int mutt_copy_message_ctx(FILE *fpout, struct Context *src, struct Header *hdr,
                          int flags, int chflags)
{
  return mutt_copy_message_ctx_autocrypt(fpout, src, hdr, flags, chflags, NULL);
}

/**
 * mutt_copy_message_ctx - Copy a message from a Context
 * @param fpout   FILE pointer to write to
 * @param src     Source mailbox
 * @param hdr     Email Header
 * @param flags   Flags, see: mutt_copy_message_fp()
 * @param chflags Header flags, see: mutt_copy_header()
 * @retval  0 Success
 * @retval -1 Failure
 *
 * should be made to return -1 on fatal errors, and 1 on non-fatal errors
 * like partial decode, where it is worth displaying as much as possible
 */
int mutt_copy_message_ctx_autocrypt(FILE *fpout, struct Context *src, struct Header *hdr,
                          int flags, int chflags, FILE *out_autocrypt)
{
  struct Message *msg = mx_open_message(src, hdr->msgno);
  if (!msg)
    return -1;
  if (!hdr->content)
    return -1;
  int r = mutt_copy_message_fp_autocrypt(fpout, msg->fp, hdr, flags, chflags, out_autocrypt);
  if ((r == 0) && (ferror(fpout) || feof(fpout)))
  {
    mutt_debug(1, "failed to detect EOF!\n");
    r = -1;
  }
  mx_close_message(src, &msg);
  return r;
}

/**
 * append_message - appends a copy of the given message to a mailbox
 * @param dest    destination mailbox
 * @param fpin    where to get input
 * @param src     source mailbox
 * @param hdr     message being copied
 * @param flags   mutt_open_copy_message() flags
 * @param chflags mutt_copy_header() flags
 * @retval  0 Success
 * @retval -1 Error
 */
static int append_message(struct Context *dest, FILE *fpin, struct Context *src,
                          struct Header *hdr, int flags, int chflags)
{
  char buf[STRING];
  struct Message *msg = NULL;
  int r;

  if (fseeko(fpin, hdr->offset, SEEK_SET) < 0)
    return -1;
  if (fgets(buf, sizeof(buf), fpin) == NULL)
    return -1;

  msg = mx_open_new_message(dest, hdr, is_from(buf, NULL, 0, NULL) ? 0 : MUTT_ADD_FROM);
  if (!msg)
    return -1;
  if (dest->magic == MUTT_MBOX || dest->magic == MUTT_MMDF)
    chflags |= CH_FROM | CH_FORCE_FROM;
  chflags |= (dest->magic == MUTT_MAILDIR ? CH_NOSTATUS : CH_UPDATE);
  r = mutt_copy_message_fp(msg->fp, fpin, hdr, flags, chflags);
  if (mx_commit_message(msg, dest) != 0)
    r = -1;

#ifdef USE_NOTMUCH
  if (msg->commited_path && dest->magic == MUTT_MAILDIR && src->magic == MUTT_NOTMUCH)
    nm_update_filename(src, NULL, msg->commited_path, hdr);
#endif

  mx_close_message(dest, &msg);
  return r;
}

/**
 * mutt_append_message - Append a message
 * @param dest    Destination Mailbox
 * @param src     Source Mailbox
 * @param hdr     Email Header
 * @param cmflags mutt_open_copy_message() flags
 * @param chflags mutt_copy_header() flags
 * @retval  0 Success
 * @retval -1 Failure
 */
int mutt_append_message(struct Context *dest, struct Context *src,
                        struct Header *hdr, int cmflags, int chflags)
{
  struct Message *msg = mx_open_message(src, hdr->msgno);
  if (!msg)
    return -1;
  int r = append_message(dest, msg->fp, src, hdr, cmflags, chflags);
  mx_close_message(src, &msg);
  return r;
}

/**
 * copy_delete_attach - Copy a message, deleting marked attachments
 * @param b     Email Body
 * @param fpin  FILE pointer to read from
 * @param fpout FILE pointer to write to
 * @param date  Date stamp
 * @retval  0 Success
 * @retval -1 Failure
 *
 * This function copies a message body, while deleting _in_the_copy_
 * any attachments which are marked for deletion.
 * Nothing is changed in the original message -- this is left to the caller.
 */
static int copy_delete_attach(struct Body *b, FILE *fpin, FILE *fpout, char *date)
{
  struct Body *part = NULL;

  for (part = b->parts; part; part = part->next)
  {
    if (part->deleted || part->parts)
    {
      /* Copy till start of this part */
      if (mutt_file_copy_bytes(fpin, fpout, part->hdr_offset - ftello(fpin)))
        return -1;

      if (part->deleted)
      {
        fprintf(
            fpout,
            "Content-Type: message/external-body; access-type=x-mutt-deleted;\n"
            "\texpiration=%s; length=" OFF_T_FMT "\n"
            "\n",
            date + 5, part->length);
        if (ferror(fpout))
          return -1;

        /* Copy the original mime headers */
        if (mutt_file_copy_bytes(fpin, fpout, part->offset - ftello(fpin)))
          return -1;

        /* Skip the deleted body */
        fseeko(fpin, part->offset + part->length, SEEK_SET);
      }
      else
      {
        if (copy_delete_attach(part, fpin, fpout, date))
          return -1;
      }
    }
  }

  /* Copy the last parts */
  if (mutt_file_copy_bytes(fpin, fpout, b->offset + b->length - ftello(fpin)))
    return -1;

  return 0;
}

/**
 * format_address_header - Write address headers to a buffer
 * @param h Array of header strings
 * @param a Email Address
 *
 * This function is the equivalent of mutt_write_address_list(), but writes to
 * a buffer instead of writing to a stream.  mutt_write_address_list could be
 * re-used if we wouldn't store all the decoded headers in a huge array, first.
 *
 * TODO fix that.
 */
static void format_address_header(char **h, struct Address *a)
{
  char buf[HUGE_STRING];
  char cbuf[STRING];
  char c2buf[STRING];
  char *p = NULL;
  size_t linelen, buflen, plen;

  linelen = mutt_str_strlen(*h);
  plen = linelen;
  buflen = linelen + 3;

  mutt_mem_realloc(h, buflen);
  for (int count = 0; a; a = a->next, count++)
  {
    struct Address *tmp = a->next;
    a->next = NULL;
    *buf = *cbuf = *c2buf = '\0';
    const size_t l = mutt_addr_write(buf, sizeof(buf), a, false);
    a->next = tmp;

    if (count && linelen + l > 74)
    {
      strcpy(cbuf, "\n\t");
      linelen = l + 8;
    }
    else
    {
      if (a->mailbox)
      {
        strcpy(cbuf, " ");
        linelen++;
      }
      linelen += l;
    }
    if (!a->group && a->next && a->next->mailbox)
    {
      linelen++;
      buflen++;
      strcpy(c2buf, ",");
    }

    const size_t cbuflen = mutt_str_strlen(cbuf);
    const size_t c2buflen = mutt_str_strlen(c2buf);
    buflen += l + cbuflen + c2buflen;
    mutt_mem_realloc(h, buflen);
    p = *h;
    strcat(p + plen, cbuf);
    plen += cbuflen;
    strcat(p + plen, buf);
    plen += l;
    strcat(p + plen, c2buf);
    plen += c2buflen;
  }

  /* Space for this was allocated in the beginning of this function. */
  strcat(p + plen, "\n");
}

/**
 * address_header_decode - Parse an email's headers
 * @param h Array of header strings
 * @retval 0 Success
 * @retval 1 Failure
 */
static int address_header_decode(char **h)
{
  char *s = *h;
  size_t l;
  bool rp = false;

  struct Address *a = NULL;
  struct Address *cur = NULL;

  switch (tolower((unsigned char) *s))
  {
    case 'b':
    {
      if (mutt_str_strncasecmp(s, "bcc:", 4) != 0)
        return 0;
      l = 4;
      break;
    }
    case 'c':
    {
      if (mutt_str_strncasecmp(s, "cc:", 3) != 0)
        return 0;
      l = 3;
      break;
    }
    case 'f':
    {
      if (mutt_str_strncasecmp(s, "from:", 5) != 0)
        return 0;
      l = 5;
      break;
    }
    case 'm':
    {
      if (mutt_str_strncasecmp(s, "mail-followup-to:", 17) != 0)
        return 0;
      l = 17;
      break;
    }
    case 'r':
    {
      if (mutt_str_strncasecmp(s, "return-path:", 12) == 0)
      {
        l = 12;
        rp = true;
        break;
      }
      else if (mutt_str_strncasecmp(s, "reply-to:", 9) == 0)
      {
        l = 9;
        break;
      }
      return 0;
    }
    case 's':
    {
      if (mutt_str_strncasecmp(s, "sender:", 7) != 0)
        return 0;
      l = 7;
      break;
    }
    case 't':
    {
      if (mutt_str_strncasecmp(s, "to:", 3) != 0)
        return 0;
      l = 3;
      break;
    }
    default:
      return 0;
  }

  a = mutt_addr_parse_list(a, s + l);
  if (!a)
    return 0;

  mutt_addrlist_to_local(a);
  rfc2047_decode_addrlist(a);
  for (cur = a; cur; cur = cur->next)
    if (cur->personal)
      mutt_str_dequote_comment(cur->personal);

  /* angle brackets for return path are mandated by RFC5322,
   * so leave Return-Path as-is */
  if (rp)
    *h = mutt_str_strdup(s);
  else
  {
    *h = mutt_mem_calloc(1, l + 2);
    mutt_str_strfcpy(*h, s, l + 1);
    format_address_header(h, a);
  }

  mutt_addr_free(&a);

  FREE(&s);
  return 1;
}
