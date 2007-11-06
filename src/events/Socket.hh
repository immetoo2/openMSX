// $Id$

#ifndef SOCKET_HH
#define SOCKET_HH

#include <string>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include <winsock.h>
#endif

namespace openmsx {

#ifndef _WIN32
static const int OPENMSX_INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;
typedef int SOCKET;
#else
// INVALID_SOCKET is #defined as  (SOCKET)(~0)
// but that gives a old-style-cast warning
static const SOCKET OPENMSX_INVALID_SOCKET = static_cast<SOCKET>(~0);
#endif

std::string sock_error();
void sock_startup();
void sock_cleanup();
void sock_reuseAddr(int sd);
void sock_close(int sd);
int sock_recv(int sd, char* buf, size_t count);
int sock_send(int sd, const char* buf, size_t count);

} // namespace openmsx

#endif
