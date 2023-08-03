#include "config.h"
#include <assert.h>
#include <stdio.h>
#include "parser.h"

static void print_node(FILE *fp, const struct ExpandoNode *n, int indent);

static void print_empty_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  fprintf(fp, "%*sEMPTY\n", indent, "");
}

static void print_text_node(FILE *fp, const struct ExpandoTextNode *n, int indent)
{
  const int len = n->end - n->start;
  fprintf(fp, "%*sTEXT: `%.*s`\n", indent, "", len, n->start);
}

static void print_expando_node(FILE *fp, const struct ExpandoExpandoNode *n, int indent)
{
  if (n->format)
  {
    const int elen = n->end - n->start;
    const struct ExpandoFormat *f = n->format;
    fprintf(fp, "%*sEXPANDO: `%.*s`", indent, "", elen, n->start);

    const char *just = f->justification == FMT_J_RIGHT ? "RIGHT" : "LEFT";
    fprintf(fp, " (min=%d, max=%d, just=%s, leader=`%c`)\n", f->min, f->max, just, f->leader);
  }
  else
  {
    const int len = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`\n", indent, "", len, n->start);
  }
}

static void print_date_node(FILE *fp, const struct ExpandoDateNode *n, int indent)
{
  const int len = n->end - n->start;
  const char *dt = NULL;

  switch (n->date_type)
  {
    case DT_SENDER_SEND_TIME:
      dt = "SENDER_SEND_TIME";
      break;

    case DT_LOCAL_SEND_TIME:
      dt = "DT_LOCAL_SEND_TIME";
      break;

    case DT_LOCAL_RECIEVE_TIME:
      dt = "DT_LOCAL_RECIEVE_TIME";
      break;

    default:
      assert(0 && "Unknown date type.");
  }

  fprintf(fp, "%*sDATE: `%.*s` (type=%s, ignore_locale=%d)\n", indent, "", len,
          n->start, dt, n->ingnore_locale);
}

static void print_pad_node(FILE *fp, const struct ExpandoPadNode *n, int indent)
{
  const char *pt = NULL;
  switch (n->pad_type)
  {
    case PT_FILL:
      pt = "FILL";
      break;

    case PT_HARD_FILL:
      pt = "HARD_FILL";
      break;

    case PT_SOFT_FILL:
      pt = "SOFT_FILL";
      break;

    default:
      assert(0 && "Unknown pad type.");
  }

  const int len = n->end - n->start;
  fprintf(fp, "%*sPAD: `%.*s` (type=%s)\n", indent, "", len, n->start, pt);
}

static void print_condition_node(FILE *fp, const struct ExpandoConditionNode *n, int indent)
{
  fprintf(fp, "%*sCONDITION: ", indent, "");
  print_node(fp, n->condition, indent);
  fprintf(fp, "%*sIF TRUE : ", indent + 4, "");
  print_node(fp, n->if_true, indent + 4);

  if (n->if_false)
  {
    fprintf(fp, "%*sIF FALSE: ", indent + 4, "");
    print_node(fp, n->if_false, indent + 4);
  }
}

static void print_index_format_hook_node(FILE *fp,
                                         const struct ExpandoIndexFormatHookNode *n, int indent)
{
  const int len = n->end - n->start;
  fprintf(fp, "%*sINDEX FORMAT HOOK: `%.*s`\n", indent, "", len, n->start);
}

static void print_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  if (n == NULL)
  {
    fprintf(fp, "<null>\n");
    return;
  }

  switch (n->type)
  {
    case NT_EMPTY:
    {
      print_empty_node(fp, n, indent);
    }
    break;

    case NT_TEXT:
    {
      const struct ExpandoTextNode *nn = (const struct ExpandoTextNode *) n;
      print_text_node(fp, nn, indent);
    }
    break;

    case NT_EXPANDO:
    {
      const struct ExpandoExpandoNode *nn = (const struct ExpandoExpandoNode *) n;
      print_expando_node(fp, nn, indent);
    }
    break;

    case NT_DATE:
    {
      const struct ExpandoDateNode *nn = (const struct ExpandoDateNode *) n;
      print_date_node(fp, nn, indent);
    }
    break;

    case NT_PAD:
    {
      const struct ExpandoPadNode *nn = (const struct ExpandoPadNode *) n;
      print_pad_node(fp, nn, indent);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      const struct ExpandoIndexFormatHookNode *nn = (const struct ExpandoIndexFormatHookNode *) n;
      print_index_format_hook_node(fp, nn, indent);
    }
    break;

    case NT_CONDITION:
    {
      const struct ExpandoConditionNode *nn = (const struct ExpandoConditionNode *) n;
      print_condition_node(fp, nn, indent);
    }
    break;

    default:
      assert(0 && "Unknown node.");
  }
}

void expando_tree_print(FILE *fp, struct ExpandoNode **root)
{
  const struct ExpandoNode *n = *root;
  while (n)
  {
    print_node(fp, n, 0);
    n = n->next;
  }
}
