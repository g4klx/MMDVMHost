# Linux Scripts

This directory (and its sub-directories) contain various third-party Linux shell scripts that have been written to provide certain system administration functions.

These are:

### DMRIDUpdate.sh (and DMRIDUpdateBM.sh)

Updates the DMRIds.dat file periodically from a cron job.
The only difference between the two scripts is the source of the data.

### tg_shutdown.sh

Automated shutdown on receipt of a specific talkgroup.
The script relies on at least a file loglevel of 2.

# Daemon Startup Scripts

In the subfolders here you will find start scripts for running MMDVMHost as a daemon on Linux systems using either  
systemd or init for their boot.

In both cases the scripts are fully functional providing the usual start / stop / status etc.

These have been writting specifically for Raspbian Wheezy (init) and Rasbian Jessie (systemd) although there is  
no reason that they shouldnt work on many other distributions.
