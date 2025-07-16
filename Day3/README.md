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
  Fix 
  '''
  sudo modprobe kvm
  sudo modprobe kvm_intel  
  '''
</pre>
