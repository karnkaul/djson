#!/bin/bash

sudo apt update -yq
sudo apt install libbtrfsutil-dev

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 10

sudo update-alternatives --remove-all clang
sudo update-alternatives --remove-all clang++
sudo update-alternatives --remove-all clang-format
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-22 10
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-22 10
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-22 10

exit 0
