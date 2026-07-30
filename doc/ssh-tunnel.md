# SSH tunnel connection profiles

The native TigerVNC viewer on Linux and macOS can store an SSH gateway
together with the VNC server in a `.tigervnc` connection profile. In the
connection dialog, open **Options**, select **SSH tunnel**, enable
**Tunnel VNC through SSH**, and enter an OpenSSH destination such as
`user@gateway` or a host alias from `~/.ssh/config`.

The VNC server name is resolved from the gateway. For example, a profile with
`ServerName=localhost:1` connects to display `:1` on the gateway itself, while
`ServerName=desktop.internal:1` asks the gateway to connect to
`desktop.internal`.

TigerVNC does not store SSH passwords, private keys, or key passphrases.
OpenSSH handles authentication and reads advanced settings such as `User`,
`Port`, `IdentityFile`, `ProxyJump`, and host-key policy from its normal
configuration. Key or agent authentication is recommended when profiles are
opened from a desktop environment without a terminal.

Saving the connection with **Save as...** writes both `ServerName` and `via`
to the profile. Opening the profile with `vncviewer /path/name.tigervnc`, or
opening the associated file from a Linux or macOS desktop, creates the SSH
forward and then opens the VNC session.

The `VNC_VIA_CMD` environment variable remains available for compatibility
with custom tunnel launchers. Its `$G`, `$H`, `$R`, and `$L` substitutions are
the gateway, VNC host, VNC port, and selected local port.

## Windows extension plan

The connection-file model intentionally uses the existing platform-neutral
`via` setting, so profiles do not need a format change when native Windows
support is added. The Windows viewer currently rejects a profile that requests
an SSH tunnel instead of silently making an unencrypted direct connection.

A Windows implementation should:

1. Move tunnel startup from `vncviewer.cxx` behind a small cross-platform,
   RAII-style process interface.
2. Launch the Windows OpenSSH client with `CreateProcessW`, explicit UTF-16
   argument quoting, `ExitOnForwardFailure=yes`, and no command shell.
3. Keep the process handle in the VNC connection object and place it in a Job
   Object so disconnecting or exiting reliably closes the tunnel.
4. Wait for either the forwarded listener to become ready or the SSH process
   to fail before starting the VNC protocol connection.
5. Use the Windows OpenSSH agent or a standard askpass helper for interactive
   authentication; never add private-key or password storage to `.tigervnc`
   files.
6. Enable the existing SSH options page on Windows and register the
   `.tigervnc` file association in the Inno Setup installer.

The same process abstraction can later replace the legacy Unix shell command,
providing identical readiness and lifetime behavior on all three platforms.
