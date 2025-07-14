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
  
vm2.qcow2 - is the new disk image created cloning the first virtual machine's disk image
  
vm2.qcow2 depends on vm1.qcow2
  
any new changes done to the the vm2.qcow2 are stored in the vm2.qcow2, all existing changes are 
used read-only from vm1.qcow
  
this helps to conservatively use the storage
  
Faster, changes done in vm2 only goes to vm2

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

