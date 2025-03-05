# Anbox Kernel Module

This repository contains the kernel module necessary to run the Anbox
Android container runtime. They're split out of the original Anbox
repository to make packaging in various Linux distributions easier.

# Install Instruction

You need to have `dkms` and linux-headers on your system. You can install them by
`sudo apt install dkms` or `sudo yum install dkms` (`dkms` is available in epel repo
for CentOS).

Package name for linux-headers varies on different distributions, e.g.
`linux-headers-generic` (Ubuntu), `linux-headers-amd64` (Debian),
`kernel-devel` (CentOS, Fedora), `kernel-default-devel` (openSUSE).


You can either run `./INSTALL.sh` script to automate the installation steps or follow them manually below:

* First install the configuration files:

  ```
   sudo cp anbox.conf /etc/modules-load.d/
   sudo cp 99-anbox.rules /lib/udev/rules.d/
  ```

* Then copy the module sources to `/usr/src/`:

  ```
   sudo cp -rT binder /usr/src/anbox-binder-1
  ```

* Finally use `dkms` to build and install:

  ```
   sudo dkms install anbox-binder/1
  ```

You can verify by loading these module and checking the created device:

```
 sudo modprobe binder_linux
 lsmod | grep -e binder_linux
 ls -alh /dev/binder
```

You are expected to see output like:

```
binder_linux          114688  0
crw-rw-rw- 1 root root 511,  0 Jun 19 16:30 /dev/binder
```

# Uninstall Instructions

ou can either run `./UNINSTALL.sh` script to automate the installation steps or follow them manually below:

* First use dkms to remove the module:

  ```
   sudo dkms remove anbox-binder/1
  ```

* Then remove the module sources from /usr/src/:

  ```
   sudo rm -rf /usr/src/anbox-binder-1
  ```

* Finally remove the configuration files:

  ```
   sudo rm -f /etc/modules-load.d/anbox.conf
   sudo rm -f /lib/udev/rules.d/99-anbox.rules 
  ```

You must then restart your device. You can then verify module was removed by trying to load the module and checking the created device:

```
 sudo modprobe binder_linux
 lsmod | grep -e binder_linux
 ls -alh /dev/binder
```

You are expected to see output like:

```
modprobe: FATAL: Module binder_linux not found in directory /lib/modules/6.0.2-76060002-generic
ls: cannot access '/dev/binder': No such file or directory
```

# Packaging:
## Debian/Ubuntu:
```
sudo apt-get install devscripts dh-dkms -y 
git log --pretty=" -%an<%ae>:%aI - %s" > ./debian/changelog
debuild -i -us -uc -b 
ls -lrt ../anbox-modules-dkms_*.deb 
```
