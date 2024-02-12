#include "util.h"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

void *get_addr_in(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
