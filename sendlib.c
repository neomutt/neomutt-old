/**
 * @file
 * Miscellaneous functions for sending an email
 *
 * @authors
 * Copyright (C) 1996-2002,2009-2012 Michael R. Elkins <me@mutt.org>
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
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include "lib/lib.h"
#include "mutt.h"
#include "address.h"
#include "body.h"
#include "buffy.h"
#include "charset.h"
#include "content.h"
#include "context.h"
#include "copy.h"
#include "envelope.h"
#include "filter.h"
#include "format_flags.h"
#include "globals.h"
#include "header.h"
#include "list.h"
#include "mailbox.h"
#include "mime.h"
#include "mutt_curses.h"
#include "mutt_idna.h"
#include "mx.h"
#include "ncrypt/ncrypt.h"
#include "options.h"
#include "pager.h"
#include "parameter.h"
#include "protos.h"
#include "rfc2047.h"
#include "rfc2231.h"
#include "rfc822.h"
#include "state.h"
#ifdef USE_NNTP
#include "nntp.h"
#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else /* Make sure EX_OK is defined <philiph@pobox.com> */
#define EX_OK 0
#endif
/* If you are debugging this file, comment out the following line. */
#define NDEBUG
#ifdef NDEBUG
#define assert(x)
#else
#include <assert.h>
#endif

extern char RFC822Specials[];

const char MimeSpecials[] = "@.,;:<>[]\\\"()?/= \t";

static void encode_quoted(FGETCONV *fc, FILE *fout, int istext)
{
  int c, linelen = 0;
  char line[77], savechar;

  while ((c = fgetconv(fc)) != EOF)
  {
    /* Wrap the line if needed. */
    if (linelen == 76 && ((istext && c != '\n') || !istext))
    {
      /* If the last character is "quoted", then be sure to move all three
       * characters to the next line.  Otherwise, just move the last
       * character...
       */
      if (line[linelen - 3] == '=')
      {
        line[linelen - 3] = 0;
        fputs(line, fout);
        fputs("=\n", fout);
        line[linelen] = 0;
        line[0] = '=';
        line[1] = line[linelen - 2];
        line[2] = line[linelen - 1];
        linelen = 3;
      }
      else
      {
        savechar = line[linelen - 1];
        line[linelen - 1] = '=';
        line[linelen] = 0;
        fputs(line, fout);
        fputc('\n', fout);
        line[0] = savechar;
        linelen = 1;
      }
    }

    /* Escape lines that begin with/only contain "the message separator". */
    if (linelen == 4 && (mutt_strncmp("From", line, 4) == 0))
    {
      strfcpy(line, "=46rom", sizeof(line));
      linelen = 6;
    }
    else if (linelen == 4 && (mutt_strncmp("from", line, 4) == 0))
    {
      strfcpy(line, "=66rom", sizeof(line));
      linelen = 6;
    }
    else if (linelen == 1 && line[0] == '.')
    {
      strfcpy(line, "=2E", sizeof(line));
      linelen = 3;
    }

    if (c == '\n' && istext)
    {
      /* Check to make sure there is no trailing space on this line. */
      if (linelen > 0 && (line[linelen - 1] == ' ' || line[linelen - 1] == '\t'))
      {
        if (linelen < 74)
        {
          sprintf(line + linelen - 1, "=%2.2X", (unsigned char) line[linelen - 1]);
          fputs(line, fout);
        }
        else
        {
          int savechar2 = line[linelen - 1];

          line[linelen - 1] = '=';
          line[linelen] = 0;
          fputs(line, fout);
          fprintf(fout, "\n=%2.2X", (unsigned char) savechar2);
        }
      }
      else
      {
        line[linelen] = 0;
        fputs(line, fout);
      }
      fputc('\n', fout);
      linelen = 0;
    }
    else if (c != 9 && (c < 32 || c > 126 || c == '='))
    {
      /* Check to make sure there is enough room for the quoted character.
       * If not, wrap to the next line.
       */
      if (linelen > 73)
      {
        line[linelen++] = '=';
        line[linelen] = 0;
        fputs(line, fout);
        fputc('\n', fout);
        linelen = 0;
      }
      sprintf(line + linelen, "=%2.2X", (unsigned char) c);
      linelen += 3;
    }
    else
    {
      /* Don't worry about wrapping the line here.  That will happen during
       * the next iteration when I'll also know what the next character is.
       */
      line[linelen++] = c;
    }
  }

  /* Take care of anything left in the buffer */
  if (linelen > 0)
  {
    if (line[linelen - 1] == ' ' || line[linelen - 1] == '\t')
    {
      /* take care of trailing whitespace */
      if (linelen < 74)
        sprintf(line + linelen - 1, "=%2.2X", (unsigned char) line[linelen - 1]);
      else
      {
        savechar = line[linelen - 1];
        line[linelen - 1] = '=';
        line[linelen] = 0;
        fputs(line, fout);
        fputc('\n', fout);
        sprintf(line, "=%2.2X", (unsigned char) savechar);
      }
    }
    else
      line[linelen] = 0;
    fputs(line, fout);
  }
}

/**
 * struct B64Context - Cursor for the Base64 conversion
 */
struct B64Context
{
  char buffer[3];
  short size;
  short linelen;
};

static int b64_init(struct B64Context *ctx)
{
  memset(ctx->buffer, '\0', sizeof(ctx->buffer));
  ctx->size = 0;
  ctx->linelen = 0;

  return 0;
}

static void b64_flush(struct B64Context *ctx, FILE *fout)
{
  /* for some reasons, mutt_to_base64 expects the
   * output buffer to be larger than 10B */
  char encoded[11];
  size_t ret;

  if (!ctx->size)
    return;

  if (ctx->linelen >= 72)
  {
    fputc('\n', fout);
    ctx->linelen = 0;
  }

  /* ret should always be equal to 4 here, because ctx->size
   * is a value between 1 and 3 (included), but let's not hardcode it
   * and prefer the return value of the function */
  ret = mutt_to_base64(encoded, ctx->buffer, ctx->size, sizeof(encoded));
  for (size_t i = 0; i < ret; i++)
  {
    fputc(encoded[i], fout);
    ctx->linelen++;
  }

  ctx->size = 0;
}

static void b64_putc(struct B64Context *ctx, char c, FILE *fout)
{
  if (ctx->size == 3)
    b64_flush(ctx, fout);

  ctx->buffer[ctx->size++] = c;
}

static void encode_base64(FGETCONV *fc, FILE *fout, int istext)
{
  struct B64Context ctx;
  int ch, ch1 = EOF;

  b64_init(&ctx);

  while ((ch = fgetconv(fc)) != EOF)
  {
    if (SigInt == 1)
    {
      SigInt = 0;
      return;
    }
    if (istext && ch == '\n' && ch1 != '\r')
      b64_putc(&ctx, '\r', fout);
    b64_putc(&ctx, ch, fout);
    ch1 = ch;
  }
  b64_flush(&ctx, fout);
  fputc('\n', fout);
}

static void encode_8bit(FGETCONV *fc, FILE *fout, int istext)
{
  int ch;

  while ((ch = fgetconv(fc)) != EOF)
  {
    if (SigInt == 1)
    {
      SigInt = 0;
      return;
    }
    fputc(ch, fout);
  }
}

int mutt_write_mime_header(struct Body *a, FILE *f)
{
  char buffer[STRING];
  char *t = NULL;
  char *fn = NULL;
  int len;
  int tmplen;
  int encode;

  fprintf(f, "Content-Type: %s/%s", TYPE(a), a->subtype);

  if (a->parameter)
  {
    len = 25 + mutt_strlen(a->subtype); /* approximate len. of content-type */

    for (struct Parameter *p = a->parameter; p; p = p->next)
    {
      char *tmp = NULL;

      if (!p->value)
        continue;

      fputc(';', f);

      buffer[0] = 0;
      tmp = safe_strdup(p->value);
      encode = rfc2231_encode_string(&tmp);
      rfc822_cat(buffer, sizeof(buffer), tmp, MimeSpecials);

      /* Dirty hack to make messages readable by Outlook Express
       * for the Mac: force quotes around the boundary parameter
       * even when they aren't needed.
       */

      if ((mutt_strcasecmp(p->attribute, "boundary") == 0) && (strcmp(buffer, tmp) == 0))
        snprintf(buffer, sizeof(buffer), "\"%s\"", tmp);

      FREE(&tmp);

      tmplen = mutt_strlen(buffer) + mutt_strlen(p->attribute) + 1;

      if (len + tmplen + 2 > 76)
      {
        fputs("\n\t", f);
        len = tmplen + 8;
      }
      else
      {
        fputc(' ', f);
        len += tmplen + 1;
      }

      fprintf(f, "%s%s=%s", p->attribute, encode ? "*" : "", buffer);
    }
  }

  fputc('\n', f);

  if (a->description)
    fprintf(f, "Content-Description: %s\n", a->description);

  if (a->disposition != DISPNONE)
  {
    const char *dispstr[] = { "inline", "attachment", "form-data" };

    if (a->disposition < sizeof(dispstr) / sizeof(char *))
    {
      fprintf(f, "Content-Disposition: %s", dispstr[a->disposition]);

      if (a->use_disp)
      {
        fn = a->d_filename;
        if (!fn)
          fn = a->filename;

        if (fn)
        {
          char *tmp = NULL;

          /* Strip off the leading path... */
          if ((t = strrchr(fn, '/')))
            t++;
          else
            t = fn;

          buffer[0] = 0;
          tmp = safe_strdup(t);
          encode = rfc2231_encode_string(&tmp);
          rfc822_cat(buffer, sizeof(buffer), tmp, MimeSpecials);
          FREE(&tmp);
          fprintf(f, "; filename%s=%s", encode ? "*" : "", buffer);
        }
      }

      fputc('\n', f);
    }
    else
    {
      mutt_debug(1, "ERROR: invalid content-disposition %d\n", a->disposition);
    }
  }

  if (a->encoding != ENC7BIT)
    fprintf(f, "Content-Transfer-Encoding: %s\n", ENCODING(a->encoding));

  /* Do NOT add the terminator here!!! */
  return (ferror(f) ? -1 : 0);
}

static bool write_as_text_part(struct Body *b)
{
  return (mutt_is_text_part(b) || ((WithCrypto & APPLICATION_PGP) && mutt_is_application_pgp(b)));
}

