/* In Unix, many of the bigger violations of the principle of least
privilege stem from the rule that only root can bind to TCP or UDP
ports below 1024.  Many network daemons run as root just so they can
do this.

This pair of programs illustrates how you can have a small, simple
program bind to the port and accept connections, while the program
that actually handles inbound connections can remain unprivileged.
After I wrote this, I realized that it would probably be more sensible
to have the small, simple program bind to the port and pass the file
descriptor for the listening socket to the other program, but as it
is, it passes the existing connections over instead.  (There's some
weak justification for this in the comments in the program.)

There are other reasons you might want to transmit open file
descriptors to other programs.

So here's portlisten.c, which binds to the port and listens on it:
*/
/* I think listening on ports should be separated from the rest of the
 * program for a couple of reasons:
 * - Frequently, listening on ports requires more privileges than the rest of
 *   the program.
 * - If there's a crash-causing bug in the program, if the program listens on
 *   the port itself, then crashing it will cause the port to stop listening 
 *   until the program is restarted; aside from being (frequently user-
 *   visible) incorrect behavior, this is a helpful clue to would-be attackers
 *   that they have found a serious, possibly exploitable, bug in the program.
 * Unix lets one pass open file descriptors across Unix-domain SOCK_DGRAM 
 * sockets, so that's the approach this program takes.  You give it a pathname
 * for a Unix-domain socket to try to pass connections to and a port number,
 * and it listens on the port and passes connections to a Unix-domain socket
 * at that pathname.
 * Another approach that solves the same two problems is to listen on the port
 * and then fork and let the child process do all the work, but let the parent
 * process restart the child when it dies.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int unix_socket_fd = -1;
static struct sockaddr_un unix_socket_name = {0};

static int send_connection(int fd)
{
  struct msghdr msg;
  char ccmsg[CMSG_SPACE(sizeof(fd))];
  struct cmsghdr *cmsg;
  struct iovec vec;  /* stupidity: must send/receive at least one byte */
  char *str = "x";
  int rv;
  
  msg.msg_name = (struct sockaddr*)&unix_socket_name;
  msg.msg_namelen = sizeof(unix_socket_name);

  vec.iov_base = str;
  vec.iov_len = 1;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;

  /* old BSD implementations should use msg_accrights instead of 
   * msg_control; the interface is different. */
  msg.msg_control = ccmsg;
  msg.msg_controllen = sizeof(ccmsg);
  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
  *(int*)CMSG_DATA(cmsg) = fd;
  msg.msg_controllen = cmsg->cmsg_len;

  msg.msg_flags = 0;

  rv = (sendmsg(unix_socket_fd, &msg, 0) != -1);
  if (rv) close(fd);
  return rv;
}

#ifndef UNIX_PATH_MAX
/* uh-oh, nothing safe to do here */
static int UNIX_PATH_MAX = sizeof(unix_socket_name.sun_path);
#endif

static int open_unix_fd(char *path)
{
  unix_socket_name.sun_family = AF_UNIX;
  if (strlen(path) >= UNIX_PATH_MAX - 1) return 0;
  strcpy(unix_socket_name.sun_path, path);
  unix_socket_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (unix_socket_fd == -1) return 0;
  /* doesn't do anything at the moment:
   * connect(unix_socket_fd, (sockaddr*)&unix_socket_name); */
  return 1;
}

static void maybeclose(int fd)
{
  if (fd != -1) close(fd);
}

static void close_unix_fd()
{
  maybeclose(unix_socket_fd);
  unix_socket_fd = -1;
}

/* xxx listen on particular iface? */
static int listen_on_port(int port)
{
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in sin;
  int one = 1;
  
  if (sock == -1) return sock;
  memset(&sin, '\0', sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY;
  /* all these functions return 0 on success, -1 on failure; how stupid */
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&one, sizeof(one)) ||
      bind(sock, (struct sockaddr*)&sin, sizeof(sin)) ||
      listen(sock, 5)) {
    close(sock);
    return -1;
  }
  return sock;
}

int run(int port, char *path)
{
  int sock = listen_on_port(port);
  
  if (sock == -1) {
    perror("listening on port");
    goto error;
  }

  if (!open_unix_fd(path)) {
    perror("opening Unix fd");
    goto error;
  }
  
  for (;;) {
    int ii = 0;

    int fd = accept(sock, 0, 0);
    if (fd == -1) {
      perror("accept");
      /* stupid Linux returns pending network errors on the new socket as
       * error codes from accept(); this monstrosity is what accept(2) 
       * actually recommends doing: */
      switch(errno) {
      case ENETDOWN: case EPROTO: case ENOPROTOOPT: case EHOSTDOWN: 
      case ENONET: case EHOSTUNREACH: case EOPNOTSUPP: case ENETUNREACH:
        continue;
      default:
        goto error;
      }
    }
    
    while (!send_connection(fd)) {
      /* two magic constants here: 100 ms because it's comparable to the time
       * between two arbitrary Internet hosts at the moment (so it won't be
       * a noticeable additional delay, unlike, say, 1 second) and trying
       * to resend ten times a second won't impose a noticeable load on the
       * machine; and 600 tries, because a minute is long enough to get
       * any reasonable service restarted (even if it's written in Java),
       * and short enough that you'll still probably see the errors while you
       * can still remember what you broke */
      struct timeval hundredms;
      hundredms.tv_sec = 0;
      hundredms.tv_usec = 100000;

      if (++ii > 600) {
        perror("600 times: sendmsg");
        goto error;
      }

      /* a portable way to sleep 100 ms on POSIX */
      select(0, 0, 0, 0, &hundredms);
    }
  }
     
error:
  maybeclose(sock);
  close_unix_fd();
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s portnum path\n", argv[0]);
    return -1;
  }
  run(atoi(argv[1]), argv[2]);
  return -1;
}



