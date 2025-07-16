# Day 3

## Lab - Emulate Raspberry Pi with existing images
In case QEMU isn't installed already
```
sudo apt-get install qemu-system -y
```

Let's create a folder to maintain all the dependent files under that
```
mkdir -p ~/qemu-raspi
cd ~/qemu-raspi
```

Let's download raspberrypi os, pre-built kernel
```
cd ~/qemu-raspi
wget https://downloads.raspberrypi.org/raspbian/images/raspbian-2017-04-10/2017-04-10-raspbian-jessie.zip
wget https://github.com/dhruvvyas90/qemu-rpi-kernel/blob/master/kernel-qemu-4.4.34-jessie
unzip 2017-04-10-raspbian-jessie.zip
```

We need to locate the exact location where the OS image starts in the img disk partition
```
fdisk -l ./2017-04-10-raspbian-jessie.img
```
<pre>
jegan@tektutor.org $ fdisk -l ./2017-04-10-raspbian-jessie.img
Disk ./2017-04-10-raspbian-jessie.img: 3.99 GiB, 4285005824 bytes, 8369152 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x402e4a57

Device                            Boot Start     End Sectors  Size Id Type
./2017-04-10-raspbian-jessie.img1       8192   92159   83968   41M  c W95 FAT32 (LBA)
./2017-04-10-raspbian-jessie.img2      92160 8369151 8276992  3.9G 83 Linux  
</pre>

We are only interested in the second partition, let's do some math
<pre>
92160 * 512 = 47185920
</pre>

Let's use the above as an offset while mounting the img file
```
sudo mkdir -p /mnt/raspbian
sudo mount -v -o offset=47185920 -t ext4 ~/qemu-raspi/2017-04-10-raspbian-jessie.img /mnt/raspbian
```

Edit the /mnt/raspbian/etc/fstab 
```
sudo vim /mnt/raspbian/etc/fstab
```
If you see anything with mmcblk0 in fstab, then:

Replace the first entry containing /dev/mmcblk0p1 with /dev/sda1
Replace the second entry containing /dev/mmcblk0p2 with /dev/sda2, other lines can be commented out, save and exit.

Now we can emutate as shown below
```
qemu-system-arm \
-kernel ~/qemu-raspi/kernel-qemu-4.4.34-jessie \
-cpu arm1176 \
-m 256 \
-M versatilepb \
-serial stdio \
-append "root=/dev/sda2 \
rootfstype=ext4 rw" \
-hda ~/qemu-raspi/2017-04-10-raspbian-jessie.img \
-no-reboot \
-vga std \
-display gtk
````

## Lab - Build a custom Embedded Linux Kernel for ARM and boot with QEMU


#### Install Required Packages

```
sudo apt update
sudo apt install -y git build-essential gcc-arm-linux-gnueabi qemu-system-arm libncurses-dev bison flex libssl-dev
```

#### Download the Linux Kernel Source
```
cd ~
git clone --depth=1 https://github.com/torvalds/linux.git
cd linux
```

#### Configure the Kernel for ARM
```
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
```

#### Build the Kernel Image
```
make -j$(nproc) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- zImage
```

#### Create an init script
```
echo -e '#!/bin/sh\nmount -t proc none /proc\nmount -t sysfs none /sys\nexec /bin/busybox sh' > init
chmod +x init
```


#### Build a Simple Initramfs (Root Filesystem)
```
cd ~
mkdir initramfs
cd initramfs
mkdir -p bin sbin etc proc sys usr/bin usr/sbin dev tmp
cp ../init .
```

#### Download and build Busybox (Do this inside Initramfs folder)
```
git clone https://git.busybox.net/busybox
cd busybox
make defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- install CONFIG_PREFIX=../
cd ..
chmod u+s bin/busybox
#make CONFIG_PREFIX=./_install install
#cd ~/linux-minimal/busybox/_install
```

#### Create device nodes
```
sudo mknod -m 666 initramfs/dev/console c 5 1
sudo mknod -m 666 initramfs/dev/null c 1 3
```

#### Pack the initramfs
```
cd initramfs
find . | cpio -H newc -o --owner=root:root > ../../initramfs.cpio
```

#### Boot the Linux kernel with QEMU
```
qemu-system-arm -M versatilepb -m 128M -kernel linux/arch/arm/boot/zImage \
  -initrd initramfs.cpio -append "console=ttyAMA0 rdinit=/init" \
  -nographic
