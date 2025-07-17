## Lab - Creating a simple Linux Kernel Mode PCI Device Driver

In order to build this Linux driver, you need a guest VM

Create a disk
```
qemu-img create -f qcow2 ubuntu1.qcow2 20G
```

Need to install Ubuntu first
```
```
qemu-system-x86_64 \
  -m 8192 \
  -enable-kvm \
  -cpu host \
  -cdrom ubuntu-22.04.5-live-server-amd64.iso  
  -boot d \
  -drive file=ubuntu1.qcow2,format=qcow2 \
  -smp 4 \
  -net user,hostfwd=tcp::2222-:22 \
  -net nic \
  -nographic \
  -device ed
```
Once you are done with the ubuntu installation, quit that and launch the VM as shown below.


Then we can boot the Guest VM as shown below
```
qemu-system-x86_64 \
  -m 8192 \
  -enable-kvm \
  -cpu host \
  -boot c \
  -drive file=ubuntu1.qcow2,format=qcow2 \
  -smp 4 \
  -net user,hostfwd=tcp::2222-:22 \
  -net nic \
  -nographic \
  -device ed
```


# In guest machine
```
cd ~
git clone https://github.com/tektutor/qemu-july-2025.git
cd qemu-july-2025/Day4/pci-driver
```

# Build kernel module
```
make -C /lib/modules/$(uname -r)/build M=$PWD/kernel modules
```

# Insert module ( To install the driver )
```
sudo insmod kernel/simplepci-driver.ko
```

# Check dmesg
```
sudo dmesg | tail
```

# Compile user apps
```
gcc -o producer user/producer.c
gcc -o consumer user/consumer.c
```

# Write from producer
```
sudo ./producer
```

# Read from consumer
```
sudo ./consumer
```

# To delete the driver
```
lsmod | grep simplepci
sudo rm -f /dev/simplepci
sudo dmesg | tail
```
