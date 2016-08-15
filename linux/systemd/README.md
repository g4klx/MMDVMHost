



Copy mmdvmhost.service to  /lib/systemd/system
Copy mmdvmhost.timer to    /lib/systemd/system
Edit the timeout in mmdvmhost.timer to suit - 45 secs is a reasonable starting point.
Reload systemctl with: "systemctl daemon-reload"
Add the timer serice to the boot with: "systemctl enable mmdvmhost.timer"
**NOTE - There is no need / desire to enable mmdvmhost.service!
