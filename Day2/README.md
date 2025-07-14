# Day 2

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
vm2.qcow2 - is the new disk image created cloing the first virtual machine's disk image
vm2.qcow2 depends on vm1.qcow2
any new changes done to the the vm2.qcow2 are stored in the vm2.qcow2, all existing changes are used read-only from vm1.qcows
this helps to conservatively use the storage
Faster, but changes go to vm2 only
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

After booting the second vm, we need to change the hostname avoid conflicts and to assign unique hostname to VM1 and VM2
```
sudo hostnamectl set-hostname vm2
```
