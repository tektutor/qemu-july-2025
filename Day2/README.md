# Day 2

## Info - What is the difference between normal OS copy command vs qemu-img clone ( convert and create )
<pre>
- Normal copy command doesn’t preserve sparse files efficiently, uses more disk
- Normal copy command is not QEMU format-aware, might break if metadata is modified
- Normal copy command doesn't support conversion of one image format to other
- Normal copy command doesn't support any compression 
- Normal copy command won’t detect or prevent inconsistencies in the disk format  
</pre>  

## Lab - Cloning a QEMU VM to create another Virtual machine

Info
<pre>
- in order to clone an existing VM1 to create a second VM2, we need to copy the disk image
</pre>

Create a partial clone
```
qemu-img create -f qcow2 -b vm1.qcow2 vm2.qcow2
```

In the above command
<pre>
-b creates a copy-on-write clone: vm2.qcow2 depends on vm1.qcow2
  
vm1.qcow2 - is the disk image used in existing virtual machine 1
  
vm2.qcow2 - is the new disk image created cloning the first virtual machine's disk image
  
vm2.qcow2 depends on vm1.qcow2
  
any new changes done to the the vm2.qcow2 are stored in the vm2.qcow2, all existing changes are 
used read-only from vm1.qcow
  
this helps to conservatively use the storage
  
Faster and space saving, changes done in vm2 only goes to vm2

Drawback is, we won't be able to delete the VM1 disk if you wish to delete VM1 entirely while retaining VM2.
</pre>

If you prefer a full-clone
```
qemu-img convert -f qcow2 -O qcow2 vm1.qcow2 vm2.qcow2
```
In this case, vm2.qcow2 is an independent disk image 

Launch the cloned VM2
```
qemu-system-x86_64 -hda vm2.qcow2 \
-m 1024 \
-net user \
-net nic,macaddr=52:54:00:12:34:56 
```

In the above command, we need to change the mac address of the VM2 network card to avoid conflicts between VM1 and VM2.
If suppose the macaddr of first vm1 is 52:54:00:33:44:55, we could just change the last 3 numbers as shown in the command above. We just need to ensure, they aren't same.

After booting the second vm, we need to change the hostname avoid conflicts and to assign unique hostname to VM1 and VM2
```
sudo hostnamectl set-hostname vm2
```

