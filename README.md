# libcapn

[![Build Status](http://img.shields.io/travis/adobkin/libcapn.svg?style=flat&branch=master)](http://travis-ci.org/adobkin/libcapn) [![MIT](http://img.shields.io/badge/license-MIT-red.svg?style=flat)](https://github.com/adobkin/libcapn/blob/master/LICENSE)

libcapn is a C Library to interact with the [Apple Push Notification Service](http://developer.apple.com/library/mac/#documentation/NetworkingInternet/Conceptual/RemoteNotificationsPG/ApplePushService/ApplePushService.html) (APNs for short) using simple and intuitive API.
With the library you can easily send push notifications to iOS and OS X (>= 10.8) devices.

__Version 2.0 isn't compatible with 1.0__


## Compile libcapn and apn-pusher

```bash
git clone https://github.com/azhai/apn-pusher.git
git submodule update --init
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc" \
    -DCMAKE_INSTALL_PREFIX=/opt/apn-pusher-2.0.2 ../
make
sudo make install

cd /opt/apn-pusher-2.0.2/
patchelf --set-rpath '$ORIGIN:$ORIGIN/../lib/capn' bin/*
chmod +x bin/apn-pusher
sudo ln -s /opt/apn-pusher-2.0.2/bin/apn-pusher /usr/bin/
```

## Usage of pypush

```
cd ~/projects/
cp -r apn-pusher/src/pypusher ./
cd pypusher
pip install -r requirements.txt
#change settings.py and certs/prod.p12
python pusher.py
```


## Table of Contents

<!-- toc -->
* [Installation](#installation)
  * [on *nix](#on-nix)
  * [on Windows](#on-windows)
* [Quick Start](#quick-start)
  * [Initialize and configure context](#initialize-and-configure-context)
    * [Logging](#logging)
    * [Connection](#connection)
  * [Sending notifications](#sending-notifications)
    * [The notification payload](#the-notification-payload)
    * [Tokens](#tokens)
    * [Send](#send)
  * [Example](#example)
* [apn-pusher](#apn-pusher)

<!-- toc stop -->
## Installation

### on *nix

__Requirements__

- [CMake](http://cmake.org) >= 2.8.5
- Clang 3 and later or GCC 4.6 and later
- make

__Build instructions__

```sh
$ git clone https://github.com/adobkin/libcapn.git
$ git submodule update --init
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ../
$ make
$ sudo make install
```

### on Windows

__Requirements__

- [Microsoft Visual Studio 2015](https://www.visualstudio.com)
- [CMake](http://cmake.org) >= 2.8.5
- [Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145)

__Build instructions__

1. [Download](https://github.com/adobkin/libcapn/releases/latest) the latest source archive from [GitHub](https://github.com/adobkin/libcapn/releases/latest) and extract it somewhere on your disk, e.g. `C:\libcapn`

2. Open command console (Win-R ==> "cmd" => Enter)

3. Go to the libcapn directory and run `win_build\build.bat`

```sh
cd C:\libcapn
win_build\build.bat
```


## apn-pusher

apn-pusher - simple command line tool to send push notifications to iOS and OS X devices:

```sh
apn-pusher -c ./test_push.p12 -p -d -m 'Test' -t 1D2EE2B3A38689E0D43E6608FEDEFCA534BBAC6AD6930BFDA6F5CD72A808832B:1D2EE2B3A38689E0D43E6608FEDEFCA534BBAC6AD6930BFDA6F5CD72A808832A
```

```sh
apn-pusher -c ./test_push.p12 -P 123456 -d -m 'Test' -T ./tokens.txt -o ./logs/push.log -v
```

```sh
python pusher.py tokens.txt '我们的APP有了新功能，请大家升级吧！'
```

Options:

```sh
Usage: apn-pusher [OPTION]
    -h Print this message and exit
    -c Path to .p12 file (required)
    -P Passphrase string for .p12 file
    -p Passphrase for .p12 file. Will be asked from the tty
    -d Use sandbox mode
    -m Body of the alert to send in notification
    -a Indicates content available
    -b Badge number to set with notification
    -s Name of a sound file in the app bundle
    -i Name of an image file in the app bundle
    -y Category name of notification
    -t Tokens, separated with ':' (required)
    -T Path to file with tokens
    -o Path to logging file
    -v Make the operation more talkative
```
