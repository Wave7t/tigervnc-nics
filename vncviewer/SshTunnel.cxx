#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <stdexcept>
#include <new>

#include <core/i18n.h>
#include <core/string.h>

#include <network/TcpSocket.h>

#include "SshTunnel.h"

class SshTunnel::Process {
};

SshTunnel::SshTunnel(const char* gatewayHost, const char* vncServerName)
  : process(nullptr)
{
  const char* command = getenv("VNC_VIA_CMD");
  std::string remoteHost;
  int localPort;
  int remotePort;
  char localPortString[10];
  char remotePortString[10];
  char* expandedCommand;
  char* percent;

  network::getHostAndPort(vncServerName, &remoteHost, &remotePort);
  localPort = network::findFreeTcpPort();

  snprintf(localPortString, sizeof(localPortString), "%d", localPort);
  snprintf(remotePortString, sizeof(remotePortString), "%d", remotePort);
  setenv("G", gatewayHost, 1);
  setenv("H", remoteHost.c_str(), 1);
  setenv("R", remotePortString, 1);
  setenv("L", localPortString, 1);

  if (command == nullptr)
    command = "/usr/bin/ssh -f -o ExitOnForwardFailure=yes "
              "-L \"$L\":\"$H\":\"$R\" \"$G\" sleep 20";

  expandedCommand = strdup(command);
  if (expandedCommand == nullptr)
    throw std::bad_alloc();

  while ((percent = strchr(expandedCommand, '%')) != nullptr)
    *percent = '$';

  int result = system(expandedCommand);
  free(expandedCommand);

  if (result == -1) {
    throw std::runtime_error(core::format(
      _("Failed to run SSH tunnel command: %s"), strerror(errno)));
  } else if (WIFEXITED(result) && (WEXITSTATUS(result) != 0)) {
    throw std::runtime_error(core::format(
      _("Failed to create SSH tunnel: command exited with status %d"),
      WEXITSTATUS(result)));
  } else if (WIFSIGNALED(result)) {
    throw std::runtime_error(core::format(
      _("Failed to create SSH tunnel: command was terminated by signal %d"),
      WTERMSIG(result)));
  } else if (!WIFEXITED(result)) {
    throw std::runtime_error(_("Failed to create SSH tunnel"));
  }

  serverName = core::format("localhost::%d", localPort);
}

SshTunnel::~SshTunnel()
{
  delete process;
}
