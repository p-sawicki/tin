# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 29.05.2021
# Autor: Piotr Sawicki

FROM ubuntu:latest
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata
RUN apt-get update && apt-get install -y pkg-config git libssl-dev cmake build-essential python3-pip g++ m4 libboost-all-dev libevent-dev libdouble-conversion-dev libgoogle-glog-dev libgflags-dev libiberty-dev liblz4-dev liblzma-dev libsnappy-dev make zlib1g-dev binutils-dev libjemalloc-dev libsodium-dev
RUN pip3 install argparse ipcqueue