# Day 4

## Lab - Socket Communication between 2 QEMU VMs

Let's create two disk images
```
qemu-img create -f qcow2 ubuntu1.qcow2 10G
qemu-img create -f qcow2 ubuntu2.qcow2 10G
```

Let's create our first Guest VM1 
```
qemu-system-x86_64 \
  -m 8192 \
  -enable-kvm \
  -cpu host \
  -cdrom ubuntu-22.04.5-live-server-amd64.iso  \
  -boot d \
  -drive file=ubuntu1.qcow2,format=qcow2 \
  -smp 4 \
  -netdev user,id=net0,net=10.0.2.15/24, dhcpstart=10.0.2.15/24 hostfwd=tcp::2222-:22 \
  -device e1000,netdev=net0 \
  -netdev socket,id=net1,listen=:1234 \
  -device virtio-net-pci,netdev=net1 
```

Boot into the first Guest vm1
```

qemu-system-x86_64 \
  -m 8192 \
  -enable-kvm \
  -cpu host \
  -boot c \
  -drive file=ubuntu1.qcow2,format=qcow2 \
  -smp 4 \
  -netdev user,id=net0,net=10.0.2.15/24, dhcpstart=10.0.2.15/24 hostfwd=tcp::2222-:22 \
  -device e1000,netdev=net0 \
  -netdev socket,id=net1,listen=:1234 \
  -device virtio-net-pci,netdev=net1 
```

Let's clone the first VM1 disk to create the second VM2 disk
```
qemu-img convert -O qcow2 ubuntu1.qcow2 ubuntu2.qcow2
```

Let's create our second Guest VM2 
```
qemu-system-x86_64 \
  -m 8192 \
  -enable-kvm \
  -cpu host \
  -boot c \
  -drive file=ubuntu2.qcow2,format=qcow2 \
  -smp 4 \
  -netdev user,id=net2,net=10.0.2.16/24, dhcpstart=10.0.2.16/24 hostfwd=tcp::3333-:22 \
  -device e1000,netdev=net2 \
  -netdev socket,id=net3,listen=:1234 \
  -device virtio-net-pci,netdev=net3 
```

From VM2 Guest shell
```
# Change the hostname from vm1 to vm2
sudo hostnamectl set-hostname vm2

# Assign a different IP
sudo ip addr flush dev ens3
sudo ip addr add 10.0.2.16/24 dev ens3
sudo ip link set dev ens3 up
sudo ip route add default via 10.0.2.1

ifconfig
# you are supposed to 10.0.2.16
```
