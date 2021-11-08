#ifndef MUTT_IPC_H
#define MUTT_IPC_H

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "monitor.h"
#include "errno.h"
#include "mutt/logging.h"

enum ipc_type {
  IPC_COMMAND,
  IPC_MAILBOX,
  IPC_CONFIG
};

struct socket_msg {
  bool ready;
  enum ipc_type type;
  char data[1024];
};

struct socket{
  int fd;
  int conn; /* send data back to client and close connection */
  struct socket_msg msg;
};

extern struct socket Socket;

int socket_create();

#endif
