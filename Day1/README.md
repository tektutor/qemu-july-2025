# Day 1

## Info - Hypervisor
<pre>
- is a virtualization technology
- it is a Hardware + Software Technology
- there are 2 types
  1. Type 1
  - Bare metal Hypervisor
  - used in Servers & Workstations
  - Doesn't require a Host OS
  - Examples
    - VMWare vSphere/vCenter ( Commercial Product - requires paid license )
    - KVM ( opensource )
      - can be installed on top of some linux distros ( Ubuntu, Fedora, RHEL, etc )
  2. Type 2
  - is installed on top of a Host OS ( Windows, Linux or Mac OS-X )
  - used in Desktops, Laptops or Workstations
  - Examples
    - Oracle VirtualBox - Free ( Windows, Linux & Mac )
    - VMWare Workstation ( Windows & Linux )
    - VMWare Fusion ( Mac OS-X - Paid )
    - Parallels ( Mac OS-X - Paid )
  - each Virtual Machine must be allocated with dedicated hardware resources
    - each 
</pre>

## Lab - Installing QEMU in Ubuntu
```
sudo apt update
sudo apt install -y qemu-kvm qemu-system qemu-utils gcc make libvirt-daemon-system libvirt-clients bridge-utils virt-manager
sudo usermod -aG libvirt $(whoami)
newgrp libvirt
egrep -c '(vmx|svm)' /proc/cpuinfo
virsh list --all
qemu-system-x86_64 --version

sudo apt update
sudo apt-get install qemu-system
sudo apt install qemu-efi-arm -y
sudo apt install qemu-system-arm qemu-efi qemu-efi-aarch64 qemu-utils -y
sudo apt install qemu-system-arm qemu-system-aarch64 -y
wget https://github.com/raspberrypi/firmware/blob/master/boot/kernel8.img
sudo modprobe kvm
lsmod | grep kvm
```

## Info - QEMU Overview
<pre>
- QEMU is a powerful open-source machine emulator and virtualizer
- QEMU is incredibly powerful because it supports a wide range of CPU architectures and platforms, both for emulation and virtualization
- QEMU supports 3 modes
  - Full System Emulation mode
    - Use Cases:
      - Running a full guest OS for a different architecture (e.g., ARM on x86)
      - Testing firmware, bootloaders, or custom hardware setups
      - Embedded system development
  - User Mode Emulation mode
  - KVM Virtualizaiton Mode
- What it can and can't do depends largely on how it's used—whether in full system emulation or user-mode emulation
- Emulate entire hardware systems (including CPU and devices)
- Virtualize guest operating systems using KVM for near-native performance (on x86/x86_64 and other platforms)
- Run code for one CPU architecture on a different one (e.g., ARM on x86). Slower but very flexible
- Run guests using the same architecture as the host using KVM. Very fast.
- Emulates guest CPU (if needed)
- Emulates disk, network, USB, graphics, etc.
- Can boot VMs like physical machines (BIOS/UEFI)
- Virtual hard drive file (e.g., .qcow2, .raw) for 
- Supports user-mode, TAP, bridge, socket, etc for Network
- Without KVM, QEMU falls back to software emulation, which is much slower.
- Common Usecases
  - Running Linux, Windows, or BSD VMs on Linux
  - Emulating different architectures (e.g., ARM, MIPS, PowerPC)
  - Building and testing OS kernels and drivers
  - Network simulation (routers, firewalls, switches)
  - Embedded systems development
  - CI/testing pipelines in isolated environments
</pre>

## Info - QCOW2 vs Raw Disk Format
<pre>
|-----------------------------------------------------------|  
| QCOW2 Disk Format         | RAW Disk Format               |
|-----------------------------------------------------------|
| QEMU Copy ON Write        | Unstructured binary image     |
| Slower                    | Faster                        |
| Sparse - Grows as needed  | Allocates full size upfront   |
| Compression supported     | Compression not supported     |
| Encryption supported      | Encryption not supported      |
| Suppoted only by QEMU/KVM | Supported by other VMs as well|
| Can be converted to raw   | Can be converted to QCOW2     |
|-----------------------------------------------------------|
</pre>

## Info - VMDK Disk Format
<pre>
- VMDK (Virtual Machine Disk) is a virtual disk file format developed by VMware 
- Useful for interoperability with VMware tools like ESXi or Workstation
- It's used to store the contents of a virtual machine’s hard disk — essentially 
- it's a virtual hard drive stored as a file
- VMware Workstation, ESXi, VirtualBox (also supports it), and QEMU (via qemu-img)
- Can be a single file or split into multiple 2GB chunks
- Stores VM operating system, files, apps — like a physical disk
- Types of VMDK Files
  - Monolithic Flat
    - A single file for entire virtual disk. Large and simple
  - Split (Sparse)
    - Disk split into 2GB files. Easier to move across filesystems
  - Thin Provisioned
    - Allocates disk space on demand (saves storage)
  - Thick Provisioned
    - All disk space allocated up front (better performance)
