#!/usr/bin/env bash

# First install the configuration files:
sudo cp anbox.conf /etc/modules-load.d/
sudo cp 99-anbox.rules /lib/udev/rules.d/

# Then copy the module source to /usr/src/:
sudo cp -rT binder /usr/src/anbox-binder-1

# Finally use dkms to build and install:
sudo dkms install anbox-binder/1

# Verify by loading the module and checking the created device:
sudo modprobe binder_linux
lsmod | grep -e binder_linux
ls -alh /dev/binder

#Add symlinks to devices needed by gbinder
sudo ln -s /dev/binderfs/anbox-binder /dev/binder
sudo ln -s /dev/binderfs/anbox-hwbinder /dev/hwbinder
sudo ln -s /dev/binderfs/anbox-vndbinder /dev/vndbinder
sudo chmod 777 /dev/binder
sudo chmod 777 /dev/hwbinder
sudo chmod 777 /dev/vndbinder