```


## Info - QEMU/KVM Performance Optimizations
<pre>
- use -enable-kvm and -cpu host 
- Allocate sufficient CPU cores and RAM to VM ( e.g -smp 8 -m 8192 )
- Disk optimizations
  - -drive file=disk.qcow2,if=virtio,cache=none,format=qcow2
  - cache=none
    - bypasses host page cache, reducing latency and improving consistency
  - use raw disks, raw disk are faster than qcow2
    -drive file=disk.img,format=raw
- Network Performance
  - User virtio-net
    - -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
      -device virtio-net-pci,netdev=net0
  - Enable Multi-Queue
    -device virtio-net-pci,netdev=net0,mq=on,vectors=10
  - Use vhost-net
    - -netdev tap,...,vhost=on
- Use Huge Pages
  - -mem-prealloc -mem-path /dev/hugepages
  - Makes sure huge pages are supported
    - echo 512 > /proc/sys/vm/nr_hugepages
    - mkdir -p /dev/hugepages
    - mount -t hugetlbfs none /dev/hugepages
- Disable unnecessary devices
</pre>

## Info - Troubleshooting common KVM-related issues
<pre>
- Problem - KVM Module Not Loaded or Missing
  Solution - run the below commands
    sudo modprobe kvm, 
    sudo modprobe kvm_intel 
  Check if modules are loaded
    lsmod | grep kvm
  If not available, ensure Virtualization is enabled in BIOS(VT-X/AMD-V)
  - Check CPU support, use the below command
       egrep -c '(vmx|svm)' /proc/cpuinfo
- Permission Denied
  Problem - Could not access KVM kernel module: Permission denied
  Solution - Add the user to kvm user group with the below command
     sudo usermod -aG kvm $USER
     newgrp kvm
     sudo chmod 666 /dev/kvm
- Peformance issues
  - qemu-system-xxx commands runs 100% on CPU
  - use virtio drivers as much as possible
  - always use enable-kvm, cpu host options when possible
  - -machine accel=kvm:tcg
- Nested Virtualization Doesn’t Work
  - Enable nested VM
    # Intel
      echo "Y" | sudo tee /sys/module/kvm_intel/parameters/nested
    # AMD
      echo "1" | sudo tee /sys/module/kvm_amd/parameters/nested
    # Check
      cat /sys/module/kvm_intel/parameters/nested
</pre>

## Lab - Multiple NICs with Shared VLAN (Legacy Style)

Overview
<pre>
- VLAN Tagging
  - VLAN tagging is a method of marking Ethernet frames with VLAN IDs to differentiate traffic 
    belonging to different Virtual Local Area Networks (VLANs) 
    even though the traffic is transmitted over the same physical or virtual link
  - Goal
    - is to logically separate networks over shared physical infrastructure 
     (switches, bridges, virtual interfaces) while enabling traffic isolation and control
- Linux bridge + VLAN subinterfaces simulate VLAN tagging
- TAP interfaces per VM allow isolated control
- QEMU connects each VM to the proper VLAN via -netdev tap
- Manual IP setup inside guests completes the scenario  
</pre>

Installing VLAN
```
sudo apt update
sudo apt install qemu bridge-utils vlan
```

Create Linux Bridge
```
# Create base bridge
sudo ip link add name br0 type bridge
sudo ip link set br0 up
```

Create VLAN Interfaces on Host
```
# VLAN 10
sudo ip link add link br0 name br0.10 type vlan id 10
sudo ip link set br0.10 up

# VLAN 20
sudo ip link add link br0 name br0.20 type vlan id 20
sudo ip link set br0.20 up
```

Create TAP Interfaces for Each VM
```
# For Guest A (VLAN 10)
sudo ip tuntap add dev tap10 mode tap
sudo ip link set tap10 master br0.10
sudo ip link set tap10 up

# For Guest B (VLAN 10)
sudo ip tuntap add dev tap11 mode tap
sudo ip link set tap11 master br0.10
sudo ip link set tap11 up

# For Guest C (VLAN 20)
sudo ip tuntap add dev tap20 mode tap
sudo ip link set tap20 master br0.20
sudo ip link set tap20 up
```

Create the Guest VMs
Guest A (VLAN 10)
```
qemu-system-x86_64 \
  -name guestA \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=guestA.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap10,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:aa:aa:aa \
  -nographic -serial mon:stdio
```

Guest B (VLAN 10)
```
qemu-system-x86_64 \
  -name guestB \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=guestB.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap11,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:bb:bb:bb \
  -nographic -serial mon:stdio
```

Guest C (VLAN 20)
```
qemu-system-x86_64 \
  -name guestC \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=guestC.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap20,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:cc:cc:cc \
  -nographic -serial mon:stdio
```

In Guest A
```
ip addr add 192.168.10.2/24 dev eth0
ip link set eth0 up
```

In Guest B
```
ip addr add 192.168.10.3/24 dev eth0
ip link set eth0 up
```

In Guest C
```
ip addr add 192.168.20.2/24 dev eth0
ip link set eth0 up
```

Test the connectivity From Guest A
```
# Should be able to ping B
ping 192.168.10.3

