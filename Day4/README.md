# Day 4

## Lab - Emulate Raspberry Pi with an emulated Fake USB Camera (Incomplete)

Install
```
sudo apt update
sudo apt install v4l-utils fswebcam -y
```

Setup a virtual camera on the host using v4l2loopback
```
sudo modprobe v4l2loopback devices=1 video_nr=10 card_label="FakeCam" exclusive_caps=1
```

Let's see if we are able to list the fake camera we created
```
v4l2-ctl --list-devices
```

Let's stream some looping video
```
ffmpeg -re -stream_loop -1 -i test.mp4 -f v4l2 /dev/video10
```

Find the USB deviceid
```
lsusb
```
