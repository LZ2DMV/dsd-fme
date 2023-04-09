## How to clone and build this branch

### Ubuntu 22.04/20.04/LM20/Debian Bullseye or Newer:

Using the included download-and-install.sh should make for a simple and painless clone, build, and install on newer Debian/Ubuntu/Mint/Pi systems. Simply acquire or copy the script, and run it. Update: Ubuntu 22.04 and RPi Bullseye 64-bit has been tested working with the installer script and functions appropriately.

If you need all dependencies build and installed first (only on Debian/Ubuntu/Mint/Pi), run:

```
wget https://raw.githubusercontent.com/lwvmobile/dsd-fme/main/download-and-install.sh
chmod +x download-and-install.sh
./download-and-install.sh
```

If you have dependencies already installed (i.e. need a fresh clean install on a system with DSD-FME already or using system other than Debian/Ubuntu, etc), please run this instead:

```
wget https://raw.githubusercontent.com/lwvmobile/dsd-fme/main/download-and-install-nodeps.sh
chmod +x download-and-install-nodeps.sh
./download-and-install-nodeps.sh
```

### Ubuntu 18.04/LM19/Buster Note:
The above install.sh should now function on older system types. You can elect to manually follow the steps down below. Do NOT Manually build and install ITPP 4.3.1 on older systems, it is currently not wanting to build on Ubuntu 18.04 and Linux Mint 19. Install it from the repository instead.

## Manual Install

First, install dependency packages. This guide will assume you are using Debian/Ubuntu based distros. Check your package manager for equivalent packages if different.

```
sudo apt update
sudo apt install libpulse-dev pavucontrol libsndfile1-dev libfftw3-dev liblapack-dev socat libusb-1.0-0-dev libncurses5 libncurses5-dev rtl-sdr librtlsdr-dev libusb-1.0-0-dev cmake git wget make build-essential libitpp-dev libncursesw5-dev
```
## Headless

If running headless, swap out pavucontrol for pulsemixer, and also install pulseaudio as well. Attempting to install pavucontrol in a headless environment may attempt to install a minimal desktop environment. Note: Default behavior of pulseaudio in a headless environment may be to be muted, so check by opening pulsemixer and unmuting and routing audio appropriately.

```
sudo apt install libpulse-dev libsndfile1-dev libfftw3-dev liblapack-dev socat libusb-1.0-0-dev libncurses5 libncurses5-dev rtl-sdr librtlsdr-dev libusb-1.0-0-dev cmake git wget make build-essential libitpp-dev libncursesw5-dev pulsemixer pulseaudio
```

### Build and Install ITPP - ONLY IF NOT IN REPO!!

```
wget -O itpp-latest.tar.bz2 http://sourceforge.net/projects/itpp/files/latest/download?source=files
tar xjf itpp*
#if you can't cd into this folder, double check folder name first
cd itpp-4.3.1
mkdir build
cd build
cmake ..
make -j `nproc`
sudo make install
sudo ldconfig
cd ..
cd ..
```

MBELib is considered a requirement in this build. You must read this notice prior to continuing. [MBElib Patent Notice](https://github.com/lwvmobile/mbelib#readme "MBElib Patent Notice") This version of MBELib is 1.3.2 and prints to STDERR, using the stock 1.3.0 MBElib may cause problems with print alignments if paired with this version of DSD-FME, or cause future compatibility issues.

```
git clone https://github.com/lwvmobile/mbelib
cd mbelib
mkdir build
cd build
cmake ..
make -j `nproc`
sudo make install
sudo ldconfig
cd ..
cd ..
```

Note: Some environment/distro users (Cygwin, Arch, and others) may need to manually copy any libmbe files found in the build folder to the /usr/lib folder on root.
`sudo cp libmbe* /usr/lib`

Finish by running these steps to clone and build DSD-FME.

```
git clone https://github.com/lwvmobile/dsd-fme
cd dsd-fme
sudo cp tone8.wav /usr/share/
sudo cp tone24.wav /usr/share/
sudo cp tone48.wav /usr/share/
sudo chmod 777 /usr/share/tone8.wav
sudo chmod 777 /usr/share/tone24.wav
sudo chmod 777 /usr/share/tone48.wav
mkdir build
cd build
cmake ..
make -j `nproc`
sudo make install
sudo ldconfig

```
Optional 'Virtual Sinks' for routing audio from SDR++ or GQRX, Media Players, etc. into DSD-FME

You may wish to direct sound into DSD-FME via Virtual Sinks. You may set up a Virtual Sink or two on your machine for routing audio in and out of applications to other applications using the following command, and opening up pavucontrol "PulseAudio Volume Control" in the menu (or `pulsemixer` in headless mode) to change line out of application to virtual sink, and line in of DSD-FME to monitor of virtual sink. This command will not persist past a reboot, so you will need to invoke them each time you reboot, or search for how to add this to your conf files for persistency if desired.

```
pactl load-module module-null-sink sink_name=virtual_sink  sink_properties=device.description=Virtual_Sink
pactl load-module module-null-sink sink_name=virtual_sink2  sink_properties=device.description=Virtual_Sink2
```

Already have this branch, and just want to pull the latest build? You can run the rebuild.sh file in the dsd-fme folder, or manually do the pull with the commands:

```
##Open your clone folder##
git pull
##cd into your build folder##
cd build
cmake ..
make -j `nproc`
sudo make install
sudo ldconfig
```

If the call alert wav files aren't playing, then make sure to run the following in the dsd-fme folder to copy the wav files to the /usr/share/ folder and give them adequate permission to be accessed.

```
sudo cp tone8.wav /usr/share/
sudo cp tone24.wav /usr/share/
sudo cp tone48.wav /usr/share/
sudo chmod 777 /usr/share/tone8.wav
sudo chmod 777 /usr/share/tone24.wav
sudo chmod 777 /usr/share/tone48.wav
```