int mutt_write_mime_body(struct Body *a, FILE *f)
{
  char *p, boundary[SHORT_STRING];
  char send_charset[SHORT_STRING];
  FILE *fpin = NULL;
  FGETCONV *fc = NULL;

  if (a->type == TYPEMULTIPART)
  {
    /* First, find the boundary to use */
    p = mutt_get_parameter("boundary", a->parameter);
    if (!p)
    {
      mutt_debug(1, "mutt_write_mime_body(): no boundary parameter found!\n");
      mutt_error(_("No boundary parameter found! [report this error]"));
      return -1;
    }
    strfcpy(boundary, p, sizeof(boundary));

    for (struct Body *t = a->parts; t; t = t->next)
    {
      fprintf(f, "\n--%s\n", boundary);
      if (mutt_write_mime_header(t, f) == -1)
        return -1;
      fputc('\n', f);
      if (mutt_write_mime_body(t, f) == -1)
        return -1;
    }
    fprintf(f, "\n--%s--\n", boundary);
    return (ferror(f) ? -1 : 0);
  }

  /* This is pretty gross, but it's the best solution for now... */
  if ((WithCrypto & APPLICATION_PGP) && a->type == TYPEAPPLICATION &&
      (mutt_strcmp(a->subtype, "pgp-encrypted") == 0))
  {
    fputs("Version: 1\n", f);
    return 0;
  }

  fpin = fopen(a->filename, "r");
  if (!fpin)
  {
    mutt_debug(1, "write_mime_body: %s no longer exists!\n", a->filename);
    mutt_error(_("%s no longer exists!"), a->filename);
    return -1;
  }

  if (a->type == TYPETEXT && (!a->noconv))
    fc = fgetconv_open(fpin, a->charset,
                       mutt_get_body_charset(send_charset, sizeof(send_charset), a), 0);
  else
    fc = fgetconv_open(fpin, 0, 0, 0);

  mutt_allow_interrupt(1);
  if (a->encoding == ENCQUOTEDPRINTABLE)
    encode_quoted(fc, f, write_as_text_part(a));
  else if (a->encoding == ENCBASE64)
    encode_base64(fc, f, write_as_text_part(a));
  else if (a->type == TYPETEXT && (!a->noconv))
    encode_8bit(fc, f, write_as_text_part(a));
  else
    mutt_copy_stream(fpin, f);
  mutt_allow_interrupt(0);

  fgetconv_close(&fc);
  safe_fclose(&fpin);

  if (SigInt == 1)
  {
    SigInt = 0;
    return -1;
  }
  return (ferror(f) ? -1 : 0);
}

#undef write_as_text_part

void mutt_generate_boundary(struct Parameter **parm)
{
  char rs[MUTT_RANDTAG_LEN + 1];

  mutt_rand_base32(rs, sizeof(rs) - 1);
  rs[MUTT_RANDTAG_LEN] = 0;
  mutt_set_parameter("boundary", rs, parm);
}

/**
 * struct ContentState - Info about the body of an email
 */
struct ContentState
{
  int from;
  int whitespace;
  int dot;
  int linelen;
  int was_cr;
};

static void update_content_info(struct Content *info, struct ContentState *s,
                                char *d, size_t dlen)
{
  int from = s->from;
  int whitespace = s->whitespace;
  int dot = s->dot;
  int linelen = s->linelen;
  int was_cr = s->was_cr;

  if (!d) /* This signals EOF */
  {
    if (was_cr)
      info->binary = true;
    if (linelen > info->linemax)
      info->linemax = linelen;

    return;
  }

  for (; dlen; d++, dlen--)
  {
    char ch = *d;

    if (was_cr)
    {
      was_cr = 0;
      if (ch != '\n')
      {
        info->binary = true;
      }
      else
      {
        if (whitespace)
          info->space = true;
        if (dot)
          info->dot = true;
        if (linelen > info->linemax)
          info->linemax = linelen;
        whitespace = 0;
        dot = 0;
        linelen = 0;
        continue;
      }
    }

    linelen++;
    if (ch == '\n')
    {
      info->crlf++;
      if (whitespace)
        info->space = true;
      if (dot)
        info->dot = true;
      if (linelen > info->linemax)
        info->linemax = linelen;
      whitespace = 0;
      linelen = 0;
      dot = 0;
    }
    else if (ch == '\r')
    {
      info->crlf++;
      info->cr = true;
      was_cr = 1;
      continue;
    }
    else if (ch & 0x80)
      info->hibin++;
    else if (ch == '\t' || ch == '\f')
    {
      info->ascii++;
      whitespace++;
    }
    else if (ch == 0)
    {
      info->nulbin++;
      info->lobin++;
    }
    else if (ch < 32 || ch == 127)
      info->lobin++;
    else
    {
      if (linelen == 1)
      {
        if ((ch == 'F') || (ch == 'f'))
          from = 1;
        else
          from = 0;
        if (ch == '.')
          dot = 1;
        else
          dot = 0;
      }
      else if (from)
      {
        if (linelen == 2 && ch != 'r')
          from = 0;
        else if (linelen == 3 && ch != 'o')
          from = 0;
        else if (linelen == 4)
        {
          if (ch == 'm')
            info->from = true;
          from = 0;
        }
      }
      if (ch == ' ')
        whitespace++;
      info->ascii++;
    }

    if (linelen > 1)
      dot = 0;
    if (ch != ' ' && ch != '\t')
      whitespace = 0;
  }

  s->from = from;
  s->whitespace = whitespace;
  s->dot = dot;
  s->linelen = linelen;
  s->was_cr = was_cr;
}

/* Define as 1 if iconv sometimes returns -1(EILSEQ) instead of transcribing. */
#define BUGGY_ICONV 1

/**
 * convert_file_to - Change the encoding of a file
 * @param[in]  file       File to convert
 * @param[in]  fromcode   Original encoding
 * @param[in]  ncodes     Number of target encodings
 * @param[in]  tocodes    List of target encodings
 * @param[out] tocode     Chosen encoding
 * @param[in]  info       Encoding information
 * @retval -1 Error, no conversion was possible
 * @retval >0 Success, number of bytes converted
 *
 * Find the best charset conversion of the file from fromcode into one
 * of the tocodes. If successful, set *tocode and Content *info and
 * return the number of characters converted inexactly.
 *
 * We convert via UTF-8 in order to avoid the condition -1(EINVAL),
 * which would otherwise prevent us from knowing the number of inexact
 * conversions. Where the candidate target charset is UTF-8 we avoid
 * doing the second conversion because iconv_open("UTF-8", "UTF-8")
 * fails with some libraries.
 *
 * We assume that the output from iconv is never more than 4 times as
 * long as the input for any pair of charsets we might be interested
 * in.
 */
static size_t convert_file_to(FILE *file, const char *fromcode, int ncodes,
                              const char **tocodes, int *tocode, struct Content *info)
{
  iconv_t cd1, *cd = NULL;
  char bufi[256], bufu[512], bufo[4 * sizeof(bufi)];
  ICONV_CONST char *ib = NULL, *ub = NULL;
  char *ob = NULL;
  size_t ibl, obl, ubl, ubl1, n, ret;
  struct Content *infos = NULL;
  struct ContentState *states = NULL;
  size_t *score = NULL;

  cd1 = mutt_iconv_open("utf-8", fromcode, 0);
  if (cd1 == (iconv_t)(-1))
    return -1;

  cd = safe_calloc(ncodes, sizeof(iconv_t));
  score = safe_calloc(ncodes, sizeof(size_t));
  states = safe_calloc(ncodes, sizeof(struct ContentState));
  infos = safe_calloc(ncodes, sizeof(struct Content));

  for (int i = 0; i < ncodes; i++)
  {
    if (mutt_strcasecmp(tocodes[i], "utf-8") != 0)
      cd[i] = mutt_iconv_open(tocodes[i], "utf-8", 0);
    else
    {
      /* Special case for conversion to UTF-8 */
      cd[i] = (iconv_t)(-1);
      score[i] = (size_t)(-1);
    }
  }

  rewind(file);
  ibl = 0;
  for (;;)
  {
    /* Try to fill input buffer */
    n = fread(bufi + ibl, 1, sizeof(bufi) - ibl, file);
    ibl += n;

    /* Convert to UTF-8 */
    ib = bufi;
    ob = bufu;
    obl = sizeof(bufu);
    n = iconv(cd1, ibl ? &ib : 0, &ibl, &ob, &obl);
    assert(n == (size_t)(-1) || !n);
    if (n == (size_t)(-1) && ((errno != EINVAL && errno != E2BIG) || ib == bufi))
    {
      assert(errno == EILSEQ || (errno == EINVAL && ib == bufi && ibl < sizeof(bufi)));
      ret = (size_t)(-1);
      break;
    }
    ubl1 = ob - bufu;

    /* Convert from UTF-8 */
    for (int i = 0; i < ncodes; i++)
    {
      if (cd[i] != (iconv_t)(-1) && score[i] != (size_t)(-1))
      {
        ub = bufu;
        ubl = ubl1;
        ob = bufo;
        obl = sizeof(bufo);
        n = iconv(cd[i], (ibl || ubl) ? &ub : 0, &ubl, &ob, &obl);
        if (n == (size_t)(-1))
        {
          assert(errno == E2BIG || (BUGGY_ICONV && (errno == EILSEQ || errno == ENOENT)));
          score[i] = (size_t)(-1);
        }
        else
        {
          score[i] += n;
          update_content_info(&infos[i], &states[i], bufo, ob - bufo);
        }
      }
      else if (cd[i] == (iconv_t)(-1) && score[i] == (size_t)(-1))
        /* Special case for conversion to UTF-8 */
        update_content_info(&infos[i], &states[i], bufu, ubl1);
    }

    if (ibl)
      /* Save unused input */
      memmove(bufi, ib, ibl);
    else if (!ubl1 && ib < bufi + sizeof(bufi))
    {
      ret = 0;
      break;
    }
  }

  if (!ret)
  {
    /* Find best score */
    ret = (size_t)(-1);
    for (int i = 0; i < ncodes; i++)
    {
      if (cd[i] == (iconv_t)(-1) && score[i] == (size_t)(-1))
      {
        /* Special case for conversion to UTF-8 */
        *tocode = i;
        ret = 0;
        break;
      }
      else if (cd[i] == (iconv_t)(-1) || score[i] == (size_t)(-1))
        continue;
      else if (ret == (size_t)(-1) || score[i] < ret)
      {
        *tocode = i;
        ret = score[i];
        if (!ret)
          break;
      }
    }
    if (ret != (size_t)(-1))
    {
      memcpy(info, &infos[*tocode], sizeof(struct Content));
      update_content_info(info, &states[*tocode], 0, 0); /* EOF */
    }
  }

  for (int i = 0; i < ncodes; i++)
    if (cd[i] != (iconv_t)(-1))
      iconv_close(cd[i]);

  iconv_close(cd1);
  FREE(&cd);
  FREE(&infos);
  FREE(&score);
  FREE(&states);

  return ret;
}

/**
 * convert_file_from_to - Convert a file between encodings
 *
 * Find the first of the fromcodes that gives a valid conversion and the best
 * charset conversion of the file into one of the tocodes. If successful, set
 * *fromcode and *tocode to dynamically allocated strings, set Content *info,
 * and return the number of characters converted inexactly. If no conversion
 * was possible, return -1.
 *
 * Both fromcodes and tocodes may be colon-separated lists of charsets.
 * However, if fromcode is zero then fromcodes is assumed to be the name of a
 * single charset even if it contains a colon.
 */
