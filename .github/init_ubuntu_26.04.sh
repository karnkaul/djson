#!/bin/bash

install_compilers=true
if [[ "$1" == "--skip-install" ]]; then
  install_compilers=false
fi

echo "install_compilers: $install_compilers"

if [[ "$install_compilers" == "true" ]]; then
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 23

  sudo apt install -yq gcc-16 g++-16

  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 10
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 10

  sudo update-alternatives --remove-all clang
  sudo update-alternatives --remove-all clang++
  sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-23 10
  sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-23 10
fi

sudo update-alternatives --remove-all clang-format
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-22 10
