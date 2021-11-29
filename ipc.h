#ifndef MUTT_IPC_H
#define MUTT_IPC_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "monitor.h"
#include "errno.h"
#include "mutt/logging.h"
#include "string.h"
#include "mutt/list.h"
#include "stdlib.h"

struct ipc_data {
  struct ListHead attach;
  struct ListHead bcc;
  struct ListHead cc;
  struct ListHead config;
  struct ListHead command;
  struct ListHead folder;
  bool postponed;
  struct ListHead query;
  char *subject;
  struct ListHead to;
};

struct socket_msg {
  bool ready;
  struct ipc_data data;
};

struct socket{
  int fd;
  int conn; /* send data back to client and close connection */
  struct socket_msg msg;
};

extern struct socket Socket;

int streq(const char *c1, const char *c2);
int socket_create();
void ipc_populate_data(char *buf);
void close_conn(int ret, char *msg);
void ipc_clear_data();

#endif