static size_t convert_file_from_to(FILE *file, const char *fromcodes, const char *tocodes,
                                   char **fromcode, char **tocode, struct Content *info)
{
  char *fcode = NULL;
  char **tcode = NULL;
  const char *c = NULL, *c1 = NULL;
  size_t ret;
  int ncodes, i, cn;

  /* Count the tocodes */
  ncodes = 0;
  for (c = tocodes; c; c = c1 ? c1 + 1 : 0)
  {
    c1 = strchr(c, ':');
    if (c1 == c)
      continue;
    ncodes++;
  }

  /* Copy them */
  tcode = safe_malloc(ncodes * sizeof(char *));
  for (c = tocodes, i = 0; c; c = c1 ? c1 + 1 : 0, i++)
  {
    c1 = strchr(c, ':');
    if (c1 == c)
      continue;
    tcode[i] = mutt_substrdup(c, c1);
  }

  ret = (size_t)(-1);
  if (fromcode)
  {
    /* Try each fromcode in turn */
    for (c = fromcodes; c; c = c1 ? c1 + 1 : 0)
    {
      c1 = strchr(c, ':');
      if (c1 == c)
        continue;
      fcode = mutt_substrdup(c, c1);

      ret = convert_file_to(file, fcode, ncodes, (const char **) tcode, &cn, info);
      if (ret != (size_t)(-1))
      {
        *fromcode = fcode;
        *tocode = tcode[cn];
        tcode[cn] = 0;
        break;
      }
      FREE(&fcode);
    }
  }
  else
  {
    /* There is only one fromcode */
    ret = convert_file_to(file, fromcodes, ncodes, (const char **) tcode, &cn, info);
    if (ret != (size_t)(-1))
    {
      *tocode = tcode[cn];
      tcode[cn] = 0;
    }
  }

  /* Free memory */
  for (i = 0; i < ncodes; i++)
    FREE(&tcode[i]);

  FREE(&tcode);

  return ret;
}

/**
 * mutt_get_content_info - Analyze file to determine MIME encoding to use
 *
 * Also set the body charset, sometimes, or not.
 */
struct Content *mutt_get_content_info(const char *fname, struct Body *b)
{
  struct Content *info = NULL;
  struct ContentState state;
  FILE *fp = NULL;
  char *fromcode = NULL;
  char *tocode = NULL;
  char buffer[100];
  char chsbuf[STRING];
  size_t r;

  struct stat sb;

  if (b && !fname)
    fname = b->filename;

  if (stat(fname, &sb) == -1)
  {
    mutt_error(_("Can't stat %s: %s"), fname, strerror(errno));
    return NULL;
  }

  if (!S_ISREG(sb.st_mode))
  {
    mutt_error(_("%s isn't a regular file."), fname);
    return NULL;
  }

  fp = fopen(fname, "r");
  if (!fp)
  {
    mutt_debug(1, "mutt_get_content_info: %s: %s (errno %d).\n", fname,
               strerror(errno), errno);
    return NULL;
  }

  info = safe_calloc(1, sizeof(struct Content));
  memset(&state, 0, sizeof(state));

  if (b != NULL && b->type == TYPETEXT && (!b->noconv && !b->force_charset))
  {
    char *chs = mutt_get_parameter("charset", b->parameter);
    char *fchs = b->use_disp ?
                     ((AttachCharset && *AttachCharset) ? AttachCharset : Charset) :
                     Charset;
    if (Charset && (chs || SendCharset) &&
        convert_file_from_to(fp, fchs, chs ? chs : SendCharset, &fromcode,
                             &tocode, info) != (size_t)(-1))
    {
      if (!chs)
      {
        mutt_canonical_charset(chsbuf, sizeof(chsbuf), tocode);
        mutt_set_parameter("charset", chsbuf, &b->parameter);
      }
      FREE(&b->charset);
      b->charset = fromcode;
      FREE(&tocode);
      safe_fclose(&fp);
      return info;
    }
  }

  rewind(fp);
  while ((r = fread(buffer, 1, sizeof(buffer), fp)))
    update_content_info(info, &state, buffer, r);
  update_content_info(info, &state, 0, 0);

  safe_fclose(&fp);

  if (b != NULL && b->type == TYPETEXT && (!b->noconv && !b->force_charset))
    mutt_set_parameter(
        "charset",
        (!info->hibin ? "us-ascii" : Charset && !mutt_is_us_ascii(Charset) ? Charset : "unknown-8bit"),
        &b->parameter);

  return info;
}

/**
 * mutt_lookup_mime_type - Find the MIME type for an attachment
 * @param att  Email with attachment
 * @param path Path to attachment
 * @retval n MIME type, e.g. #TYPEIMAGE
 *
 * Given a file at `path`, see if there is a registered MIME type.
 * Returns the major MIME type, and copies the subtype to ``d''.  First look
 * for ~/.mime.types, then look in a system mime.types if we can find one.
 * The longest match is used so that we can match `ps.gz' when `gz' also
 * exists.
 */
int mutt_lookup_mime_type(struct Body *att, const char *path)
{
  FILE *f = NULL;
  char *p = NULL, *q = NULL, *ct = NULL;
  char buf[LONG_STRING];
  char subtype[STRING], xtype[STRING];
  int szf, sze, cur_sze;
  int type;
  bool found_mimetypes = false;

  *subtype = '\0';
  *xtype = '\0';
  type = TYPEOTHER;
  cur_sze = 0;

  szf = mutt_strlen(path);

  for (int count = 0; count < 4; count++)
  {
    /*
     * can't use strtok() because we use it in an inner loop below, so use
     * a switch statement here instead.
     */
    switch (count)
    {
      /* last file with last entry to match wins type/xtype */
      case 0:
        /*
         * check default unix mimetypes location first
         */
        strfcpy(buf, "/etc/mime.types", sizeof(buf));
        break;
      case 1:
        strfcpy(buf, SYSCONFDIR "/mime.types", sizeof(buf));
        break;
      case 2:
        strfcpy(buf, PKGDATADIR "/mime.types", sizeof(buf));
        break;
      case 3:
        snprintf(buf, sizeof(buf), "%s/.mime.types", NONULL(HomeDir));
        break;
      default:
        mutt_debug(1, "mutt_lookup_mime_type: Internal error, count = %d.\n", count);
        goto bye; /* shouldn't happen */
    }

    f = fopen(buf, "r");
    if (f)
    {
      found_mimetypes = true;

      while (fgets(buf, sizeof(buf) - 1, f) != NULL)
      {
        /* weed out any comments */
        if ((p = strchr(buf, '#')))
          *p = 0;

        /* remove any leading space. */
        ct = buf;
        SKIPWS(ct);

        /* position on the next field in this line */
        p = strpbrk(ct, " \t");
        if (!p)
          continue;
        *p++ = 0;
        SKIPWS(p);

        /* cycle through the file extensions */
        while ((p = strtok(p, " \t\n")))
        {
          sze = mutt_strlen(p);
          if ((sze > cur_sze) && (szf >= sze) &&
              ((mutt_strcasecmp(path + szf - sze, p) == 0) ||
               (mutt_strcasecmp(path + szf - sze, p) == 0)) &&
              (szf == sze || path[szf - sze - 1] == '.'))
          {
            /* get the content-type */

            p = strchr(ct, '/');
            if (!p)
            {
              /* malformed line, just skip it. */
              break;
            }
            *p++ = 0;

            for (q = p; *q && !ISSPACE(*q); q++)
              ;

            mutt_substrcpy(subtype, p, q, sizeof(subtype));

            type = mutt_check_mime_type(ct);
            if (type == TYPEOTHER)
              strfcpy(xtype, ct, sizeof(xtype));

            cur_sze = sze;
          }
          p = NULL;
        }
      }
      safe_fclose(&f);
    }
  }

bye:

  /* no mime.types file found */
  if (!found_mimetypes)
  {
    mutt_error(_("Could not find any mime.types file."));
  }

  if (type != TYPEOTHER || *xtype != '\0')
  {
    att->type = type;
    mutt_str_replace(&att->subtype, subtype);
    mutt_str_replace(&att->xtype, xtype);
  }

  return type;
}

static void transform_to_7bit(struct Body *a, FILE *fpin)
{
  char buff[_POSIX_PATH_MAX];
  struct State s;
  struct stat sb;

  memset(&s, 0, sizeof(s));
  for (; a; a = a->next)
  {
    if (a->type == TYPEMULTIPART)
    {
      if (a->encoding != ENC7BIT)
        a->encoding = ENC7BIT;

      transform_to_7bit(a->parts, fpin);
    }
    else if (mutt_is_message_type(a->type, a->subtype))
    {
      mutt_message_to_7bit(a, fpin);
    }
    else
    {
      a->noconv = true;
      a->force_charset = true;

      mutt_mktemp(buff, sizeof(buff));
      s.fpout = safe_fopen(buff, "w");
      if (!s.fpout)
      {
        mutt_perror("fopen");
        return;
      }
      s.fpin = fpin;
      mutt_decode_attachment(a, &s);
      safe_fclose(&s.fpout);
      FREE(&a->d_filename);
      a->d_filename = a->filename;
      a->filename = safe_strdup(buff);
      a->unlink = true;
      if (stat(a->filename, &sb) == -1)
      {
        mutt_perror("stat");
        return;
      }
      a->length = sb.st_size;

      mutt_update_encoding(a);
      if (a->encoding == ENC8BIT)
        a->encoding = ENCQUOTEDPRINTABLE;
      else if (a->encoding == ENCBINARY)
        a->encoding = ENCBASE64;
    }
  }
}

void mutt_message_to_7bit(struct Body *a, FILE *fp)
{
  char temp[_POSIX_PATH_MAX];
  char *line = NULL;
  FILE *fpin = NULL;
  FILE *fpout = NULL;
  struct stat sb;

  if (!a->filename && fp)
    fpin = fp;
  else if (!a->filename || !(fpin = fopen(a->filename, "r")))
  {
    mutt_error(_("Could not open %s"), a->filename ? a->filename : "(null)");
    return;
  }
  else
  {
    a->offset = 0;
    if (stat(a->filename, &sb) == -1)
    {
      mutt_perror("stat");
      safe_fclose(&fpin);
    }
    a->length = sb.st_size;
  }

  mutt_mktemp(temp, sizeof(temp));
  fpout = safe_fopen(temp, "w+");
  if (!fpout)
  {
    mutt_perror("fopen");
    goto cleanup;
  }

  if (!fpin)
    goto cleanup;

  fseeko(fpin, a->offset, SEEK_SET);
  a->parts = mutt_parse_message_rfc822(fpin, a);

  transform_to_7bit(a->parts, fpin);

  mutt_copy_hdr(fpin, fpout, a->offset, a->offset + a->length,
                CH_MIME | CH_NONEWLINE | CH_XMIT, NULL);

  fputs("MIME-Version: 1.0\n", fpout);
  mutt_write_mime_header(a->parts, fpout);
  fputc('\n', fpout);
  mutt_write_mime_body(a->parts, fpout);

cleanup:
  FREE(&line);

  if (fpin && fpin != fp)
    safe_fclose(&fpin);
  if (fpout)
    safe_fclose(&fpout);
  else
    return;

  a->encoding = ENC7BIT;
  FREE(&a->d_filename);
  a->d_filename = a->filename;
  if (a->filename && a->unlink)
    unlink(a->filename);
  a->filename = safe_strdup(temp);
  a->unlink = true;
  if (stat(a->filename, &sb) == -1)
  {
    mutt_perror("stat");
    return;
  }
  a->length = sb.st_size;
  mutt_free_body(&a->parts);
  a->hdr->content = NULL;
}

/**
 * set_encoding - determine which Content-Transfer-Encoding to use
 */
