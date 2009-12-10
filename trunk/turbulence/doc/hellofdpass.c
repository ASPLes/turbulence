/* And here's a sample program, called hellofdpass, that takes inbound
   connections portlisten makes, says "hello, world" over them, and
   closes them: */

/* hello, world program that works with portlisten.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/* open a PF_UNIX SOCK_DGRAM socket bound to 'path' */
static int sock_dgram(char *path)
{
  struct sockaddr_un unix_socket_name = {0};
  int fd;
  
  if (unlink(path) < 0) {
    if (errno != ENOENT) {
      fprintf(stderr, "%s: ", path);
      perror("unlink");
      return -1;
    }
  }
  
  unix_socket_name.sun_family = AF_UNIX;

  if (strlen(path) >= sizeof(unix_socket_name.sun_path)) return -1;
  strcpy(unix_socket_name.sun_path, path);
  fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (fd == -1) return -1;
  
  umask (0077);
  if (bind(fd, &unix_socket_name, sizeof(unix_socket_name))) {
    close(fd);
    return -1;
  }
  return fd;
}

/* receive a file descriptor over file descriptor fd */
static int receive_fd(int fd)
{
  struct msghdr msg;
  struct iovec iov;
  char buf[1];
  int rv;
  int connfd = -1;
  char ccmsg[CMSG_SPACE(sizeof(connfd))];
  struct cmsghdr *cmsg;

  iov.iov_base = buf;
  iov.iov_len = 1;

  msg.msg_name = 0;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  /* old BSD implementations should use msg_accrights instead of 
   * msg_control; the interface is different. */
  msg.msg_control = ccmsg;
  msg.msg_controllen = sizeof(ccmsg); /* ? seems to work... */
    
  rv = recvmsg(fd, &msg, 0);
  if (rv == -1) {
    perror("recvmsg");
    return -1;
  }

  cmsg = CMSG_FIRSTHDR(&msg);
  if (!cmsg->cmsg_type == SCM_RIGHTS) {
    fprintf(stderr, "got control message of unknown type %d\n", 
            cmsg->cmsg_type);
    return -1;
  }
  return *(int*)CMSG_DATA(cmsg);
}

void run(char *path, char *string) {

  int fd = sock_dgram(path);
  if (fd == -1) {
    perror("sock_dgram");
    return;
  }
  for (;;) {
    int connfd = receive_fd(fd);
    if (connfd == -1) {
      close(fd);
      return;
    }
    write(connfd, string, strlen(string));
    close(connfd);
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s path string\n", argv[0]);
    return -1;
  }
  run(argv[1], argv[2]);
  return -1;
}
