#!/usr/bin/env bash

# Important: The script should check if DKMS is Present, because its important so no one would cry at issues
CHECK=$(command -v dkms 2>/dev/null)
if [ -n "$CHECK" ]
  then 
  echo "DKMS Present, Istalling Anbox Modules"
  else
  echo "DKMS Did not respond, it seems that DKMS is not Present or you are not running in sudo/root mode"
  echo "To make sure, please install dkms and try running the script again with sudo"
  exit 1
fi
unset CHECK

# Start the Operation! : install the configuration files:
sudo cp anbox.conf /etc/modules-load.d/
sudo cp 99-anbox.rules /lib/udev/rules.d/

# Then copy the module sources to /usr/src/:
sudo cp -rT ashmem /usr/src/anbox-ashmem-1
sudo cp -rT binder /usr/src/anbox-binder-1

# Finally use dkms to build and install:
sudo dkms install anbox-ashmem/1
sudo dkms install anbox-binder/1

# Verify by loading these modules and checking the created devices:
sudo modprobe ashmem_linux
sudo modprobe binder_linux
lsmod | grep -e ashmem_linux -e binder_linux
ls -alh /dev/binder /dev/ashmem
