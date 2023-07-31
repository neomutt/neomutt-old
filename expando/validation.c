#include "core/neomutt.h"
#include "mutt.h"
#include "validation.h"

static const char *alias_1[] = {
  "a", "c", "f", "n", "r", "t", NULL,
};

static const char *attach_1[] = {
  "c", "C", "d", "D", "e", "f", "F", "I", "m",
  "M", "n", "Q", "s", "t", "T", "u", "X", NULL,
};

static const char *index_1[] = {
  "a", "A", "b", "B", "c", "C", "d", "D", "e", "E", "f", "F", "g", "H",
  "i", "I", "J", "K", "l", "L", "m", "M", "n", "N", "O", "P", "q", "r",
  "R", "s", "S", "t", "T", "u", "v", "W", "x", "X", "y", "Y", "Z", NULL,
};

static const char *index_2[] = {
  "cr", "Fp", "Gx", "zc", "zs", "zt", NULL,
};

static struct ExpandoValidation expando_validation[EFMT_FORMAT_COUNT] = {
  [EFMT_ALIAS_FORMAT] { "alias_format", alias_1, NULL, },
  [EFMT_ATTACH_FORMAT] { "attach_format", attach_1, NULL },
  [EFMT_INDEX_FORMAT] { "index_format", index_1, index_2, },
};

bool expando_validate_string(struct Buffer *name, struct Buffer *value, struct Buffer *err)
{
  for (enum ExpandoFormatIndex i = EFMT_ALIAS_FORMAT; i < EFMT_FORMAT_COUNT; ++i)
  {
    if (mutt_str_equal(name->data, expando_validation[i].name))
    {
      const char *input = mutt_str_dup(value->data);
      struct ExpandoParseError error = { 0 };
      struct ExpandoNode *root = NULL;

      expando_tree_parse(&root, &input, expando_validation[i].valid_short_expandos,
                         expando_validation[i].valid_two_char_expandos, NULL, &error);

      if (error.position != NULL)
      {
        buf_printf(err, _("$%s: %s\nDefault value will be used."), name->data,
                   error.message);
        expando_tree_free(&root);
        // NOTE(g0mb4): segfaults on free
        //FREE(input);
        return false;
      }

      // NOTE(g0mb4): no nedd to free(), since they are "global"
      NeoMutt->expando_table.table[i].string = input;
      NeoMutt->expando_table.table[i].tree = root;
      return true;
    }
  }

  return true;
}