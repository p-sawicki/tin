#ifndef COMMON
#define COMMON

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

enum contex_type { SERVER_CONTEXT, CLIENT_CONTEXT };


SSL_CTX *create_context();
void init_openssl();
void cleanup_openssl();

#endif /* COMMON */
