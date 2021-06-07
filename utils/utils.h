// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 29.05.2021
// Autor: Piotr Sawicki

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

enum packet_size_t { PACKET_SMALL, PACKET_MED, PACKET_LARGE };
extern const size_t packet_sizes[]; // 100B, 10MiB, 1GiB

int get_port(char const *port_arg);
int is_valid_packet_size(enum packet_size_t size);
FILE *get_log(int argc, char **argv);
int delay(int argc, char **argv, int nb_packets);
void get_packet_info(int scenario, int *nb_packets,
                     enum packet_size_t *packet_size);

#endif // UTILS_H