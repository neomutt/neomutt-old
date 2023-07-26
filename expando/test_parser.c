#include <stdio.h>
#include "parser.h"

#if 0

int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    const char *text = argv[i];
    printf("`%s`\n", text);

    struct ExpandoParseError error = { 0 };
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, text, &error);

    if (error.position == NULL)
    {
      expando_tree_print(stdout, &root);
    }
    else
    {
      int location = error.position - argv[i];
      printf("%*s^\n", location, "");
      printf("Parsing error: %s\n", error.message);
    }

    expando_tree_free(&root);
  }

  return 0;
}

#endif
