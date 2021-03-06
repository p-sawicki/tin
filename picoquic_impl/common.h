// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 11.05.2021
// Autor: Piotr Sawicki

#ifndef PICOQUIC_COMMON_H
#define PICOQUIC_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define PICOQUIC_ALPN "tin"
#define PICOQUIC_SNI "localhost"
#define PICOQUIC_PACKET_BYTE 0xff

#define PICOQUIC_NO_ERROR 0
#define PICOQUIC_INTERNAL_ERROR 0x101
#define PICOQUIC_UNKNOWN_PACKET_SIZE_ERROR 0x102
#define PICOQUIC_PACKET_CANCEL_ERROR 0x103

#endif // PICOQUIC_COMMON_H