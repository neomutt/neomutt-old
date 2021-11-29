#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/neomutt.socket"

enum ipc_type
{
  IPC_COMMAND,
  IPC_MAILBOX,
  IPC_CONFIG,
  IPC_MAIL
};

enum arg_type
{
  ATTACH,
  BCC,
  CC,
  COMMAND,
  CONFIG,
  FOLDER,
  QUERY,
  SUBJECT,
  TO
};

int streq(const char *c1, const char *c2)
{
  return !strcmp(c1, c2);
}

void add_arg(char *c, char *arg)
{
  if (strlen(c) + strlen(arg) + 1 >= 4096)
  {
    printf("[ERRO] Arguments size too big\n");
    exit(1);
  }
  strcat(c, "\x1F"); //unit separator
  strcat(c, arg);
}

int main(int argc, char **argv)
{
  char attach[4096] = { 0 };
  char bcc[4096] = { 0 };
  char cc[4096] = { 0 };
  char command[4096] = { 0 };
  char config[4096] = { 0 };
  char folder[4096] = { 0 };
  char postponed[] = { '\x1D', '\x7', '\x1F', '0', '\0' };
  char query[4096] = { 0 };
  char subject[4096] = { 0 };
  char to[4096] = { 0 };
  char msg[4097] = { 0 };
  attach[0] = '\x1D';
  bcc[0] = '\x1D';
  cc[0] = '\x1D';
  command[0] = '\x1D';
  config[0] = '\x1D';
  folder[0] = '\x1D';
  query[0] = '\x1D';
  subject[0] = '\x1D';
  to[0] = '\x1D';
  attach[1] = '\x1';
  bcc[1] = '\x2';
  cc[1] = '\x3';
  command[1] = '\x4';
  config[1] = '\x5';
  folder[1] = '\x6';
  query[1] = '\x8';
  subject[1] = '\x9';
  to[1] = '\x10';
  // enum ipc_type ipct;
  enum arg_type argt = TO;
  // unsigned int arg = 1;
  strcat(msg, "\2");

  for (int i = 1; i < argc; i++)
  {
    if (streq(argv[i], "-p"))
    {
      postponed[3] = '1';
    }
    else if (streq(argv[i], "-a"))
      argt = ATTACH;
    else if (streq(argv[i], "-b"))
      argt = BCC;
    else if (streq(argv[i], "-c"))
      argt = CC;
    else if (streq(argv[i], "-e"))
      argt = COMMAND;
    else if (streq(argv[i], "-F"))
      argt = CONFIG;
    else if (streq(argv[i], "-f"))
      argt = FOLDER;
    else if (streq(argv[i], "-Q"))
      argt = QUERY;
    else if (streq(argv[i], "-s"))
      argt = SUBJECT;
    else if (streq(argv[i], "-t"))
      argt = TO;
    else
    {
      switch (argt)
      {
        case ATTACH:
          add_arg(attach, argv[i]);
          break;
        case BCC:
          add_arg(bcc, argv[i]);
          break;
        case CC:
          add_arg(cc, argv[i]);
          break;
        case COMMAND:
          add_arg(command, argv[i]);
          break;
        case CONFIG:
          add_arg(config, argv[i]);
          break;
        case FOLDER:
          add_arg(folder, argv[i]);
          break;
        case QUERY:
          add_arg(query, argv[i]);
          break;
        case SUBJECT:
          add_arg(subject, argv[i]);
          break;
        case TO:
          add_arg(to, argv[i]);
          break;
        default:
          break;
      }
    }
  }
  if (strlen(attach) + strlen(bcc) + strlen(cc) + strlen(command) +
          strlen(config) + strlen(folder) + strlen(postponed) + strlen(query) +
          strlen(subject) + strlen(to) + 2 >
      4096) //2 for start and end char
  {
    printf("[ERRO] IPC message too long (>4096)\n");
    exit(1);
  }
  strcat(msg, attach);
  strcat(msg, bcc);
  strcat(msg, cc);
  strcat(msg, command);
  strcat(msg, config);
  strcat(msg, folder);
  strcat(msg, postponed);
  strcat(msg, query);
  strcat(msg, subject);
  strcat(msg, to);
  strcat(msg, "\x1D\3");
  int s, t, len;
  struct sockaddr_un remote;
  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(s, (struct sockaddr *) &remote, len) == -1)
  {
    perror("connect");
    exit(1);
  }
  if (send(s, msg, strlen(msg), 0) == -1)
  {
    perror("send");
    exit(1);
  }
  char resp[1025];
  int ret = 0;
  if ((t = recv(s, resp, 1024, 0)) > 0)
  {
    resp[t] = '\0';
    char *code = strtok(resp, "\x1F");
    char *rmsg = strtok(NULL, "\x1F");
    if (streq(code, "0"))
      printf("[SUCC] ");
    else if (streq(code, "1"))
    {
      printf("[ERRO] ");
      ret = 1;
    }
    else if (streq(code, "2"))
    {
      printf("[WARN] ");
      ret = 1;
    }
    if (rmsg)
      printf("%s", rmsg);
    printf("\n");
  }
  else if (t < 0)
  {
    perror("recv");
    ret = 1;
  }
  else
  {
    printf("Server closed connection\n");
    exit(1);
  }
  close(s);
  return ret;
}

// vim: tabstop=2:shiftwidth=2