# Should not be able to ping C
ping 192.168.20.2 
```

Test the connectivity from Guest C
```
# Should not be able to ping Guest A
ping 192.168.10.2 
```

Cleanup
<pre>
sudo ip link delete tap10
sudo ip link delete tap11
sudo ip link delete tap20
sudo ip link delete br0.10
sudo ip link delete br0.20
sudo ip link delete br0  
</pre>


## Lab - Writing scripts to automate VM Creation

Create a directory structure as shown below
<pre>
qemu-vm-lab/
├── vms/
│   ├── guestA.img
│   ├── guestB.img
│   └── guestC.img
├── scripts/
│   ├── create_net.sh
│   ├── start_guestA.sh
│   ├── start_guestB.sh
│   ├── start_guestC.sh
│   ├── cleanup.sh
│   └── common.sh
└── logs/  
</pre>

scripts/common.sh
<pre>
#!/bin/bash

BRIDGE=br0
VLAN10=$BRIDGE.10
VLAN20=$BRIDGE.20

create_bridge() {
  sudo ip link add name $BRIDGE type bridge
  sudo ip link set $BRIDGE up
}

create_vlan_interface() {
  VLAN_ID=$1
  IFNAME=$BRIDGE.$VLAN_ID
  sudo ip link add link $BRIDGE name $IFNAME type vlan id $VLAN_ID
  sudo ip link set $IFNAME up
}

create_tap() {
  TAP=$1
  VLAN_IF=$2
  sudo ip tuntap add dev $TAP mode tap
  sudo ip link set $TAP master $VLAN_IF
  sudo ip link set $TAP up
}

delete_if_exists() {
  sudo ip link delete $1 2>/dev/null || true
}  
</pre>

create_net.sh
<pre>
#!/bin/bash
source ./scripts/common.sh

# Clean before setup 
delete_if_exists tap10
delete_if_exists tap11
delete_if_exists tap20
delete_if_exists $VLAN10
delete_if_exists $VLAN20
delete_if_exists $BRIDGE

# Build network
create_bridge
create_vlan_interface 10
create_vlan_interface 20

create_tap tap10 $VLAN10  # Guest A
create_tap tap11 $VLAN10  # Guest B
create_tap tap20 $VLAN20  # Guest C

echo "[INFO] Network setup complete"  
</pre>

VM Launch Scripts

start_guestA.sh
<pre>
#!/bin/bash
qemu-system-x86_64 \
  -name guestA \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=vms/guestA.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap10,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:aa:aa:aa \
  -nographic -serial mon:stdio | tee logs/guestA.log  
</pre>

start_guestB.sh
<pre>
#!/bin/bash
qemu-system-x86_64 \
  -name guestB \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=vms/guestB.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap11,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:bb:bb:bb \
  -nographic -serial mon:stdio | tee logs/guestB.log  
</pre>

start_guestC.sh
<pre>
#!/bin/bash
qemu-system-x86_64 \
  -name guestC \
  -m 1024 \
  -smp 2 \
  -enable-kvm \
  -drive file=vms/guestC.img,format=qcow2 \
  -netdev tap,id=net0,ifname=tap20,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:cc:cc:cc \
  -nographic -serial mon:stdio | tee logs/guestC.log  
</pre>

cleanup.sh
<pre>
#!/bin/bash
source ./scripts/common.sh

for i in tap10 tap11 tap20 $VLAN10 $VLAN20 $BRIDGE; do
  delete_if_exists $i
done

echo "[INFO] All virtual interfaces and VLANs deleted."
</pre>

Let's put them to use
```
chmod +x scripts/*.sh

# Setup network
./scripts/create_net.sh

# Start guests in different terminals
./scripts/start_guestA.sh
./scripts/start_guestB.sh
./scripts/start_guestC.sh

# Cleanup
./scripts/cleanup.sh
```


## Info - QEMU API and Custom Automation

Overview
<pre>
- QEMU provides multiple interfaces to automate, control, and extend virtual machines 
  beyond just launching them via the command line
- These interfaces include:
  1. QEMU Machine Protocol (QMP)
  2. Human Monitor Protocol (HMP)
</pre>

For example, we can enable QMP
<pre>
qemu-system-x86_64 \
  -qmp unix:/tmp/qmp-sock,server,nowait \
  ...  
</pre>

Communicate with QMP from python script
<pre>
import qmp
conn = qmp.QEMUMonitorProtocol('/tmp/qmp-sock')
conn.connect()
conn.cmd('query-status')  
</pre>


Enable 
<pre>
qemu-system-x86_64 -monitor stdio  
</pre>

In the monitor window, we can query
<pre>
info cpus
info block
device_add usb-mouse  
</pre>

You can also redirect to
<pre>
-monitor unix:/tmp/hmp-sock,server,nowait  
</pre>