static void set_encoding(struct Body *b, struct Content *info)
{
  char send_charset[SHORT_STRING];

  if (b->type == TYPETEXT)
  {
    char *chsname = mutt_get_body_charset(send_charset, sizeof(send_charset), b);
    if ((info->lobin && (mutt_strncasecmp(chsname, "iso-2022", 8) != 0)) ||
        info->linemax > 990 || (info->from && option(OPT_ENCODE_FROM)))
      b->encoding = ENCQUOTEDPRINTABLE;
    else if (info->hibin)
      b->encoding = option(OPT_ALLOW_8BIT) ? ENC8BIT : ENCQUOTEDPRINTABLE;
    else
      b->encoding = ENC7BIT;
  }
  else if (b->type == TYPEMESSAGE || b->type == TYPEMULTIPART)
  {
    if (info->lobin || info->hibin)
    {
      if (option(OPT_ALLOW_8BIT) && !info->lobin)
        b->encoding = ENC8BIT;
      else
        mutt_message_to_7bit(b, NULL);
    }
    else
      b->encoding = ENC7BIT;
  }
  else if (b->type == TYPEAPPLICATION &&
           (mutt_strcasecmp(b->subtype, "pgp-keys") == 0))
    b->encoding = ENC7BIT;
  else
  {
    /* Determine which encoding is smaller  */
    if (1.33 * (float) (info->lobin + info->hibin + info->ascii) <
        3.0 * (float) (info->lobin + info->hibin) + (float) info->ascii)
      b->encoding = ENCBASE64;
    else
      b->encoding = ENCQUOTEDPRINTABLE;
  }
}

void mutt_stamp_attachment(struct Body *a)
{
  a->stamp = time(NULL);
}

/**
 * mutt_get_body_charset - Get a body's character set
 */
char *mutt_get_body_charset(char *d, size_t dlen, struct Body *b)
{
  char *p = NULL;

  if (b && b->type != TYPETEXT)
    return NULL;

  if (b)
    p = mutt_get_parameter("charset", b->parameter);

  if (p)
    mutt_canonical_charset(d, dlen, NONULL(p));
  else
    strfcpy(d, "us-ascii", dlen);

  return d;
}

/**
 * mutt_update_encoding - Update the encoding type
 *
 * Assumes called from send mode where Body->filename points to actual file
 */
void mutt_update_encoding(struct Body *a)
{
  struct Content *info = NULL;
  char chsbuff[STRING];

  /* override noconv when it's us-ascii */
  if (mutt_is_us_ascii(mutt_get_body_charset(chsbuff, sizeof(chsbuff), a)))
    a->noconv = false;

  if (!a->force_charset && !a->noconv)
    mutt_delete_parameter("charset", &a->parameter);

  info = mutt_get_content_info(a->filename, a);
  if (!info)
    return;

  set_encoding(a, info);
  mutt_stamp_attachment(a);

  FREE(&a->content);
  a->content = info;
}

struct Body *mutt_make_message_attach(struct Context *ctx, struct Header *hdr, int attach_msg)
{
  char buffer[LONG_STRING];
  struct Body *body = NULL;
  FILE *fp = NULL;
  int cmflags, chflags;
  int pgp = WithCrypto ? hdr->security : 0;

  if (WithCrypto)
  {
    if ((option(OPT_MIME_FORWARD_DECODE) || option(OPT_FORWARD_DECRYPT)) &&
        (hdr->security & ENCRYPT))
    {
      if (!crypt_valid_passphrase(hdr->security))
        return NULL;
    }
  }

  mutt_mktemp(buffer, sizeof(buffer));
  fp = safe_fopen(buffer, "w+");
  if (!fp)
    return NULL;

  body = mutt_new_body();
  body->type = TYPEMESSAGE;
  body->subtype = safe_strdup("rfc822");
  body->filename = safe_strdup(buffer);
  body->unlink = true;
  body->use_disp = false;
  body->disposition = DISPINLINE;
  body->noconv = true;

  mutt_parse_mime_message(ctx, hdr);

  chflags = CH_XMIT;
  cmflags = 0;

  /* If we are attaching a message, ignore OPT_MIME_FORWARD_DECODE */
  if (!attach_msg && option(OPT_MIME_FORWARD_DECODE))
  {
    chflags |= CH_MIME | CH_TXTPLAIN;
    cmflags = MUTT_CM_DECODE | MUTT_CM_CHARCONV;
    if ((WithCrypto & APPLICATION_PGP))
      pgp &= ~PGPENCRYPT;
    if ((WithCrypto & APPLICATION_SMIME))
      pgp &= ~SMIMEENCRYPT;
  }
  else if (WithCrypto && option(OPT_FORWARD_DECRYPT) && (hdr->security & ENCRYPT))
  {
    if ((WithCrypto & APPLICATION_PGP) && mutt_is_multipart_encrypted(hdr->content))
    {
      chflags |= CH_MIME | CH_NONEWLINE;
      cmflags = MUTT_CM_DECODE_PGP;
      pgp &= ~PGPENCRYPT;
    }
    else if ((WithCrypto & APPLICATION_PGP) &&
             (mutt_is_application_pgp(hdr->content) & PGPENCRYPT))
    {
      chflags |= CH_MIME | CH_TXTPLAIN;
      cmflags = MUTT_CM_DECODE | MUTT_CM_CHARCONV;
      pgp &= ~PGPENCRYPT;
    }
    else if ((WithCrypto & APPLICATION_SMIME) &&
             mutt_is_application_smime(hdr->content) & SMIMEENCRYPT)
    {
      chflags |= CH_MIME | CH_TXTPLAIN;
      cmflags = MUTT_CM_DECODE | MUTT_CM_CHARCONV;
      pgp &= ~SMIMEENCRYPT;
    }
  }

  mutt_copy_message(fp, ctx, hdr, cmflags, chflags);

  fflush(fp);
  rewind(fp);

  body->hdr = mutt_new_header();
  body->hdr->offset = 0;
  /* we don't need the user headers here */
  body->hdr->env = mutt_read_rfc822_header(fp, body->hdr, 0, 0);
  if (WithCrypto)
    body->hdr->security = pgp;
  mutt_update_encoding(body);
  body->parts = body->hdr->content;

  safe_fclose(&fp);

  return body;
}

static void run_mime_type_query(struct Body *att)
{
  FILE *fp, *fperr;
  char cmd[HUGE_STRING];
  char *buf = NULL;
  size_t buflen;
  int dummy = 0;
  pid_t thepid;

  mutt_expand_file_fmt(cmd, sizeof(cmd), MimeTypeQueryCommand, att->filename);

  if ((thepid = mutt_create_filter(cmd, NULL, &fp, &fperr)) < 0)
  {
    mutt_error(_("Error running \"%s\"!"), cmd);
    return;
  }

  buf = mutt_read_line(buf, &buflen, fp, &dummy, 0);
  if (buf)
  {
    if (strchr(buf, '/'))
      mutt_parse_content_type(buf, att);
    FREE(&buf);
  }

  safe_fclose(&fp);
  safe_fclose(&fperr);
  mutt_wait_filter(thepid);
}

struct Body *mutt_make_file_attach(const char *path)
{
  struct Body *att = NULL;
  struct Content *info = NULL;

  att = mutt_new_body();
  att->filename = safe_strdup(path);

  if (MimeTypeQueryCommand && *MimeTypeQueryCommand && option(OPT_MIME_TYPE_QUERY_FIRST))
    run_mime_type_query(att);

  /* Attempt to determine the appropriate content-type based on the filename
   * suffix.
   */
  if (!att->subtype)
    mutt_lookup_mime_type(att, path);

  if (!att->subtype && MimeTypeQueryCommand && *MimeTypeQueryCommand &&
      !option(OPT_MIME_TYPE_QUERY_FIRST))
    run_mime_type_query(att);

  info = mutt_get_content_info(path, att);
  if (!info)
  {
    mutt_free_body(&att);
    return NULL;
  }

  if (!att->subtype)
  {
    if ((info->nulbin == 0) &&
        (info->lobin == 0 || (info->lobin + info->hibin + info->ascii) / info->lobin >= 10))
    {
      /*
       * Statistically speaking, there should be more than 10% "lobin"
       * chars if this is really a binary file...
       */
      att->type = TYPETEXT;
      att->subtype = safe_strdup("plain");
    }
    else
    {
      att->type = TYPEAPPLICATION;
      att->subtype = safe_strdup("octet-stream");
    }
  }

  FREE(&info);
  mutt_update_encoding(att);
  return att;
}

static int get_toplevel_encoding(struct Body *a)
{
  int e = ENC7BIT;

  for (; a; a = a->next)
  {
    if (a->encoding == ENCBINARY)
      return ENCBINARY;
    else if (a->encoding == ENC8BIT)
      e = ENC8BIT;
  }

  return e;
}

/**
 * check_boundary - check for duplicate boundary
 * @retval true if duplicate found
 */
static bool check_boundary(const char *boundary, struct Body *b)
{
  char *p = NULL;

  if (b->parts && check_boundary(boundary, b->parts))
    return true;

  if (b->next && check_boundary(boundary, b->next))
    return true;

  if ((p = mutt_get_parameter("boundary", b->parameter)) && (mutt_strcmp(p, boundary) == 0))
    return true;
  return false;
}

struct Body *mutt_make_multipart(struct Body *b)
{
  struct Body *new = NULL;

  new = mutt_new_body();
  new->type = TYPEMULTIPART;
  new->subtype = safe_strdup("mixed");
  new->encoding = get_toplevel_encoding(b);
  do
  {
    mutt_generate_boundary(&new->parameter);
    if (check_boundary(mutt_get_parameter("boundary", new->parameter), b))
      mutt_delete_parameter("boundary", &new->parameter);
  } while (!mutt_get_parameter("boundary", new->parameter));
  new->use_disp = false;
  new->disposition = DISPINLINE;
  new->parts = b;

  return new;
}

/**
 * mutt_remove_multipart - remove the multipart body if it exists
 */
struct Body *mutt_remove_multipart(struct Body *b)
{
  struct Body *t = NULL;

  if (b->parts)
  {
    t = b;
    b = b->parts;
    t->parts = NULL;
    mutt_free_body(&t);
  }
  return b;
}

/**
 * mutt_write_address_list - wrapper around mutt_write_address()
 *
 * So we can handle very large recipient lists without needing a huge temporary
 * buffer in memory
 */
void mutt_write_address_list(struct Address *adr, FILE *fp, int linelen, int display)
{
  struct Address *tmp = NULL;
  char buf[LONG_STRING];
  int count = 0;
  int len;

  while (adr)
  {
    tmp = adr->next;
    adr->next = NULL;
    buf[0] = 0;
    rfc822_write_address(buf, sizeof(buf), adr, display);
    len = mutt_strlen(buf);
    if (count && linelen + len > 74)
    {
      fputs("\n\t", fp);
      linelen = len + 8; /* tab is usually about 8 spaces... */
    }
    else
    {
      if (count && adr->mailbox)
      {
        fputc(' ', fp);
        linelen++;
      }
      linelen += len;
    }
    fputs(buf, fp);
    adr->next = tmp;
    if (!adr->group && adr->next && adr->next->mailbox)
    {
      linelen++;
      fputc(',', fp);
    }
    adr = adr->next;
    count++;
  }
  fputc('\n', fp);
}

/* arbitrary number of elements to grow the array by */
#define REF_INC 16

/**
 * mutt_write_references - Add the message refrerences to a list
 *
 * need to write the list in reverse because they are stored in reverse order
 * when parsed to speed up threading
 */
void mutt_write_references(const struct ListHead *r, FILE *f, size_t trim)
{
  struct ListNode *np;
  size_t length = 0;

  STAILQ_FOREACH(np, r, entries)
  {
    if (++length == trim)
      break;
  }

  struct ListNode **ref = safe_calloc(length, sizeof(struct ListNode *));

  // store in reverse order
  size_t tmp = length;
  STAILQ_FOREACH(np, r, entries)
  {
    ref[--tmp] = np;
    if (tmp == 0)
      break;
  }

  for (size_t i = 0; i < length; ++i)
  {
    if (i != 0)
      fputc(' ', f);
    fputs(ref[i]->data, f);
    if (i != length - 1)
      fputc('\n', f);
  }

  FREE(&ref);
}

