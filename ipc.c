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
  return 0;
}
