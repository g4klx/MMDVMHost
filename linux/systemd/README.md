MMDVMHost Systemd setup
=======================


Install Instructions
--------------------

  1. Copy mmdvmhost_service to   /usr/local/sbin
  2. Change the permisions of /usr/local/sbin/mmdvmhost_service to 500 (root executable)
  3. Change the variables in /usr/local/sbin/mmdvmhost_service to match your setup
      (MMDVMHost.ini location, log location, user and daemon location)
  4. Copy mmdvmhost.service to   /lib/systemd/system
  5. Copy mmdvmhost.timer to     /lib/systemd/system
  6. Edit the timeout in mmdvmhost.timer to suit - 45 secs is a reasonable starting point.
  7. Reload systemctl with: "systemctl daemon-reload"
  8. Add the timer serice to the boot with: "systemctl enable mmdvmhost.timer"
      **NOTE - There is no need / desire to enable mmdvmhost.service!

