#!/bin/sh

# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 29.05.2021
# Autor: Piotr Sawicki

PATH=.

if [ "$#" -eq 1 ]; then
  PATH=$1
fi

$PATH/build/picoquic_impl/pico_server 1111 cert.pem key.pem --quiet &
$PATH/build/msquic_impl/ms_server 1112 cert.pem key.pem --quiet &
$PATH/build/tcp/tcp_server 1113 cert.pem key.pem --quiet & 