- Common Usecases
  - Run VMs in VMware Workstation or VMware ESXi
  - Import/export virtual disks between platforms (e.g., VMware ↔ VirtualBox)
  - Convert VMDK to/from other formats (e.g., for use in QEMU, KVM, VirtualBox, etc.)
</pre>

## Info - When to use RAW Format?
<pre>
- You need maximum performance
- You’re deploying to production or running I/O-heavy workloads
- You want easier compatibility with other virtualization platforms (e.g., VirtualBox, VMware)  
- Example
  - High-performance server VMs, cloud images, or passthrough devices.
</pre> 

## Info - When to use QCOW2 Format?
<pre>
- You want snapshots, backing files, or smaller initial disk space usage
- You’re testing/developing and want flexibility
- Disk space is limited or shared across multiple VMs (via copy-on-write)
- Example
  - Dev/testing environments, personal labs, or when using virt-manager  
</pre>  

## Info - How to choose between different formats?
<pre>
- Use qcow2 for flexibility (snapshots, dynamic size, etc.)
- Use raw for maximum performance or when integrating with non-QEMU tools
- Use other formats (VMDK, VDI, VHD) when exchanging VMs with other platforms
</pre>

## Info - Emulation vs Virtualization
<pre>
- These are two core concepts in system virtualization
- QEMU can do both
- But they serve different purposes 
  - have major differences in performance, compatibility, and use cases
- Emulation
  - Imitates different hardware or architecture in software
  - You can run code compiled for one platform (e.g., ARM) on another (e.g., x86)
- Virtualization
  - Runs code natively on the host CPU, using hardware support (like Intel VT-x, AMD-V)
  - Only works when host and guest share the same architecture
</pre>

## Info - Understanding QEMU command-line options
<pre>
Understanding QEMU’s command-line options is key to configuring and running virtual machines exactly how you need them — whether for emulation, virtualization, or advanced networking.  
</pre>

Common Switches
<pre>
m 2048 
- allocates 2048 MB of RAM
smp 2
- 2 Virtual CPUs
hda file.qcow2
- Use this as primary hard disk
cdrom file.iso
- Attach ISO as CD-ROM
boot d	
- Boot from CD-ROM
boot c
- Boot from Hard disk
boot a
- Boot from Floppy Drive
boot n
- Boot from Network (PXE)
enable-kvm
- Enables KVM Hardware Acceleration 
cpu host
- Use Host CPU model in guest(faster)
nographic
- Disables graphical windows (use console only)
vnc :1
- Enables VNC Server on display :1
display gtk
- Use GTK GUI
display none
- Runs headless(no display)
netdev user,id=net0
- User user-mode NAT ( easy but limited )
device e1000,netdev=net0
- Attach an Intel e1000 NIC to net0
netdev tap,id=net1,ifname=tap1,scripti=no
- Use a TAP interface for full control
drive file=disk.qcow2,format=qcow2,if=virtio	
- Adds disk using virtio(faster)
snapshot
- Run VM without saving changes
drive file=usb.img,if=none,id=usbdev
- Defines drive for USB passthrough
device usb=storage,drive=usbdev
- Attaches above drive as USB device
machine type=pc,accel=kvm
- Set machine type and accelerator
device virtio-net-pci
- Add virtio network device
device usb-ehci
- Add USB 2.0 Controller
rtc base=localtime
- Set real-time clock to local time(Windows VMs)
serial stdio
- Use your terminal as VM serial console
S -s
- Pause VM at startup and open GDB stub on port 1234
monitor stdio
- Enable interactive QEMU monitor in terminal
d guest-errors
- Enable debug output for guest errors
</pre>

## Lab - Create a VM using qcow2 virtual disk
Create a Virtual Disk
```
qemu-img create -f qcow2 ubuntu_vm.qcow2 20G
```

Now let's install Ubuntu in the qcow2 Virtual Disk using ubuntu ISO
```
qemu-system-x86_64 \
  -m 4G \
  -cdrom ubuntu-24.04.2-live-server-amd64.iso \
  -drive file=ubuntu_vm.qcow2,format=qcow2 \
  -boot d \
  -enable-kvm \
  -cpu host \
  -smp 2
```

Boot the VM 
```
qemu-system-x86_64 \
  -m 4G \
  -drive file=ubuntu_vm.qcow2,format=qcow2 \
  -enable-kvm \
  -cpu host \
  -smp 2 
```

Install ifconfig and ping command with the vm
```
sudo apt update && sudo apt install net-tools iputils-ping -y
```

To check if the SSH Server is running on the VM
```
sudo systemctl status ssh
sudo systemctl enable ssh # Creates a service for SSH also ensure the service runs once the machine is rebooted
sudo systemctl start ssh
sudo systemctl status ssh
sudo ufw status # Whether the firewall is active or inactive
sudo ufw start
sudo ufw allow 22/tcp  # We are openning up the SSH port
sudo ufw list
```

List running QEMU VMs from different terminal
```
pgrep -a qemu
```

Shutdown the VM and delete the disk
```
init 0
## kill 31827 or kill -9 31827
rm ubuntu_vm.qcow2
# lsof -p 31827 | grep qcow2
```
