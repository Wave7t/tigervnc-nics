#ifndef __SSHTUNNEL_H__
#define __SSHTUNNEL_H__

#include <string>

class SshTunnel {
public:
  SshTunnel(const char* gatewayHost, const char* vncServerName);
  ~SshTunnel();

  SshTunnel(const SshTunnel&) = delete;
  SshTunnel& operator=(const SshTunnel&) = delete;

  const char* getServerName() const { return serverName.c_str(); }

private:
  class Process;

  std::string serverName;
  Process* process;
};

#endif
