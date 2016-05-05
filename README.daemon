On Linux, to run MMDVMHost as a daemon, set "Daemon=1" in the ini file. 

When this is set, MMDVMHost will attempt to drop privileges to user "mmdvm" and 
group "mmdvm". If this user and group do not exist on your system, an error 
will occur and 
MMDVMHost will not start. 

To add these users, please do the following from the Linux command line:

groupadd mmdvm
useradd mmdvm -g mmdvm -s /sbin/nologin
usermod mmdvm -G dialout

Note, without the last line, MMDVMHost will not be able to open the modem. 

Also note, when running as a daemon, STDIN, STDOUT and STDERR are closed, so 
you must use a logfile to capture logging and the logfile entry in the ini file 
must be given a full path as MMDVMHost calls "cd /" when daemonising. The same 
applies to the DMRIds.dat file. 

Also, please note that the code to drop privileges is currently disabled when 
MMDVMHost is compiled with the HD44780 display as it's currently not possible 
to use this display as a non-root user.


Simon - G7RZU