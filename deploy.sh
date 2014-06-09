#!/bin/bash

kext=build/Debug/osxcpuinfo.kext/

sudo kextunload $kext
sudo rm -rf $kext
xcodebuild -configuration Debug
sudo chown -R root:wheel $kext
sudo kextutil -tn $kext
sudo kextload $kext