static const char *find_word(const char *src)
{
  const char *p = src;

  while (p && *p && strchr(" \t\n", *p))
    p++;
  while (p && *p && !strchr(" \t\n", *p))
    p++;
  return p;
}

/**
 * my_width - like wcwidth(), but gets const char* not wchar_t*
 */
static int my_width(const char *str, int col, int flags)
{
  wchar_t wc;
  int l, w = 0, nl = 0;
  const char *p = str;

  while (p && *p)
  {
    if (mbtowc(&wc, p, MB_CUR_MAX) >= 0)
    {
      l = wcwidth(wc);
      if (l < 0)
        l = 1;
      /* correctly calc tab stop, even for sending as the
       * line should look pretty on the receiving end */
      if (wc == L'\t' || (nl && wc == L' '))
      {
        nl = 0;
        l = 8 - (col % 8);
      }
      /* track newlines for display-case: if we have a space
       * after a newline, assume 8 spaces as for display we
       * always tab-fold */
      else if ((flags & CH_DISPLAY) && wc == '\n')
        nl = 1;
    }
    else
      l = 1;
    w += l;
    p++;
  }
  return w;
}

static int print_val(FILE *fp, const char *pfx, const char *value, int flags, size_t col)
{
  while (value && *value)
  {
    if (fputc(*value, fp) == EOF)
      return -1;
    /* corner-case: break words longer than 998 chars by force,
     * mandated by RfC5322 */
    if (!(flags & CH_DISPLAY) && ++col >= 998)
    {
      if (fputs("\n ", fp) < 0)
        return -1;
      col = 1;
    }
    if (*value == '\n')
    {
      if (*(value + 1) && pfx && *pfx && fputs(pfx, fp) == EOF)
        return -1;
      /* for display, turn folding spaces into folding tabs */
      if ((flags & CH_DISPLAY) && (*(value + 1) == ' ' || *(value + 1) == '\t'))
      {
        value++;
        while (*value && (*value == ' ' || *value == '\t'))
          value++;
        if (fputc('\t', fp) == EOF)
          return -1;
        continue;
      }
    }
    value++;
  }
  return 0;
}

static int fold_one_header(FILE *fp, const char *tag, const char *value,
                           const char *pfx, int wraplen, int flags)
{
  const char *p = value, *next = NULL, *sp = NULL;
  char buf[HUGE_STRING] = "";
  int first = 1, enc, col = 0, w, l = 0, fold;

  mutt_debug(4, "mwoh: pfx=[%s], tag=[%s], flags=%d value=[%s]\n", pfx, tag,
             flags, NONULL(value));

  if (tag && *tag && fprintf(fp, "%s%s: ", NONULL(pfx), tag) < 0)
    return -1;
  col = mutt_strlen(tag) + (tag && *tag ? 2 : 0) + mutt_strlen(pfx);

  while (p && *p)
  {
    fold = 0;

    /* find the next word and place it in `buf'. it may start with
     * whitespace we can fold before */
    next = find_word(p);
    l = MIN(sizeof(buf) - 1, next - p);
    memcpy(buf, p, l);
    buf[l] = 0;

    /* determine width: character cells for display, bytes for sending
     * (we get pure ascii only) */
    w = my_width(buf, col, flags);
    enc = (mutt_strncmp(buf, "=?", 2) == 0);

    mutt_debug(5, "mwoh: word=[%s], col=%d, w=%d, next=[0x0%x]\n", buf, col, w, *next);

    /* insert a folding \n before the current word's lwsp except for
     * header name, first word on a line (word longer than wrap width)
     * and encoded words */
    if (!first && !enc && col && col + w >= wraplen)
    {
      col = mutt_strlen(pfx);
      fold = 1;
      if (fprintf(fp, "\n%s", NONULL(pfx)) <= 0)
        return -1;
    }

    /* print the actual word; for display, ignore leading ws for word
     * and fold with tab for readability */
    if ((flags & CH_DISPLAY) && fold)
    {
      char *pc = buf;
      while (*pc && (*pc == ' ' || *pc == '\t'))
      {
        pc++;
        col--;
      }
      if (fputc('\t', fp) == EOF)
        return -1;
      if (print_val(fp, pfx, pc, flags, col) < 0)
        return -1;
      col += 8;
    }
    else if (print_val(fp, pfx, buf, flags, col) < 0)
      return -1;
    col += w;

    /* if the current word ends in \n, ignore all its trailing spaces
     * and reset column; this prevents us from putting only spaces (or
     * even none) on a line if the trailing spaces are located at our
     * current line width
     * XXX this covers ASCII space only, for display we probably
     * XXX want something like iswspace() here */
    sp = next;
    while (*sp && (*sp == ' ' || *sp == '\t'))
      sp++;
    if (*sp == '\n')
    {
      next = sp;
      col = 0;
    }

    p = next;
    first = 0;
  }

  /* if we have printed something but didn't \n-terminate it, do it
   * except the last word we printed ended in \n already */
  if (col && (l == 0 || buf[l - 1] != '\n'))
    if (putc('\n', fp) == EOF)
      return -1;

  return 0;
}

static char *unfold_header(char *s)
{
  char *p = s, *q = s;

  while (p && *p)
  {
    /* remove CRLF prior to FWSP, turn \t into ' ' */
    if (*p == '\r' && *(p + 1) && *(p + 1) == '\n' && *(p + 2) &&
        (*(p + 2) == ' ' || *(p + 2) == '\t'))
    {
      *q++ = ' ';
      p += 3;
      continue;
    }
    /* remove LF prior to FWSP, turn \t into ' ' */
    else if (*p == '\n' && *(p + 1) && (*(p + 1) == ' ' || *(p + 1) == '\t'))
    {
      *q++ = ' ';
      p += 2;
      continue;
    }
    *q++ = *p++;
  }
  if (q)
    *q = 0;

  return s;
}

static int write_one_header(FILE *fp, int pfxw, int max, int wraplen, const char *pfx,
                            const char *start, const char *end, int flags)
{
  char *tagbuf = NULL, *valbuf = NULL, *t = NULL;
  int is_from = ((end - start) > 5 && (mutt_strncasecmp(start, "from ", 5) == 0));

  /* only pass through folding machinery if necessary for sending,
     never wrap From_ headers on sending */
  if (!(flags & CH_DISPLAY) && (pfxw + max <= wraplen || is_from))
  {
    valbuf = mutt_substrdup(start, end);
    mutt_debug(4, "mwoh: buf[%s%s] short enough, "
                  "max width = %d <= %d\n",
               NONULL(pfx), valbuf, max, wraplen);
    if (pfx && *pfx)
    {
      if (fputs(pfx, fp) == EOF)
      {
        FREE(&valbuf);
        return -1;
      }
    }

    t = strchr(valbuf, ':');
    if (!t)
    {
      mutt_debug(1, "mwoh: warning: header not in 'key: value' format!\n");
      FREE(&valbuf);
      return 0;
    }
    if (print_val(fp, pfx, valbuf, flags, mutt_strlen(pfx)) < 0)
    {
      FREE(&valbuf);
      return -1;
    }
    FREE(&valbuf);
  }
  else
  {
    t = strchr(start, ':');
    if (!t || t > end)
    {
      mutt_debug(1, "mwoh: warning: header not in 'key: value' format!\n");
      return 0;
    }
    if (is_from)
    {
      tagbuf = NULL;
      valbuf = mutt_substrdup(start, end);
    }
    else
    {
      tagbuf = mutt_substrdup(start, t);
      /* skip over the colon separating the header field name and value */
      t++;

      /* skip over any leading whitespace (WSP, as defined in RFC5322)
       * NOTE: skip_email_wsp() does the wrong thing here.
       *       See tickets 3609 and 3716. */
      while (*t == ' ' || *t == '\t')
        t++;

      valbuf = mutt_substrdup(t, end);
    }
    mutt_debug(4, "mwoh: buf[%s%s] too long, max width = %d > %d\n",
               NONULL(pfx), NONULL(valbuf), max, wraplen);
    if (fold_one_header(fp, tagbuf, valbuf, pfx, wraplen, flags) < 0)
    {
      FREE(&valbuf);
      FREE(&tagbuf);
      return -1;
    }
    FREE(&tagbuf);
    FREE(&valbuf);
  }
  return 0;
}

/**
 * mutt_write_one_header - Write one header line to a file
 *
 * split several headers into individual ones and call write_one_header
 * for each one
 */
int mutt_write_one_header(FILE *fp, const char *tag, const char *value,
                          const char *pfx, int wraplen, int flags)
{
  char *p = (char *) value, *last = NULL, *line = NULL;
  int max = 0, w, rc = -1;
  int pfxw = mutt_strwidth(pfx);
  char *v = safe_strdup(value);

  if (!(flags & CH_DISPLAY) || option(OPT_WEED))
    v = unfold_header(v);

  /* when not displaying, use sane wrap value */
  if (!(flags & CH_DISPLAY))
  {
    if (WrapHeaders < 78 || WrapHeaders > 998)
      wraplen = 78;
    else
      wraplen = WrapHeaders;
  }
  else if (wraplen <= 0 || wraplen > MuttIndexWindow->cols)
    wraplen = MuttIndexWindow->cols;

  if (tag)
  {
    /* if header is short enough, simply print it */
    if (!(flags & CH_DISPLAY) && mutt_strwidth(tag) + 2 + pfxw + mutt_strwidth(v) <= wraplen)
    {
      mutt_debug(4, "mwoh: buf[%s%s: %s] is short enough\n", NONULL(pfx), tag, v);
      if (fprintf(fp, "%s%s: %s\n", NONULL(pfx), tag, v) <= 0)
        goto out;
      rc = 0;
      goto out;
    }
    else
    {
      rc = fold_one_header(fp, tag, v, pfx, wraplen, flags);
      goto out;
    }
  }

  p = last = line = (char *) v;
  while (p && *p)
  {
    p = strchr(p, '\n');

    /* find maximum line width in current header */
    if (p)
      *p = 0;
    if ((w = my_width(line, 0, flags)) > max)
      max = w;
    if (p)
      *p = '\n';

    if (!p)
      break;

    line = ++p;
    if (*p != ' ' && *p != '\t')
    {
      if (write_one_header(fp, pfxw, max, wraplen, pfx, last, p, flags) < 0)
        goto out;
      last = p;
      max = 0;
    }
  }

  if (last && *last)
    if (write_one_header(fp, pfxw, max, wraplen, pfx, last, p, flags) < 0)
      goto out;

  rc = 0;

out:
  FREE(&v);
  return rc;
}

/* Note: all RFC2047 encoding should be done outside of this routine, except
 * for the "real name."  This will allow this routine to be used more than
 * once, if necessary.
 *
 * Likewise, all IDN processing should happen outside of this routine.
 *
 * mode == 1  => "lite" mode (used for edit_headers)
 * mode == 0  => normal mode.  write full header + MIME headers
 * mode == -1 => write just the envelope info (used for postponing messages)
 *
 * privacy != 0 => will omit any headers which may identify the user.
 *               Output generated is suitable for being sent through
 *               anonymous remailer chains.
 */