## Info - Types of QEMU Networks
<pre>
- QEMU supports the following types of Networks
  1. User mode
     - this is the default type of network used in QEMU VMs when we don't mention any network options
     - host machine won't be able to access the VM's IP address
     - optionally, explicit host port forwarding can be done to expose the machine's services to external world
     - VM can access Internet
     - Other VMs on the same machine can't acces each other
     - Regular users(non-admins) will be able to create this user-mode networking 
  2. Tap
     - Host machine will be able to access the VM's IP address
     - VM can be given access to Internet via bridge
     - VM to VM communication is possible
     - Only an administrator can create this kind of network
  3. Socket
     - Connects the VMs directly using unix sockets
     - Host machine won't be able to connect to the VMs
     - Virtual Machines won't get Internet access
     - VM to VM communicaiton is possible
     - Only an administrator can create this kind of network
  4. Bridge
     - attaches the VM to physical ethernet bridge
     - Host machine can access the VMs
     - Virtual machines will get Internet access
     - VM to VM communication is possible
     - Only an administrator can create this kind of network
     - The virtual machine gets an IP address from the same subnet the host machine is connect to
  5. SLIRP
     - is a replacement for user mode networking 
     - host machine can't access the VMs
     - gives internet access to VM
     - VM to VM communicaiton is not possible
     - Non-admin regular users also can create this kind of network
  6. VDE ( Virtual Distributed Ethernet - not widely used, you won't require this type of network for your usecases )
  7. Multiqueue ( High Performance )
</pre>

## Lab - VM1 to VM2 communication via Socket

Download the alpine linux iso
```
mkdir ~/qemu
cd ~/qemu
wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-standard-3.20.0-x86_64.iso
```

Create the first VM using with alpine cloud image live boot (Terminal 1)
```
qemu-system-x86_64 \
  -m 512 \
  -enable-kvm \
  -cdrom alpine-standard-3.20.0-x86_64.iso \
  -boot d \
  -netdev socket,id=net0,listen=:1234 \
  -device virtio-net-pci,netdev=net0 \
  -nographic
```

Create the first VM using with alpine cloud image live boot (Terminal 2)
```
qemu-system-x86_64 \
  -m 512 \
  -enable-kvm \
  -cdrom alpine-standard-3.20.0-x86_64.iso \
  -boot d \
  -netdev socket,id=net0,connect=127.0.0.1:1234 \
  -device virtio-net-pci,netdev=net0 \
  -nographic
```

In VM1 terminal
```
ip a
ip addr add 10.0.0.1/24 dev eth0
ip link set eth0 up
ip a
```

In VM2 terminal
```
ip a
ip addr add 10.0.0.2/24 dev eth0
ip link set eth0 up
ip a
```

Ping VM2 IP address from VM1 terminal
```
ping 10.0.0.2
```

Ping VM1 IP address from VM2 terminal
```
ping 10.0.0.1
```

Let's test the socket communicaiton between VM1 and VM2, on VM1 let's start the socket server
```
nc -l -p 12345
```

From VM2, let's connect to socket server
```
echo "hello from vm2" | nc 10.0.0.2 12345
```

Once you are done with the VMs, let's poweroff both VMs

In VM1 terminal
```
poweroff
```


In VM2 terminal
```
poweroff
```

## Lab - VM1 to VM2 communication over Tap device
Note
```
- Without a bridge
  - each TAP interface is isolated
  - VMs wouldn’t be able to talk to each other unless you manually set up routing or forwarding
```

Let's create the tap device one per VM
```
# Create tap interfaces
sudo ip tuntap add tap0 mode tap user $(whoami)
sudo ip tuntap add tap1 mode tap user $(whoami)

# Bring them up
sudo ip link set tap0 up
sudo ip link set tap1 up

sudo ip link add name br0 type bridge
# Add TAP interfaces to bridge
sudo ip link set tap0 master br0
sudo ip link set tap1 master br0
sudo ip link set br0 up
```

Start VM1
```
qemu-system-x86_64 \
  -m 512 \
  -enable-kvm \
  -cdrom alpine-standard-3.20.0-x86_64.iso \
  -boot d \
  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
  -device e1000,netdev=net0,mac=52:54:00:00:00:01 \
  -nographic
```
In the above command
<pre>
script=no
- Prevents QEMU from trying to run the default up script ( qemu-ifup ) to configure the TAP 
  interface (tap0) when starting the VM
- This means we must manually configure tap0 before starting QEMU

downscript=no
- Prevents QEMU from running a down script (qemu-ifdown) to tear down the TAP interface 
  when the VM shuts down
- So we keep control of the interface lifecycle 
</pre>


Start VM2
```
qemu-system-x86_64 \
  -m 512 \
  -enable-kvm \
  -cdrom alpine-standard-3.20.0-x86_64.iso \
  -boot d \
  -netdev tap,id=net1,ifname=tap1,script=no,downscript=no \
  -device e1000,netdev=net1,mac=52:54:00:00:00:02 \
  -nographic
```

On the VM1 terminal, assign IP address to the VM manually
```
sudo ip addr add 192.168.100.1/24 dev eth0
sudo ip link set eth0 up
```

On the VM2 terminal, assign IP address to the VM manually
```
sudo ip addr add 192.168.100.2/24 dev eth0
sudo ip link set eth0 up
```

Test the connectivity, ping VM2 from VM1
```
ping 192.168.100.2
```

Test the connectivity, ping VM1 from VM2
```
ping 192.168.100.1
```

Cleanup on the local machine
```
sudo ip link set tap0 nomaster
sudo ip link set tap1 nomaster

sudo ip link set tap0 down
sudo ip link set tap1 down

sudo ip tuntap del mode tap dev tap0
sudo ip tuntap del mode tap dev tap1

sudo ip link set br0 down
sudo ip link delete br0 type bridge

# Verify
ip link show | grep br
```

## Lab - Build a minimal linux with custom kernel

Linux kernel build officially only supports upto Ubuntu 22.04, hence building from ubuntu latest versions we will report build errors, hence let's create a VM with ubuntu 22.04

Download Ubuntu 22.04
```
wget https://releases.ubuntu.com/jammy/ubuntu-22.04.5-live-server-amd64.iso
```

You need to create folder in your host machine
```
mkdir -p ~/qemu-share
```


Let's create a VM with ubuntu-22.04
```
qemu-system-x86_64 \
  -enable-kvm \
  -cpu host \
  -m 4096 \
  -smp 4 \
  -drive file=ubuntu1.qcow2,format=qcow2 \
  -boot c \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -device e1000,netdev=net0 \
  -serial mon:stdio \
-nographic \
-fsdev local,id=fsdev0,path=$HOME/qemu-share,security_model=none \
-device virtio-9p-pci,fsdev=fsdev0,mount_tag=hostshare
```

Inside the Guest VM Shell, you need to run this
```
sudo mount -t 9p -o trans=virtio hostshare /mnt/build
```


All the below command we need to perform within the ubuntu 22.04 vm

Install required softwares on the Guest linux machine 
```
sudo apt update
sudo apt install net-tools tree iputils-ping build-essential libncurses-dev flex bison libssl-dev libelf-dev qemu-system-x86 busybox -y
```

Instead of above step, you could also download and Compile Linux Kernel
```
mkdir ~/linux-minimal && cd ~/linux-minimal
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.tar.xz
tar -xf linux-6.9.tar.xz && cd linux-6.9
```

Configure the Kernel
```
make defconfig   # Generates a default config
make menuconfig  # Optional: for customizing features (e.g. disable modules, enable early printk)
```

Build the Kernel
```
make -j$(nproc)
```

Create a minimal root file system

Download and build busybox. In menuconfig, enable Build BusyBox as a static binary under Settings

```
cd ~/linux-minimal
git clone https://git.busybox.net/busybox
cd busybox
make defconfig
make menuconfig   # Enable "Build static binary"
make -j$(nproc)
make CONFIG_PREFIX=./_install install
cd ~/linux-minimal/busybox/_install
```

Create a new /init script under ~/linux-minimal/busybox/_install folder
```
cat > init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t tmpfs tmpfs /tmp

printf "\033[1;32mWelcome to our custom BusyBox OS !\033[0m\n"
echo -e "\033[36mHit enter button to access the shell\033[0m"

printf "\033[1;31mYou may use poweroff -f, once you are done ...\033[0m\n"

# Shell with job control
exec setsid /bin/sh

echo "Attempting to shut down..."

# Try standard poweroff
poweroff -f

# If that fails, try sysrq-trigger
echo o > /proc/sysrq-trigger

# If that fails, try QEMU isa-debug-exit port
printf '\x01' > /dev/port

# If all else fails, just hang
echo "Could not power off. Halting."
while :; do sleep 1; done
EOF
chmod +x init
```

Create device nodes
```
cd ~/linux-minimal/busybox/_install
sudo mknod -m 622 dev/console c 5 1
sudo mknod -m 666 dev/null c 1 3
```

Pack Root Filesystem into Initramfs
```
cd ~/linux-minimal/busybox/_install
find . | cpio -H newc -o --owner=root:root > ../../initramfs.cpio
```

Copy the compiled binaris
```
cp ~/linux-minimal/linux-6.9/arch/x86/boot/bzImage /mnt/build
cp ~/linux-minimal/initramfs.cpio /mnt/build
```

From the host machine, Boot with QEMU
```
cd /home/jegan/qemu-share
qemu-system-x86_64 -kernel bzImage \
  -initrd initramfs.cpio \
  -nographic -append "console=ttyS0"
```

Expected output
```
Welcome to our custom BusyBox OS !
Hit enter button to access the shell
You may use poweroff -f, once you are done ...
```
