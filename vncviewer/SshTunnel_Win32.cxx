#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <core/Exception.h>
#include <core/i18n.h>
#include <core/string.h>

#include <network/TcpSocket.h>

#include "SshTunnel.h"

static std::wstring quoteArgument(const std::wstring& argument)
{
  std::wstring quoted;
  size_t backslashes;

  if (!argument.empty() &&
      (argument.find_first_of(L" \t\n\v\"") == std::wstring::npos))
    return argument;

  quoted.push_back(L'\"');
  backslashes = 0;
  for (wchar_t character : argument) {
    if (character == L'\\') {
      backslashes++;
      continue;
    }

    if (character == L'\"')
      quoted.append(backslashes * 2 + 1, L'\\');
    else
      quoted.append(backslashes, L'\\');

    quoted.push_back(character);
    backslashes = 0;
  }

  quoted.append(backslashes * 2, L'\\');
  quoted.push_back(L'\"');
  return quoted;
}

static std::wstring findSshExecutable()
{
  DWORD environmentLength;
  std::vector<wchar_t> environment;
  std::vector<wchar_t> path(MAX_PATH);

  environmentLength = GetEnvironmentVariableW(L"PATH", nullptr, 0);
  if (environmentLength == 0)
    throw core::win32_error(_("Unable to read PATH"), GetLastError());

  environment.resize(environmentLength);
  if (GetEnvironmentVariableW(L"PATH", environment.data(),
                              environment.size()) == 0) {
    throw core::win32_error(_("Unable to read PATH"), GetLastError());
  }

  while (true) {
    DWORD length = SearchPathW(environment.data(), L"ssh.exe", nullptr,
                               path.size(), path.data(), nullptr);
    if (length == 0)
      throw core::win32_error(_("Unable to find ssh.exe"), GetLastError());
    if (length < path.size())
      return path.data();
    path.resize(length + 1);
  }
}

static bool processOwnsListener(DWORD processId, int port)
{
  DWORD size = 0;
  DWORD result;
  std::vector<unsigned char> buffer;

  result = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET,
                               TCP_TABLE_OWNER_PID_LISTENER, 0);
  if (result != ERROR_INSUFFICIENT_BUFFER)
    return false;

  buffer.resize(size);
  PMIB_TCPTABLE_OWNER_PID table =
    reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
  result = GetExtendedTcpTable(table, &size, FALSE, AF_INET,
                               TCP_TABLE_OWNER_PID_LISTENER, 0);
  if (result != NO_ERROR)
    throw core::win32_error(_("Failed to inspect SSH tunnel listener"),
                            result);

  for (DWORD index = 0; index < table->dwNumEntries; index++) {
    const MIB_TCPROW_OWNER_PID& row = table->table[index];
    if ((row.dwOwningPid == processId) &&
        (ntohs(static_cast<u_short>(row.dwLocalPort)) == port))
      return true;
  }

  return false;
}

class SshTunnel::Process {
public:
  Process() : job(nullptr), process(nullptr), processId(0) {}

  ~Process()
  {
    if (job != nullptr)
      CloseHandle(job);
    if (process != nullptr) {
      WaitForSingleObject(process, 5000);
      CloseHandle(process);
    }
  }

  void start(const std::wstring& executable,
             const std::vector<std::wstring>& arguments)
  {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
    PROCESS_INFORMATION processInfo;
    STARTUPINFOW startupInfo;
    std::wstring commandLine;
    std::vector<wchar_t> mutableCommandLine;

    memset(&jobInfo, 0, sizeof(jobInfo));
    memset(&processInfo, 0, sizeof(processInfo));
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    job = CreateJobObjectW(nullptr, nullptr);
    if (job == nullptr)
      throw core::win32_error(_("Failed to create SSH tunnel job"),
                              GetLastError());

    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                 &jobInfo, sizeof(jobInfo))) {
      throw core::win32_error(_("Failed to configure SSH tunnel job"),
                              GetLastError());
    }

    commandLine = quoteArgument(executable);
    for (const std::wstring& argument : arguments) {
      commandLine.push_back(L' ');
      commandLine += quoteArgument(argument);
    }
    mutableCommandLine.assign(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    if (!CreateProcessW(executable.c_str(), mutableCommandLine.data(),
                        nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED,
                        nullptr, nullptr, &startupInfo, &processInfo)) {
      throw core::win32_error(_("Failed to start ssh.exe"), GetLastError());
    }

    process = processInfo.hProcess;
    processId = processInfo.dwProcessId;

    if (!AssignProcessToJobObject(job, process)) {
      DWORD error = GetLastError();
      TerminateProcess(process, 1);
      CloseHandle(processInfo.hThread);
      throw core::win32_error(_("Failed to assign ssh.exe to tunnel job"),
                              error);
    }

    if (ResumeThread(processInfo.hThread) == static_cast<DWORD>(-1)) {
      DWORD error = GetLastError();
      CloseHandle(processInfo.hThread);
      throw core::win32_error(_("Failed to resume ssh.exe"), error);
    }
    CloseHandle(processInfo.hThread);
  }

  void waitForListener(int port)
  {
    for (unsigned elapsed = 0; elapsed < 15000; elapsed += 50) {
      DWORD waitResult = WaitForSingleObject(process, 0);
      if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode;
        if (!GetExitCodeProcess(process, &exitCode))
          throw core::win32_error(_("Failed to read ssh.exe exit status"),
                                  GetLastError());
        throw std::runtime_error(core::format(
          _("Failed to create SSH tunnel: ssh.exe exited with status %lu"),
          static_cast<unsigned long>(exitCode)));
      }
      if (waitResult == WAIT_FAILED)
        throw core::win32_error(_("Failed waiting for ssh.exe"),
                                GetLastError());

      if (processOwnsListener(processId, port))
        return;

      Sleep(50);
    }

    throw std::runtime_error(
      _("Timed out waiting for the SSH tunnel to become ready"));
  }

private:
  HANDLE job;
  HANDLE process;
  DWORD processId;
};

SshTunnel::SshTunnel(const char* gatewayHost, const char* vncServerName)
  : process(nullptr)
{
  std::string remoteHost;
  int localPort;
  int remotePort;

  network::getHostAndPort(vncServerName, &remoteHost, &remotePort);
  localPort = network::findFreeTcpPort();

  if (remoteHost.find(':') != std::string::npos)
    remoteHost = core::format("[%s]", remoteHost.c_str());

  std::wstring executable = findSshExecutable();
  std::vector<std::wstring> arguments = {
    L"-N",
    L"-o",
    L"ExitOnForwardFailure=yes",
    L"-L",
    core::utf8ToUTF16(core::format("127.0.0.1:%d:%s:%d",
                                   localPort, remoteHost.c_str(), remotePort)
                        .c_str()),
    L"--",
    core::utf8ToUTF16(gatewayHost)
  };

  std::unique_ptr<Process> newProcess(new Process());
  newProcess->start(executable, arguments);
  newProcess->waitForListener(localPort);
  process = newProcess.release();

  serverName = core::format("127.0.0.1::%d", localPort);
}

SshTunnel::~SshTunnel()
{
  delete process;
}