int mutt_write_rfc822_header(FILE *fp, struct Envelope *env, struct Body *attach, int mode,
                             int privacy, int should_write_bcc)
{
  char buffer[LONG_STRING];
  char *p = NULL, *q = NULL;
  bool has_agent = false; /* user defined user-agent header field exists */

  if (mode == 0 && !privacy)
    fputs(mutt_make_date(buffer, sizeof(buffer)), fp);

  /* OPT_USE_FROM is not consulted here so that we can still write a From:
   * field if the user sets it with the `my_hdr' command
   */
  if (env->from && !privacy)
  {
    buffer[0] = 0;
    rfc822_write_address(buffer, sizeof(buffer), env->from, 0);
    fprintf(fp, "From: %s\n", buffer);
  }

  if (env->sender && !privacy)
  {
    buffer[0] = 0;
    rfc822_write_address(buffer, sizeof(buffer), env->sender, 0);
    fprintf(fp, "Sender: %s\n", buffer);
  }

  if (env->to)
  {
    fputs("To: ", fp);
    mutt_write_address_list(env->to, fp, 4, 0);
  }
  else if (mode > 0)
#ifdef USE_NNTP
    if (!option(OPT_NEWS_SEND))
#endif
      fputs("To: \n", fp);

  if (env->cc)
  {
    fputs("Cc: ", fp);
    mutt_write_address_list(env->cc, fp, 4, 0);
  }
  else if (mode > 0)
#ifdef USE_NNTP
    if (!option(OPT_NEWS_SEND))
#endif
      fputs("Cc: \n", fp);

  if (env->bcc && should_write_bcc)
  {
    if (mode != 0 || option(OPT_WRITE_BCC))
    {
      fputs("Bcc: ", fp);
      mutt_write_address_list(env->bcc, fp, 5, 0);
    }
  }
  else if (mode > 0)
#ifdef USE_NNTP
    if (!option(OPT_NEWS_SEND))
#endif
      fputs("Bcc: \n", fp);

#ifdef USE_NNTP
  if (env->newsgroups)
    fprintf(fp, "Newsgroups: %s\n", env->newsgroups);
  else if (mode == 1 && option(OPT_NEWS_SEND))
    fputs("Newsgroups: \n", fp);

  if (env->followup_to)
    fprintf(fp, "Followup-To: %s\n", env->followup_to);
  else if (mode == 1 && option(OPT_NEWS_SEND))
    fputs("Followup-To: \n", fp);

  if (env->x_comment_to)
    fprintf(fp, "X-Comment-To: %s\n", env->x_comment_to);
  else if (mode == 1 && option(OPT_NEWS_SEND) && option(OPT_X_COMMENT_TO))
    fputs("X-Comment-To: \n", fp);
#endif

  if (env->subject)
    mutt_write_one_header(fp, "Subject", env->subject, NULL, 0, 0);
  else if (mode == 1)
    fputs("Subject: \n", fp);

  /* save message id if the user has set it */
  if (env->message_id && !privacy)
    fprintf(fp, "Message-ID: %s\n", env->message_id);

  if (env->reply_to)
  {
    fputs("Reply-To: ", fp);
    mutt_write_address_list(env->reply_to, fp, 10, 0);
  }
  else if (mode > 0)
    fputs("Reply-To: \n", fp);

  if (env->mail_followup_to)
#ifdef USE_NNTP
    if (!option(OPT_NEWS_SEND))
#endif
    {
      fputs("Mail-Followup-To: ", fp);
      mutt_write_address_list(env->mail_followup_to, fp, 18, 0);
    }

  if (mode <= 0)
  {
    if (!STAILQ_EMPTY(&env->references))
    {
      fputs("References:", fp);
      mutt_write_references(&env->references, fp, 10);
      fputc('\n', fp);
    }

    /* Add the MIME headers */
    fputs("MIME-Version: 1.0\n", fp);
    mutt_write_mime_header(attach, fp);
  }

  if (!STAILQ_EMPTY(&env->in_reply_to))
  {
    fputs("In-Reply-To:", fp);
    mutt_write_references(&env->in_reply_to, fp, 0);
    fputc('\n', fp);
  }

  /* Add any user defined headers */
  struct ListNode *tmp;
  STAILQ_FOREACH(tmp, &env->userhdrs, entries)
  {
    if ((p = strchr(tmp->data, ':')))
    {
      q = p;

      *p = '\0';

      p = skip_email_wsp(p + 1);
      if (!*p)
      {
        *q = ':';
        continue; /* don't emit empty fields. */
      }

      /* check to see if the user has overridden the user-agent field */
      if (mutt_strncasecmp("user-agent", tmp->data, 10) == 0)
      {
        has_agent = true;
        if (privacy)
        {
          *q = ':';
          continue;
        }
      }

      mutt_write_one_header(fp, tmp->data, p, NULL, 0, 0);
      *q = ':';
    }
  }

  if (mode == 0 && !privacy && option(OPT_USER_AGENT) && !has_agent)
  {
    /* Add a vanity header */
    fprintf(fp, "User-Agent: NeoMutt/%s%s\n", PACKAGE_VERSION, GitVer);
  }

  return (ferror(fp) == 0 ? 0 : -1);
}

static void encode_headers(struct ListHead *h)
{
  char *tmp = NULL;
  char *p = NULL;
  int i;

  struct ListNode *np;
  STAILQ_FOREACH(np, h, entries)
  {
    p = strchr(np->data, ':');
    if (!p)
      continue;

    i = p - np->data;
    p = skip_email_wsp(p + 1);
    tmp = safe_strdup(p);

    if (!tmp)
      continue;

    rfc2047_encode_string(&tmp);
    safe_realloc(&np->data, mutt_strlen(np->data) + 2 + mutt_strlen(tmp) + 1);

    sprintf(np->data + i, ": %s", NONULL(tmp));

    FREE(&tmp);
  }
}

const char *mutt_fqdn(short may_hide_host)
{
  char *p = NULL;

  if (Hostname && Hostname[0] != '@')
  {
    p = Hostname;

    if (may_hide_host && option(OPT_HIDDEN_HOST))
    {
      if ((p = strchr(Hostname, '.')))
        p++;

      /* sanity check: don't hide the host if
       * the fqdn is something like detebe.org.
       */

      if (!p || !strchr(p, '.'))
        p = Hostname;
    }
  }

  return p;
}

static char *gen_msgid(void)
{
  char buf[SHORT_STRING];
  time_t now;
  struct tm *tm = NULL;
  const char *fqdn = NULL;
  unsigned char rndid[MUTT_RANDTAG_LEN + 1];

  mutt_rand_base32(rndid, sizeof(rndid) - 1);
  rndid[MUTT_RANDTAG_LEN] = 0;
  now = time(NULL);
  tm = gmtime(&now);
  fqdn = mutt_fqdn(0);
  if (!fqdn)
    fqdn = NONULL(ShortHostname);

  snprintf(buf, sizeof(buf), "<%d%02d%02d%02d%02d%02d.%s@%s>", tm->tm_year + 1900,
           tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, rndid, fqdn);
  return (safe_strdup(buf));
}

static void alarm_handler(int sig)
{
  SigAlrm = 1;
}

/**
 * send_msg - invoke sendmail in a subshell
 * @param[in]  path     Path to program to execute
 * @param[in]  args     Arguments to pass to program
 * @param[in]  msg      Temp file containing message to send
 * @param[out] tempfile If sendmail is put in the background, this points
 *                      to the temporary file containing the stdout of the
 *                      child process. If it is NULL, stderr and stdout
 *                      are not redirected.
 */
static int send_msg(const char *path, char **args, const char *msg, char **tempfile)
{
  sigset_t set;
  int fd, st;
  pid_t pid, ppid;

  mutt_block_signals_system();

  sigemptyset(&set);
  /* we also don't want to be stopped right now */
  sigaddset(&set, SIGTSTP);
  sigprocmask(SIG_BLOCK, &set, NULL);

  if (SendmailWait >= 0 && tempfile)
  {
    char tmp[_POSIX_PATH_MAX];

    mutt_mktemp(tmp, sizeof(tmp));
    *tempfile = safe_strdup(tmp);
  }

  pid = fork();
  if (pid == 0)
  {
    struct sigaction act, oldalrm;

    /* save parent's ID before setsid() */
    ppid = getppid();

    /* we want the delivery to continue even after the main process dies,
     * so we put ourselves into another session right away
     */
    setsid();

    /* next we close all open files */
    close(0);
#ifdef OPEN_MAX
    for (fd = tempfile ? 1 : 3; fd < OPEN_MAX; fd++)
      close(fd);
#elif defined(_POSIX_OPEN_MAX)
    for (fd = tempfile ? 1 : 3; fd < _POSIX_OPEN_MAX; fd++)
      close(fd);
#else
    if (tempfile)
    {
      close(1);
      close(2);
    }
#endif

    /* now the second fork() */
    pid = fork();
    if (pid == 0)
    {
      /* "msg" will be opened as stdin */
      if (open(msg, O_RDONLY, 0) < 0)
      {
        unlink(msg);
        _exit(S_ERR);
      }
      unlink(msg);

      if (SendmailWait >= 0 && tempfile && *tempfile)
      {
        /* *tempfile will be opened as stdout */
        if (open(*tempfile, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, 0600) < 0)
          _exit(S_ERR);
        /* redirect stderr to *tempfile too */
        if (dup(1) < 0)
          _exit(S_ERR);
      }
      else if (tempfile)
      {
        if (open("/dev/null", O_WRONLY | O_APPEND) < 0) /* stdout */
          _exit(S_ERR);
        if (open("/dev/null", O_RDWR | O_APPEND) < 0) /* stderr */
          _exit(S_ERR);
      }

      /* execvpe is a glibc extension */
      /* execvpe (path, args, mutt_envlist ()); */
      execvp(path, args);
      _exit(S_ERR);
    }
    else if (pid == -1)
    {
      unlink(msg);
      if (tempfile)
        FREE(tempfile);
      _exit(S_ERR);
    }

    /* SendmailWait > 0: interrupt waitpid() after SendmailWait seconds
     * SendmailWait = 0: wait forever
     * SendmailWait < 0: don't wait
     */
    if (SendmailWait > 0)
    {
      SigAlrm = 0;
      act.sa_handler = alarm_handler;
#ifdef SA_INTERRUPT
      /* need to make sure waitpid() is interrupted on SIGALRM */
      act.sa_flags = SA_INTERRUPT;
#else
      act.sa_flags = 0;
#endif
      sigemptyset(&act.sa_mask);
      sigaction(SIGALRM, &act, &oldalrm);
      alarm(SendmailWait);
    }
    else if (SendmailWait < 0)
      _exit(0xff & EX_OK);

    if (waitpid(pid, &st, 0) > 0)
    {
      st = WIFEXITED(st) ? WEXITSTATUS(st) : S_ERR;
      if (SendmailWait && st == (0xff & EX_OK) && tempfile && *tempfile)
      {
        unlink(*tempfile); /* no longer needed */
        FREE(tempfile);
      }
    }
    else
    {
      st = (SendmailWait > 0 && errno == EINTR && SigAlrm) ? S_BKG : S_ERR;
      if (SendmailWait > 0 && tempfile && *tempfile)
      {
        unlink(*tempfile);
        FREE(tempfile);
      }
    }

    if (SendmailWait > 0)
    {
      /* reset alarm; not really needed, but... */
      alarm(0);
      sigaction(SIGALRM, &oldalrm, NULL);
    }

    if (kill(ppid, 0) == -1 && errno == ESRCH && tempfile && *tempfile)
    {
      /* the parent is already dead */
      unlink(*tempfile);
      FREE(tempfile);
    }

    _exit(st);
  }

  sigprocmask(SIG_UNBLOCK, &set, NULL);

  if (pid != -1 && waitpid(pid, &st, 0) > 0)
    st = WIFEXITED(st) ? WEXITSTATUS(st) : S_ERR; /* return child status */
  else
    st = S_ERR; /* error */

  mutt_unblock_signals_system(1);

  return st;
}

