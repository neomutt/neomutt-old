#include "config.h"
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "parser.h"

static void print_node(FILE *fp, const struct ExpandoNode *n, int indent);

static void print_empty_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  fprintf(fp, "%*sEMPTY\n", indent, "");
}

static void print_text_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  const int len = n->end - n->start;
  fprintf(fp, "%*sTEXT: `%.*s`\n", indent, "", len, n->start);
}

static void print_expando_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  struct ExpandoFormatPrivate *f = (struct ExpandoFormatPrivate *) n->ndata;
  if (f)
  {
    const int elen = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`", indent, "", elen, n->start);

    const char *just = NULL;

    switch (f->justification)
    {
      case JUSTIFY_LEFT:
        just = "LEFT";
        break;

      case JUSTIFY_CENTER:
        just = "CENTER";
        break;

      case JUSTIFY_RIGHT:
        just = "RIGHT";
        break;

      default:
        assert(0 && "Unknown justification.");
    }
    fprintf(fp, " (min=%d, max=%d, just=%s, leader=`%c`)\n", f->min, f->max, just, f->leader);
  }
  else
  {
    const int len = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`\n", indent, "", len, n->start);
  }
}

static void print_date_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  assert(n->ndata);
  struct ExpandoDatePrivate *d = (struct ExpandoDatePrivate *) n->ndata;

  const int len = n->end - n->start;
  const char *dt = NULL;

  switch (d->date_type)
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
          n->start, dt, d->ingnore_locale);
}

static void print_pad_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  assert(n->ndata);
  struct ExpandoPadPrivate *p = (struct ExpandoPadPrivate *) n->ndata;

  const char *pt = NULL;
  switch (p->pad_type)
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

static void print_condition_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  assert(n->ndata);
  struct ExpandoConditionPrivate *c = (struct ExpandoConditionPrivate *) n->ndata;

  fprintf(fp, "%*sCONDITION: ", indent, "");
  print_node(fp, c->condition, indent);
  fprintf(fp, "%*sIF TRUE : ", indent + 4, "");
  print_node(fp, c->if_true, indent + 4);

  if (c->if_false)
  {
    fprintf(fp, "%*sIF FALSE: ", indent + 4, "");
    print_node(fp, c->if_false, indent + 4);
  }
}

static void print_index_format_hook_node(FILE *fp, const struct ExpandoNode *n, int indent)
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
      print_text_node(fp, n, indent);
    }
    break;

    case NT_EXPANDO:
    {
      print_expando_node(fp, n, indent);
    }
    break;

    case NT_DATE:
    {
      print_date_node(fp, n, indent);
    }
    break;

    case NT_PAD:
    {
      print_pad_node(fp, n, indent);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      print_index_format_hook_node(fp, n, indent);
    }
    break;

    case NT_CONDITION:
    {
      print_condition_node(fp, n, indent);
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