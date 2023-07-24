#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    const char *text = argv[i];
    printf("`%s`\n", text);

    struct Node *root = NULL;
    parse_tree(&root, text);
    print_tree(stdout, &root);
    free_tree(&root);
  }

  return 0;
}