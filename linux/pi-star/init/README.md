MMDVMHost Init setup
=======================

Preface
-------

Init script setup for Linux systems using the older Init setup (not Systemd)

I hope this helps you get going quickly.
Written on  / tested on / confirmed working on Raspbian Wheezy but should work just fine on any  
system using init scripts.

Enjoy, 73 de MW0MWZ


Install Instructions
--------------------

  1. Copy mmdvmhost to /etc/init.d
  2. Change the permisions of /etc/init.d/mmdvmhost to 550 (root executable)
  3. Change the variables in /etc/init.d/mmdvmhost to match your setup  
      (MMDVMHost.ini location, log location, user and daemon location)
  4. Enable the service "update-rc.d mmdvmhost defaults" on Rasbian Wheezy

