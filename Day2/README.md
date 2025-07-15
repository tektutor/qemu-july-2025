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



