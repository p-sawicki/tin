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

#define PICOQUIC_CLIENT_TICKET_STORE "logs/ticket_store.bin"
#define PICOQUIC_CLIENT_TOKEN_STORE "logs/token_store.bin"
#define PICOQUIC_CLIENT_QLOG_DIR "logs"
#define PICOQUIC_SERVER_QLOG_DIR "logs"

enum packet_size_t { PACKET_SMALL, PACKET_MED, PACKET_LARGE };
extern const size_t packet_sizes[]; // 100B, 10MiB, 1GiB

int get_port(char const *sample_name, char const *port_arg);
int is_valid_packet_size(enum packet_size_t size);
FILE *open_log();

#endif // PICOQUIC_COMMON_H