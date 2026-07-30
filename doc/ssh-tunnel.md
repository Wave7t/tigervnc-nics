# SSH tunnel connection profiles

The native TigerVNC viewer can store an SSH gateway together with the VNC
server in a `.tigervnc` connection profile. In the
connection dialog, open **Options**, select **SSH tunnel**, enable
**Tunnel VNC through SSH**, and enter an OpenSSH destination such as
`user@gateway` or a host alias from your SSH configuration.

The VNC server name is resolved from the gateway. For example, a profile with
`ServerName=localhost:1` connects to display `:1` on the gateway itself, while
`ServerName=desktop.internal:1` asks the gateway to connect to
`desktop.internal`.

TigerVNC does not store SSH passwords, private keys, or key passphrases.
OpenSSH handles authentication and reads advanced settings such as `User`,
`Port`, `IdentityFile`, `ProxyJump`, and host-key policy from its normal
configuration. This is normally `~/.ssh/config` on Unix and
`%USERPROFILE%\.ssh\config` on Windows. Key or agent authentication is
recommended when profiles are opened from a desktop environment without a
terminal.

On Windows, `ssh.exe` also inherits standard OpenSSH authentication settings,
including the Windows OpenSSH agent and an `SSH_ASKPASS` helper when one is
configured.

Saving the connection with **Save as...** writes both `ServerName` and `via`
to the profile. Opening the profile with `vncviewer /path/name.tigervnc`, or
opening the associated file from a desktop environment, creates the SSH
forward and then opens the VNC session.

On Unix, the `VNC_VIA_CMD` environment variable remains available for
compatibility with custom tunnel launchers. Its `$G`, `$H`, `$R`, and `$L`
substitutions are the gateway, VNC host, VNC port, and selected local port.

On Windows, TigerVNC locates `ssh.exe` through `PATH`, launches it directly
without a command shell, and waits for the local forwarding listener before
starting VNC. The SSH process belongs to the connection and is terminated when
the connection closes. Windows OpenSSH is an optional Windows capability and
can be installed from **Settings > Optional features** when it is unavailable.
