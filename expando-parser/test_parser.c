#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    const char *text = argv[i];
    printf("`%s`\n", text);

    struct ParseError error = { 0 };
    struct Node *root = NULL;

    parse_tree(&root, text, &error);

    if (error.position == NULL)
    {
      print_tree(stdout, &root);
    }
    else
    {
      int location = error.position - argv[i];
      printf("%*s^\n", location, "");
      printf("Parsing error: %s\n", error.message);
    }

    free_tree(&root);
  }

  return 0;
}
