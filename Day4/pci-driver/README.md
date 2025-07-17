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