static char **add_args(char **args, size_t *argslen, size_t *argsmax, struct Address *addr)
{
  for (; addr; addr = addr->next)
  {
    /* weed out group mailboxes, since those are for display only */
    if (addr->mailbox && !addr->group)
    {
      if (*argslen == *argsmax)
        safe_realloc(&args, (*argsmax += 5) * sizeof(char *));
      args[(*argslen)++] = addr->mailbox;
    }
  }
  return args;
}

static char **add_option(char **args, size_t *argslen, size_t *argsmax, char *s)
{
  if (*argslen == *argsmax)
    safe_realloc(&args, (*argsmax += 5) * sizeof(char *));
  args[(*argslen)++] = s;
  return args;
}

/**
 * mutt_invoke_sendmail - Run sendmail
 * @param from     The sender
 * @param to       Recipients
 * @param cc       Recipients
 * @param bcc      Recipients
 * @param msg      File containing message
 * @param eightbit Message contains 8bit chars
 */
int mutt_invoke_sendmail(struct Address *from, struct Address *to, struct Address *cc,
                         struct Address *bcc, const char *msg, int eightbit)
{
  char *ps = NULL, *path = NULL, *s = NULL, *childout = NULL;
  char **args = NULL;
  size_t argslen = 0, argsmax = 0;
  char **extra_args = NULL;
  size_t extra_argslen = 0, extra_argsmax = 0;
  int i;

#ifdef USE_NNTP
  if (option(OPT_NEWS_SEND))
  {
    char cmd[LONG_STRING];

    mutt_expando_format(cmd, sizeof(cmd), 0, MuttIndexWindow->cols,
                        NONULL(Inews), nntp_format_str, 0, 0);
    if (!*cmd)
    {
      i = nntp_post(msg);
      unlink(msg);
      return i;
    }

    s = safe_strdup(cmd);
  }
  else
#endif
    s = safe_strdup(Sendmail);

  /* ensure that $sendmail is set to avoid a crash. http://dev.mutt.org/trac/ticket/3548 */
  if (!s)
  {
    mutt_error(_("$sendmail must be set in order to send mail."));
    return -1;
  }

  ps = s;
  i = 0;
  while ((ps = strtok(ps, " ")))
  {
    if (argslen == argsmax)
      safe_realloc(&args, sizeof(char *) * (argsmax += 5));

    if (i)
    {
      if (mutt_strcmp(ps, "--") == 0)
        break;
      args[argslen++] = ps;
    }
    else
    {
      path = safe_strdup(ps);
      ps = strrchr(ps, '/');
      if (ps)
        ps++;
      else
        ps = path;
      args[argslen++] = ps;
    }
    ps = NULL;
    i++;
  }

#ifdef USE_NNTP
  if (!option(OPT_NEWS_SEND))
  {
#endif
    /* If Sendmail contained a "--", we save the recipients to append to
   * args after other possible options added below. */
    if (ps)
    {
      ps = NULL;
      while ((ps = strtok(ps, " ")))
      {
        if (extra_argslen == extra_argsmax)
          safe_realloc(&extra_args, sizeof(char *) * (extra_argsmax += 5));

        extra_args[extra_argslen++] = ps;
        ps = NULL;
      }
    }

    if (eightbit && option(OPT_USE_8BITMIME))
      args = add_option(args, &argslen, &argsmax, "-B8BITMIME");

    if (option(OPT_USE_ENVELOPE_FROM))
    {
      if (EnvelopeFromAddress)
      {
        args = add_option(args, &argslen, &argsmax, "-f");
        args = add_args(args, &argslen, &argsmax, EnvelopeFromAddress);
      }
      else if (from && !from->next)
      {
        args = add_option(args, &argslen, &argsmax, "-f");
        args = add_args(args, &argslen, &argsmax, from);
      }
    }

    if (DsnNotify)
    {
      args = add_option(args, &argslen, &argsmax, "-N");
      args = add_option(args, &argslen, &argsmax, DsnNotify);
    }
    if (DsnReturn)
    {
      args = add_option(args, &argslen, &argsmax, "-R");
      args = add_option(args, &argslen, &argsmax, DsnReturn);
    }
    args = add_option(args, &argslen, &argsmax, "--");
    for (i = 0; i < extra_argslen; i++)
      args = add_option(args, &argslen, &argsmax, extra_args[i]);
    args = add_args(args, &argslen, &argsmax, to);
    args = add_args(args, &argslen, &argsmax, cc);
    args = add_args(args, &argslen, &argsmax, bcc);
#ifdef USE_NNTP
  }
#endif

  if (argslen == argsmax)
    safe_realloc(&args, sizeof(char *) * (++argsmax));

  args[argslen++] = NULL;

  /* Some user's $sendmail command uses gpg for password decryption,
   * and is set up to prompt using ncurses pinentry.  If we
   * mutt_endwin() it leaves other users staring at a blank screen.
   * So instead, just force a hard redraw on the next refresh. */
  if (!option(OPT_NO_CURSES))
    mutt_need_hard_redraw();

  if ((i = send_msg(path, args, msg, option(OPT_NO_CURSES) ? NULL : &childout)) !=
      (EX_OK & 0xff))
  {
    if (i != S_BKG)
    {
      const char *e = NULL;

      e = mutt_strsysexit(i);
      mutt_error(_("Error sending message, child exited %d (%s)."), i, NONULL(e));
      if (childout)
      {
        struct stat st;

        if (stat(childout, &st) == 0 && st.st_size > 0)
          mutt_do_pager(_("Output of the delivery process"), childout, 0, NULL);
      }
    }
  }
  else if (childout)
    unlink(childout);

  FREE(&childout);
  FREE(&path);
  FREE(&s);
  FREE(&args);
  FREE(&extra_args);

  if (i == (EX_OK & 0xff))
    i = 0;
  else if (i == S_BKG)
    i = 1;
  else
    i = -1;
  return i;
}

/**
 * mutt_prepare_envelope - Prepare an email header
 *
 * For postponing (!final) do the necessary encodings only
 */
void mutt_prepare_envelope(struct Envelope *env, int final)
{
  char buffer[LONG_STRING];

  if (final)
  {
    if (env->bcc && !(env->to || env->cc))
    {
      /* some MTA's will put an Apparently-To: header field showing the Bcc:
       * recipients if there is no To: or Cc: field, so attempt to suppress
       * it by using an empty To: field.
       */
      env->to = rfc822_new_address();
      env->to->group = 1;
      env->to->next = rfc822_new_address();

      buffer[0] = 0;
      rfc822_cat(buffer, sizeof(buffer), "undisclosed-recipients", RFC822Specials);

      env->to->mailbox = safe_strdup(buffer);
    }

    mutt_set_followup_to(env);

    if (!env->message_id)
      env->message_id = gen_msgid();
  }

  /* Take care of 8-bit => 7-bit conversion. */
  rfc2047_encode_adrlist(env->to, "To");
  rfc2047_encode_adrlist(env->cc, "Cc");
  rfc2047_encode_adrlist(env->bcc, "Bcc");
  rfc2047_encode_adrlist(env->from, "From");
  rfc2047_encode_adrlist(env->mail_followup_to, "Mail-Followup-To");
  rfc2047_encode_adrlist(env->reply_to, "Reply-To");

  if (env->subject)
#ifdef USE_NNTP
    if (!option(OPT_NEWS_SEND) || option(OPT_MIME_SUBJECT))
#endif
    {
      rfc2047_encode_string(&env->subject);
    }
  encode_headers(&env->userhdrs);
}

void mutt_unprepare_envelope(struct Envelope *env)
{
  struct ListNode *item;
  STAILQ_FOREACH(item, &env->userhdrs, entries)
  {
    rfc2047_decode(&item->data);
  }

  rfc822_free_address(&env->mail_followup_to);

  /* back conversions */
  rfc2047_decode_adrlist(env->to);
  rfc2047_decode_adrlist(env->cc);
  rfc2047_decode_adrlist(env->bcc);
  rfc2047_decode_adrlist(env->from);
  rfc2047_decode_adrlist(env->reply_to);
  rfc2047_decode(&env->subject);
}

static int _mutt_bounce_message(FILE *fp, struct Header *h, struct Address *to,
                                const char *resent_from, struct Address *env_from)
{
  int ret = 0;
  FILE *f = NULL;
  char date[SHORT_STRING], tempfile[_POSIX_PATH_MAX];
  struct Message *msg = NULL;

  if (!h)
  {
    /* Try to bounce each message out, aborting if we get any failures. */
    for (int i = 0; i < Context->msgcount; i++)
      if (Context->hdrs[i]->tagged)
        ret |= _mutt_bounce_message(fp, Context->hdrs[i], to, resent_from, env_from);
    return ret;
  }

  /* If we failed to open a message, return with error */
  if (!fp && (msg = mx_open_message(Context, h->msgno)) == NULL)
    return -1;

  if (!fp)
    fp = msg->fp;

  mutt_mktemp(tempfile, sizeof(tempfile));
  f = safe_fopen(tempfile, "w");
  if (f)
  {
    int ch_flags = CH_XMIT | CH_NONEWLINE | CH_NOQFROM;
    char *msgid_str = NULL;

    if (!option(OPT_BOUNCE_DELIVERED))
      ch_flags |= CH_WEED_DELIVERED;

    fseeko(fp, h->offset, SEEK_SET);
    fprintf(f, "Resent-From: %s", resent_from);
    fprintf(f, "\nResent-%s", mutt_make_date(date, sizeof(date)));
    msgid_str = gen_msgid();
    fprintf(f, "Resent-Message-ID: %s\n", msgid_str);
    fputs("Resent-To: ", f);
    mutt_write_address_list(to, f, 11, 0);
    mutt_copy_header(fp, h, f, ch_flags, NULL);
    fputc('\n', f);
    mutt_copy_bytes(fp, f, h->content->length);
    FREE(&msgid_str);
    if (safe_fclose(&f) != 0)
    {
      mutt_perror(tempfile);
      unlink(tempfile);
      return -1;
    }
#ifdef USE_SMTP
    if (SmtpUrl)
      ret = mutt_smtp_send(env_from, to, NULL, NULL, tempfile, h->content->encoding == ENC8BIT);
    else
#endif /* USE_SMTP */
      ret = mutt_invoke_sendmail(env_from, to, NULL, NULL, tempfile,
                                 h->content->encoding == ENC8BIT);
  }

  if (msg)
    mx_close_message(Context, &msg);

  return ret;
}

