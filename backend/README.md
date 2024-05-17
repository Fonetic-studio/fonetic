# Fonetic Pi device

## Setting up the device
Install the Raspberry Pi with Raspbian bookworm 64bit and user pi. 
Login as pi, /home/pi is the location where alle files must be installed.

### Install packages
sudo apt update
sudo apt install git gcc alsa-devel libasound2 libasound2-devel libboost-all-dev

### Install WiringPi
```
git clone https://github.com/WiringPi/WiringPi
cd WiringPi
sudo ./build
```

### Clone the fonetic repository
```
git clone xxxx
```

### Create the listen binary
```
make
```

### Install the fonetic services
```
sudo cp *.service /etc/systemd/system
systemctl enable fonetic_startup 
systemctl enable fonetic_listen
systemctl enable fonetic_listen
systemctl enable fonetic_scanq
```

### Install environment variables in /etc/profile
```
cat profile >> /etc/profile
```

### The following environment variables in /etc/profile need to be adjusted depending on requirements.
Then reboot the device.

FONETIC_GAIN
Set gain. The gain varies per device, amixer shows that.

FONETIC_DEVICE
Name of the input device. E.g. hw:Device
Use arecord -L to list devices.

FONETIC_CHANNELS
Number of channels, typically 2

FONETIC_GATE
Threshold sample value when the gate opens and spikes are counted. If a number of spikes are reached within the current buffer, recording starts.

FONETIC_SPIKES
The number of spikes needed to start recording

FONETIC_BUFFER
Buffer size, number of samples scanned for spikes.

## Fonetic services

### fonetic_startup
runs the shell-script ~/pi/Fonetic/startup

### fonetic_listen 
Listens and creates .raw files in ~/Fonetic/queue/in for each recording.
If LED is solid red at startup, the hardware settings from startup are wrong!
Green LED means listening, and then red LED means recording.

### fonetic_scanq
Scans ~/Fonetic/queue/in and converts .raw to .mp3 ~/Fonetic/queue/out and calls the transscribe function to local.noties.nl.
Red LED means no connection to local.noties.nl

A wireless connection can be set up with nmtui
