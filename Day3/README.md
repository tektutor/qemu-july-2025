# Day 3

## Lab - Build a Raspberry Pi OS Image, create a VM with QEMU emulating Raspberry Pi 

The qemu-system-arm package is necessary for emulating ARM devices like the Raspberry Pi.
```
sudo apt-get install qemu-system-arm
```

Get the Kernel Source Code: Clone the Raspberry Pi Linux kernel repository from GitHub
```
mkdir ~/raspi-kernel
cd ~/raspi-kernel
git clone https://github.com/raspberrypi/linux.git
```
#### Kernel Configuration
Choose the correct configuration
For Raspberry Pi 3 (64-bit kernel):
```
cd ~/raspi-kernel/linux
KERNEL=kernel8
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_defconfig
```

For Raspberry Pi 4 (64-bit kernel): use bcm2711_defconfig.
For Raspberry Pi 5 (64-bit kernel): use bcm2712_defconfig.
For Raspberry Pi 1 (32-bit kernel):
```
cd ~/raspi-kernel/linux
KERNEL=kernel
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcmrpi_defconfig
```
Optional: Customizing the configuration: Use make menuconfig to customize the kernel features.
```
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig  # For 64-bit kernels
```

Build the kernel image, modules, and device tree blobs (DTBs): Use the appropriate make command for your architecture (64-bit or 32-bit).
```
# For a 64-bit kernel
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs -j24

# For a 32-bit kernel
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage modules dtbs -j24
```

<pre>
Preparing the Raspberry Pi Image for QEMU
- Download and prepare a Raspberry Pi OS image: Obtain an image from the Raspberry Pi website, unzip it, and rename it.
- Modify /etc/fstab: If needed, change /dev/mmcblk0pX entries to /dev/vdaX for QEMU.
- Mount the image and enable SSH: Mount the image using its partition offset, create an empty ssh file to enable SSH on boot, and optionally set a default user and password. Remember to unmount the image afterward.
- Obtain a suitable DTB file: Find a Device Tree Blob file within the kernel source tree that describes your hardware to the kernel. 
</pre>


Booting the Raspberry Pi emulated device with QEMU
```
qemu-system-aarch64 -machine virt -cpu cortex-a72 -smp 6 -m 4G \
-kernel <path-to-your-built-kernel-Image> -append "root=/dev/vda2 rootfstype=ext4 rw panic=0 console=ttyAMA0" \
-drive format=raw,file=<path-to-your-rpi-image>.img,if=none,id=hd0,cache=writeback \
-device virtio-blk,drive=hd0,bootindex=0 \
-netdev user,id=mynet,hostfwd=tcp::2222-:22 \
-device virtio-net-pci,netdev=mynet
```

Accessing the emulated Raspberry Pi
```
ssh -l pi localhost -p 2222
```

## Lab - Emulate Raspberry Pi with an emulated Fake USB Camera

Install
```
sudo apt update
sudo apt install v4l-utils fswebcam -y
```

Setup a virtual camera on the host using v4l2loopback
```
sudo modprobe v4l2loopback devices=1 video_nr=10 card_label="FakeCam" exclusive_caps=1
```

Let's see if we are able to list the fake camera we created
```
v4l2-ctl --list-devices
```

Let's stream some looping video
```
ffmpeg -re -stream_loop -1 -i test.mp4 -f v4l2 /dev/video10
```

Find the USB deviceid
```
lsusb
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
