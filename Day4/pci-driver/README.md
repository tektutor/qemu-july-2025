# In guest machine
cd simplepci-driver

# Build kernel module
make -C /lib/modules/$(uname -r)/build M=$PWD/kernel modules

# Insert module
sudo insmod kernel/simplepci_driver.ko

# Check dmesg
sudo dmesg | tail

# Compile user apps
gcc -o producer user/producer.c
gcc -o consumer user/consumer.c

# Write from producer
sudo ./producer

# Read from consumer
sudo ./consumer

# To delete the driver
lsmod | grep simplepci
sudo rm -f /dev/simplepci
sudo dmesg | tail
