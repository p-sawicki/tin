#ifndef COMMON
#define COMMON

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SEGMENT_SIZE 128

enum contex_type { SERVER_CONTEXT, CLIENT_CONTEXT };
enum packet_size_t { PACKET_SMALL, PACKET_MED, PACKET_LARGE };
extern const size_t packet_sizes[]; // 100B, 10MiB, 1GiB

SSL_CTX *create_context();
void init_openssl();
void cleanup_openssl();

#endif /* COMMON */
