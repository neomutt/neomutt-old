#include <assert.h>
#include "email/lib.h"
#include "color/color.h"
#include "hdrline.h"
#include "helpers.h"
#include "index_format_callbacks.h"
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

void index_C(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  const struct ExpandoExpandoNode *node = (struct ExpandoExpandoNode *) self;

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  // TODO(g0mb4): handle *start_col != 0
  char fmt[128];

  size_t colorlen = add_index_color_2gmb(fmt, sizeof(fmt), flags, MT_COLOR_INDEX_NUMBER);
  // TODO(g0mb4): see if it can be generalised
  if (node->format)
  {
    const int fmt_len = (int) (node->format->end - node->format->start);
    snprintf(fmt + colorlen, sizeof(fmt) - colorlen, "%%%.*sd", fmt_len,
             node->format->start);
  }
  else
  {
    snprintf(fmt + colorlen, sizeof(fmt) - colorlen, "%%d");
  }

  add_index_color_2gmb(fmt + colorlen, sizeof(fmt) - colorlen, flags, MT_COLOR_INDEX);
  int printed = snprintf(*buffer, *buffer_len, fmt, e->msgno + 1);

  *start_col = mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}