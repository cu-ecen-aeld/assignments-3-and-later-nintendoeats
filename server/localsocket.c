#include "aesdsocket.h"

// from https://www.gnu.org/software/libc/manual/html_node/Local-Socket-Example.html
int MakeNamedSocket (const char *filename)
{
  struct sockaddr_un name;
  int sock;
  size_t size;

  /* Create the socket. */
  sock = socket (PF_LOCAL, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      fail("Error creating local socket", -2);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        fail("setsockopt(SO_REUSEADDR) failed", -2);

  /* Bind a name to the socket. */
  name.sun_family = AF_LOCAL;
  strncpy (name.sun_path, filename, sizeof (name.sun_path));
  name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
 */
  size = (offsetof (struct sockaddr_un, sun_path)
          + strlen (name.sun_path));

  if (bind (sock, (struct sockaddr *) &name, size) < 0)
    {
      fail("Error binding local socket", -2);
    }

  return sock;
}



atomic_int interthreadSocketFD = 0;

void InitITS()
{
    interthreadSocketFD = MakeNamedSocket("");
}

void CloseITS()
{
    close(interthreadSocketFD);
    interthreadSocketFD = 0;
}

void SendOnITS(const char* message, size_t length)
{
    syslog(LOG_DEBUG, "Sending interthread %d",*(int*)message);
    send(interthreadSocketFD, message, length, 0);
}