int mutt_bounce_message(FILE *fp, struct Header *h, struct Address *to)
{
  struct Address *from = NULL, *resent_to = NULL;
  const char *fqdn = mutt_fqdn(1);
  char resent_from[STRING];
  int ret;
  char *err = NULL;

  resent_from[0] = '\0';
  from = mutt_default_from();

  /*
   * mutt_default_from() does not use $realname if the real name is not set
   * in $from, so we add it here.  The reason it is not added in
   * mutt_default_from() is that during normal sending, we execute
   * send-hooks and set the realname last so that it can be changed based
   * upon message criteria.
   */
  if (!from->personal)
    from->personal = safe_strdup(RealName);

  if (fqdn)
    rfc822_qualify(from, fqdn);

  rfc2047_encode_adrlist(from, "Resent-From");
  if (mutt_addrlist_to_intl(from, &err))
  {
    mutt_error(_("Bad IDN %s while preparing resent-from."), err);
    rfc822_free_address(&from);
    return -1;
  }
  rfc822_write_address(resent_from, sizeof(resent_from), from, 0);

#ifdef USE_NNTP
  unset_option(OPT_NEWS_SEND);
#endif

  /*
   * prepare recipient list. idna conversion appears to happen before this
   * function is called, since the user receives confirmation of the address
   * list being bounced to.
   */
  resent_to = rfc822_cpy_adr(to, 0);
  rfc2047_encode_adrlist(resent_to, "Resent-To");

  ret = _mutt_bounce_message(fp, h, resent_to, resent_from, from);

  rfc822_free_address(&resent_to);
  rfc822_free_address(&from);

  return ret;
}

/**
 * mutt_remove_duplicates - Remove duplicate addresses
 *
 * given a list of addresses, return a list of unique addresses
 */
struct Address *mutt_remove_duplicates(struct Address *addr)
{
  struct Address *top = addr;
  struct Address **last = &top;
  struct Address *tmp = NULL;
  bool dup;

  while (addr)
  {
    for (tmp = top, dup = false; tmp && tmp != addr; tmp = tmp->next)
    {
      if (tmp->mailbox && addr->mailbox &&
          (mutt_strcasecmp(addr->mailbox, tmp->mailbox) == 0))
      {
        dup = true;
        break;
      }
    }

    if (dup)
    {
      mutt_debug(2, "mutt_remove_duplicates: Removing %s\n", addr->mailbox);

      *last = addr->next;

      addr->next = NULL;
      rfc822_free_address(&addr);

      addr = *last;
    }
    else
    {
      last = &addr->next;
      addr = addr->next;
    }
  }

  return top;
}

static void set_noconv_flags(struct Body *b, short flag)
{
  for (; b; b = b->next)
  {
    if (b->type == TYPEMESSAGE || b->type == TYPEMULTIPART)
      set_noconv_flags(b->parts, flag);
    else if (b->type == TYPETEXT && b->noconv)
    {
      if (flag)
        mutt_set_parameter("x-mutt-noconv", "yes", &b->parameter);
      else
        mutt_delete_parameter("x-mutt-noconv", &b->parameter);
    }
  }
}

/**
 * mutt_write_multiple_fcc - Handle FCC with multiple, comma separated entries
 */
int mutt_write_multiple_fcc(const char *path, struct Header *hdr, const char *msgid,
                            int post, char *fcc, char **finalpath)
{
  char fcc_tok[_POSIX_PATH_MAX];
  char fcc_expanded[_POSIX_PATH_MAX];
  char *tok = NULL;
  int status;

  strfcpy(fcc_tok, path, sizeof(fcc_tok));

  tok = strtok(fcc_tok, ",");
  mutt_debug(1, "Fcc: initial mailbox = '%s'\n", tok);
  /* mutt_expand_path already called above for the first token */
  status = mutt_write_fcc(tok, hdr, msgid, post, fcc, finalpath);
  if (status != 0)
    return status;

  while ((tok = strtok(NULL, ",")) != NULL)
  {
    if (!*tok)
      continue;

    /* Only call mutt_expand_path iff tok has some data */
    mutt_debug(1, "Fcc: additional mailbox token = '%s'\n", tok);
    strfcpy(fcc_expanded, tok, sizeof(fcc_expanded));
    mutt_expand_path(fcc_expanded, sizeof(fcc_expanded));
    mutt_debug(1, "     Additional mailbox expanded = '%s'\n", fcc_expanded);
    status = mutt_write_fcc(fcc_expanded, hdr, msgid, post, fcc, finalpath);
    if (status != 0)
      return status;
  }

  return 0;
}

int mutt_write_fcc(const char *path, struct Header *hdr, const char *msgid,
                   int post, char *fcc, char **finalpath)
{
  struct Context f;
  struct Message *msg = NULL;
  char tempfile[_POSIX_PATH_MAX];
  FILE *tempfp = NULL;
  int r;
  bool need_buffy_cleanup = false;
  struct stat st;
  char buf[SHORT_STRING];
  int onm_flags;

  if (post)
    set_noconv_flags(hdr->content, 1);

  if (mx_open_mailbox(path, MUTT_APPEND | MUTT_QUIET, &f) == NULL)
  {
    mutt_debug(1, "mutt_write_fcc(): unable to open mailbox %s in append-mode, "
                  "aborting.\n",
               path);
    return -1;
  }

  /* We need to add a Content-Length field to avoid problems where a line in
   * the message body begins with "From "
   */
  if (f.magic == MUTT_MMDF || f.magic == MUTT_MBOX)
  {
    mutt_mktemp(tempfile, sizeof(tempfile));
    tempfp = safe_fopen(tempfile, "w+");
    if (!tempfp)
    {
      mutt_perror(tempfile);
      mx_close_mailbox(&f, NULL);
      return -1;
    }
    /* remember new mail status before appending message */
    need_buffy_cleanup = true;
    stat(path, &st);
  }

  hdr->read = !post; /* make sure to put it in the `cur' directory (maildir) */
  onm_flags = MUTT_ADD_FROM;
  if (post)
    onm_flags |= MUTT_SET_DRAFT;
  msg = mx_open_new_message(&f, hdr, onm_flags);
  if (!msg)
  {
    safe_fclose(&tempfp);
    mx_close_mailbox(&f, NULL);
    return -1;
  }

  /* post == 1 => postpone message. Set mode = -1 in mutt_write_rfc822_header()
   * post == 0 => Normal mode. Set mode = 0 in mutt_write_rfc822_header()
   * */
  mutt_write_rfc822_header(msg->fp, hdr->env, hdr->content, post ? -post : 0, 0, 1);

  /* (postponement) if this was a reply of some sort, <msgid> contains the
   * Message-ID: of message replied to.  Save it using a special X-Mutt-
   * header so it can be picked up if the message is recalled at a later
   * point in time.  This will allow the message to be marked as replied if
   * the same mailbox is still open.
   */
  if (post && msgid)
    fprintf(msg->fp, "X-Mutt-References: %s\n", msgid);

  /* (postponement) save the Fcc: using a special X-Mutt- header so that
   * it can be picked up when the message is recalled
   */
  if (post && fcc)
    fprintf(msg->fp, "X-Mutt-Fcc: %s\n", fcc);

  if (f.magic == MUTT_MMDF || f.magic == MUTT_MBOX)
    fprintf(msg->fp, "Status: RO\n");

  /* mutt_write_rfc822_header() only writes out a Date: header with
   * mode == 0, i.e. _not_ postponement; so write out one ourself */
  if (post)
    fprintf(msg->fp, "%s", mutt_make_date(buf, sizeof(buf)));

  /* (postponement) if the mail is to be signed or encrypted, save this info */
  if ((WithCrypto & APPLICATION_PGP) && post && (hdr->security & APPLICATION_PGP))
  {
    fputs("X-Mutt-PGP: ", msg->fp);
    if (hdr->security & ENCRYPT)
      fputc('E', msg->fp);
    if (hdr->security & OPPENCRYPT)
      fputc('O', msg->fp);
    if (hdr->security & SIGN)
    {
      fputc('S', msg->fp);
      if (PgpSignAs && *PgpSignAs)
        fprintf(msg->fp, "<%s>", PgpSignAs);
    }
    if (hdr->security & INLINE)
      fputc('I', msg->fp);
    fputc('\n', msg->fp);
  }

  /* (postponement) if the mail is to be signed or encrypted, save this info */
  if ((WithCrypto & APPLICATION_SMIME) && post && (hdr->security & APPLICATION_SMIME))
  {
    fputs("X-Mutt-SMIME: ", msg->fp);
    if (hdr->security & ENCRYPT)
    {
      fputc('E', msg->fp);
      if (SmimeEncryptWith && *SmimeEncryptWith)
        fprintf(msg->fp, "C<%s>", SmimeEncryptWith);
    }
    if (hdr->security & OPPENCRYPT)
      fputc('O', msg->fp);
    if (hdr->security & SIGN)
    {
      fputc('S', msg->fp);
      if (SmimeDefaultKey && *SmimeDefaultKey)
        fprintf(msg->fp, "<%s>", SmimeDefaultKey);
    }
    if (hdr->security & INLINE)
      fputc('I', msg->fp);
    fputc('\n', msg->fp);
  }

#ifdef MIXMASTER
  /* (postponement) if the mail is to be sent through a mixmaster
   * chain, save that information
   */

  if (post && !STAILQ_EMPTY(&hdr->chain))
  {
    fputs("X-Mutt-Mix:", msg->fp);
    struct ListNode *p;
    STAILQ_FOREACH(p, &hdr->chain, entries)
    {
      fprintf(msg->fp, " %s", (char *) p->data);
    }

    fputc('\n', msg->fp);
  }
#endif

  if (tempfp)
  {
    char sasha[LONG_STRING];
    int lines = 0;

    mutt_write_mime_body(hdr->content, tempfp);

    /* make sure the last line ends with a newline.  Emacs doesn't ensure
     * this will happen, and it can cause problems parsing the mailbox
     * later.
     */
    fseek(tempfp, -1, SEEK_END);
    if (fgetc(tempfp) != '\n')
    {
      fseek(tempfp, 0, SEEK_END);
      fputc('\n', tempfp);
    }

    fflush(tempfp);
    if (ferror(tempfp))
    {
      mutt_debug(1, "mutt_write_fcc(): %s: write failed.\n", tempfile);
      safe_fclose(&tempfp);
      unlink(tempfile);
      mx_commit_message(msg, &f); /* XXX - really? */
      mx_close_message(&f, &msg);
      mx_close_mailbox(&f, NULL);
      return -1;
    }

    /* count the number of lines */
    rewind(tempfp);
    while (fgets(sasha, sizeof(sasha), tempfp) != NULL)
      lines++;
    fprintf(msg->fp, "Content-Length: " OFF_T_FMT "\n", (LOFF_T) ftello(tempfp));
    fprintf(msg->fp, "Lines: %d\n\n", lines);

    /* copy the body and clean up */
    rewind(tempfp);
    r = mutt_copy_stream(tempfp, msg->fp);
    if (fclose(tempfp) != 0)
      r = -1;
    /* if there was an error, leave the temp version */
    if (!r)
      unlink(tempfile);
  }
  else
  {
    fputc('\n', msg->fp); /* finish off the header */
    r = mutt_write_mime_body(hdr->content, msg->fp);
  }

  if (mx_commit_message(msg, &f) != 0)
    r = -1;
  else if (finalpath)
    *finalpath = safe_strdup(msg->commited_path);
  mx_close_message(&f, &msg);
  mx_close_mailbox(&f, NULL);

  if (!post && need_buffy_cleanup)
    mutt_buffy_cleanup(path, &st);

  if (post)
    set_noconv_flags(hdr->content, 0);

  return r;
}
