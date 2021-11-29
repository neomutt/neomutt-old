#include "ipc.h"

struct socket Socket;

int socket_create()
{
  Socket.fd = -1;
  Socket.msg.ready = false;

  struct sockaddr_un localSockaddr;
  if ((Socket.fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    mutt_debug(LL_DEBUG2, "monitor: socket creation failed, errno=%d %s\n",
               errno, strerror(errno));
    return -1;
  }
  localSockaddr.sun_family = AF_UNIX;
  strcpy(localSockaddr.sun_path, "/tmp/neomutt.socket");
  unlink(localSockaddr.sun_path);
  unsigned int len = strlen(localSockaddr.sun_path) + sizeof(localSockaddr.sun_family);
  if (bind(Socket.fd, (struct sockaddr *) &localSockaddr, len) == -1)
  {
    return -1;
  }
  if (listen(Socket.fd, 5) == -1)
    return -1;
  Socket.msg.data.attach = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.attach);
  Socket.msg.data.bcc = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.bcc);
  Socket.msg.data.cc = (struct ListHead) STAILQ_HEAD_INITIALIZER(Socket.msg.data.cc);
  Socket.msg.data.config = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.config);
  Socket.msg.data.command = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.command);
  Socket.msg.data.folder = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.folder);
  Socket.msg.data.postponed = false;
  Socket.msg.data.query = (struct ListHead) STAILQ_HEAD_INITIALIZER(
      Socket.msg.data.query);
  Socket.msg.data.to = (struct ListHead) STAILQ_HEAD_INITIALIZER(Socket.msg.data.to);
  return 0;
}

/*
close_conn()
{
  Socket.msg.data.postponed = false;
  close(Socket.conn);
}
*/

int streq(const char *c1, const char *c2)
{
  return !strcmp(c1, c2);
}

void ipc_populate_data(char *buf)
{
  Socket.msg.ready = true;
  char *group;
  char *grest;
  unsigned int i;
  for (i = 0, group = strtok_r(buf, "\x1D", &grest); group;
       i++, group = strtok_r(NULL, "\x1D", &grest))
  {
    if (streq(group, "\2") || streq(group, "\3"))
      continue;
    char *unit;
    char *urest;
    for (unit = strtok_r(group + 1, "\x1F", &urest); unit;
         unit = strtok_r(NULL, "\x1F", &urest))
    {
      switch (group[0])
      {
        case '\x1':
          mutt_list_insert_tail(&Socket.msg.data.attach, mutt_str_dup(unit));
          break;
        case '\x2':
          mutt_list_insert_tail(&Socket.msg.data.bcc, mutt_str_dup(unit));
          break;
        case '\x3':
          mutt_list_insert_tail(&Socket.msg.data.cc, mutt_str_dup(unit));
          break;
        case '\x4':
          mutt_list_insert_tail(&Socket.msg.data.command, mutt_str_dup(unit));
          break;
        case '\x5':
          mutt_list_insert_tail(&Socket.msg.data.config, mutt_str_dup(unit));
          break;
        case '\x6':
          mutt_list_insert_tail(&Socket.msg.data.folder, mutt_str_dup(unit));
          break;
        case '\x7':
          if (streq(unit, "1"))
            Socket.msg.data.postponed = true;
          break;
        case '\x8':
          mutt_list_insert_tail(&Socket.msg.data.query, mutt_str_dup(unit));
          break;
        case '\x9':
          Socket.msg.data.subject = mutt_str_dup(unit);
          break;
        case '\x10':
          char *tmp = mutt_str_dup(unit);
          mutt_list_insert_tail(&Socket.msg.data.to, tmp);
          break;
      }
    }
  }
}

void close_conn(int ret, char *msg)
{
  char resp[1024];
  sprintf(resp, "%d\x1F%s", ret, msg);
  send(Socket.conn, resp, strlen(resp), 0);
  close(Socket.conn);
}

void ipc_clear_data()
{
  mutt_list_free(&Socket.msg.data.attach);
  mutt_list_free(&Socket.msg.data.bcc);
  mutt_list_free(&Socket.msg.data.cc);
  mutt_list_free(&Socket.msg.data.command);
  mutt_list_free(&Socket.msg.data.config);
  mutt_list_free(&Socket.msg.data.folder);
  Socket.msg.data.postponed = true;
  mutt_list_free(&Socket.msg.data.query);
  free(Socket.msg.data.subject);
  mutt_list_free(&Socket.msg.data.to);
  Socket.msg.ready = false;
}
