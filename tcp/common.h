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

#define TCP_CLIENT_LOG_DIR "logs"
#define TCP_SERVER_LOG_DIR "logs"

enum contex_type { SERVER_CONTEXT, CLIENT_CONTEXT };
enum packet_size_t { PACKET_SMALL, PACKET_MED, PACKET_LARGE };
extern const size_t packet_sizes[]; // 100B, 10MiB, 1GiB

SSL_CTX *create_context();
void init_openssl();
void cleanup_openssl();
FILE *open_log();

#endif /* COMMON */
