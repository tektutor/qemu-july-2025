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
  -netdev socket,id=net0,listen=:1234 \
  -device virtio-net-pci,netdev=net0 
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
  -netdev socket,id=net0,listen=:1234 \
  -device virtio-net-pci,netdev=net0 
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
  -netdev socket,id=net0,connect=:1234 \
  -device virtio-net-pci,netdev=net0 
```

From VM1 Guest shell
```
# Assign IP address manually as we have used only a socket based network
ip a
ip addr add 10.0.0.1/24 dev ens3
ip link set ens3 up
ip a
```

From VM2 Guest shell
```
# Assign IP address manually as we have used only a socket based network
ip a
ip addr add 10.0.0.2/24 dev ens3
ip link set ens3 up
ip a
```

Test if VM1 is able to ping VM2
```
ping 10.0.0.2
```

Test if VM2 is able to ping VM1
```
ping 10.0.0.1
```

In the VM1, you need write the server.py server socket
<pre>
import socket

HOST = '0.0.0.0'
PORT = 1234

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
    server.bind((HOST, PORT))
    server.listen()

    print(f"Listing on {HOST}:{PORT} ...")

    while True:
        conn, addr = server.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                print(f"Received: {data.decode().strip()}")
                conn.sendall(data)  
</pre>

In the VM2, you need to write the client.py client socket
<pre>
import socket

HOST = '10.0.0.1'
PORT = 1234

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
    client.connect((HOST, PORT))
    print(f"Connected to server @ {HOST}:{PORT} ...")

    while True:
        message = input("Hello from client")
        if message.lower() in ('exit', 'quit'):
            print("Exiting client.")
            break
        client.sendall(message.encode())
        data = client.recv(1024)
        print(f"(Received: {data.decode().strip()}") 
</pre>


From the VM1, we need to run the server.py
```
python3 server.py
```

From the VM2, we need to run the client.py
```
python3 client.py
```

You are supposed to see the below message in server
<img width="1388" height="353" alt="image" src="https://github.com/user-attachments/assets/3676cf9f-70ce-4ed3-97c9-3543993bb33c" />


