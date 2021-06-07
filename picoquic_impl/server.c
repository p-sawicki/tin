// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 11.05.2021
// Autor: Piotr Sawicki

#include "common.h"
#include "utils.h"
#include <picoquic.h>
#include <picoquic_packet_loop.h>
#include <picoquic_utils.h>
#include <picosocks.h>
#include <stdint.h>

FILE *log_file;

typedef struct stream_ctx_t {
  struct stream_ctx_t *next_stream, *prev_stream;
  uint64_t stream_id;
  enum packet_size_t packet_size;
  size_t packet_len, packet_sent;
  unsigned int is_packet_size_read : 1;
  unsigned int is_stream_reset : 1;
  unsigned int is_stream_finished : 1;
} stream_ctx_t;

typedef struct ctx_t {
  stream_ctx_t *first_stream, *last_stream;
} ctx_t;

stream_ctx_t *create_stream_context(ctx_t *server_ctx, uint64_t stream_id) {
  stream_ctx_t *stream_ctx = (stream_ctx_t *)malloc(sizeof(stream_ctx_t));

  if (stream_ctx != NULL) {
    memset(stream_ctx, 0, sizeof(stream_ctx_t));

    if (server_ctx->last_stream == NULL) {
      server_ctx->last_stream = server_ctx->first_stream = stream_ctx;
    } else {
      stream_ctx->prev_stream = server_ctx->last_stream;
      server_ctx->last_stream->next_stream = stream_ctx;
      server_ctx->last_stream = stream_ctx;
    }
    stream_ctx->stream_id = stream_id;
  }

  return stream_ctx;
}

int open_stream(ctx_t *server_ctx, stream_ctx_t *stream_ctx) {
  int ret = 0;
  stream_ctx->is_packet_size_read = 1;

  if (!is_valid_packet_size(stream_ctx->packet_size)) {
    ret = PICOQUIC_UNKNOWN_PACKET_SIZE_ERROR;
  } else {
    stream_ctx->packet_len = packet_sizes[stream_ctx->packet_size];
  }

  return ret;
}

void delete_stream_context(ctx_t *server_ctx, stream_ctx_t *stream_ctx) {
  if (stream_ctx->prev_stream == NULL) {
    server_ctx->first_stream = stream_ctx->next_stream;
  } else {
    stream_ctx->prev_stream->next_stream = stream_ctx->next_stream;
  }

  if (stream_ctx->next_stream == NULL) {
    server_ctx->last_stream = stream_ctx->prev_stream;
  } else {
    stream_ctx->next_stream->prev_stream = stream_ctx->prev_stream;
  }

  free(stream_ctx);
}

void delete_context(ctx_t *server_ctx) {
  while (server_ctx->first_stream != NULL) {
    delete_stream_context(server_ctx, server_ctx->first_stream);
  }

  free(server_ctx);
}

int callback(picoquic_cnx_t *cnx, uint64_t stream_id, uint8_t *bytes,
             size_t length, picoquic_call_back_event_t fin_or_event,
             void *callback_ctx, void *v_stream_ctx) {
  int ret = 0;
  ctx_t *server_ctx = (ctx_t *)callback_ctx;
  stream_ctx_t *stream_ctx = (stream_ctx_t *)v_stream_ctx;

  if (callback_ctx == NULL ||
      callback_ctx ==
          picoquic_get_default_callback_context(picoquic_get_quic_ctx(cnx))) {
    server_ctx = (ctx_t *)malloc(sizeof(ctx_t));

    if (server_ctx == NULL) {
      picoquic_close(cnx, PICOQUIC_ERROR_MEMORY);
      return -1;
    } else {
      ctx_t *d_ctx = (ctx_t *)picoquic_get_default_callback_context(
          picoquic_get_quic_ctx(cnx));

      if (d_ctx != NULL) {
        memcpy(server_ctx, d_ctx, sizeof(ctx_t));
      } else {
        memset(server_ctx, 0, sizeof(ctx_t));
      }

      picoquic_set_callback(cnx, callback, server_ctx);
    }
  }

  if (ret == 0) {
    switch (fin_or_event) {
    case picoquic_callback_stream_data:
    case picoquic_callback_stream_fin:
      if (stream_ctx == NULL) {
        stream_ctx = create_stream_context(server_ctx, stream_id);
      }

      if (stream_ctx == NULL) {
        picoquic_reset_stream(cnx, stream_id, PICOQUIC_INTERNAL_ERROR);
        return -1;
      } else if (stream_ctx->is_packet_size_read) {
        return -1;
      } else {
        if (length > 0) {
          memcpy(&stream_ctx->packet_size, bytes, length);
        }

        if (fin_or_event == picoquic_callback_stream_fin) {
          stream_ctx->is_packet_size_read = 1;
          int stream_ret = open_stream(server_ctx, stream_ctx);

          if (stream_ret == 0) {
            ret = picoquic_mark_active_stream(cnx, stream_id, 1, stream_ctx);
          } else {
            delete_stream_context(server_ctx, stream_ctx);
            picoquic_reset_stream(cnx, stream_id, stream_ret);
          }
        }
      }
      break;
    case picoquic_callback_prepare_to_send:
      if (stream_ctx != NULL && is_valid_packet_size(stream_ctx->packet_size)) {
        size_t avail = stream_ctx->packet_len - stream_ctx->packet_sent;
        int is_fin = 1;
        uint8_t *buffer;

        if (avail > length) {
          avail = length;
          is_fin = 0;
        }

        buffer =
            picoquic_provide_stream_data_buffer(bytes, avail, is_fin, !is_fin);
        if (buffer != NULL) {
          for (size_t i = 0; i < avail; ++i) {
            buffer[i] = PICOQUIC_PACKET_BYTE;
          }

          stream_ctx->packet_sent += avail;
        } else {
          ret = -1;
        }
      }
      break;
    case picoquic_callback_stream_reset:
    case picoquic_callback_stop_sending:
      if (stream_ctx != NULL) {
        delete_stream_context(server_ctx, stream_ctx);
        picoquic_reset_stream(cnx, stream_id, PICOQUIC_PACKET_CANCEL_ERROR);
      }
      break;
    case picoquic_callback_stateless_reset:
    case picoquic_callback_close:
    case picoquic_callback_application_close:
      delete_context(server_ctx);
      picoquic_set_callback(cnx, NULL, NULL);
      break;
    default:
      break;
    }
  }

  return ret;
}

int picoquic_server(int server_port, const char *pem_cert,
                    const char *pem_key) {
  int ret = 0;
  picoquic_quic_t *quic = NULL;
  uint64_t current_time = 0;
  ctx_t default_context = {0};

  fprintf(log_file, "Starting picoquic server on port %d\n", server_port);

  current_time = picoquic_current_time();
  quic = picoquic_create(8, pem_cert, pem_key, NULL, PICOQUIC_ALPN, callback,
                         &default_context, NULL, NULL, NULL, current_time, NULL,
                         NULL, NULL, 0);

  if (quic == NULL) {
    fprintf(log_file, "Could not create server context\n");
    ret = -1;
  } else {
    picoquic_set_cookie_mode(quic, 2);
    picoquic_set_default_congestion_algorithm(quic, picoquic_bbr_algorithm);
  }

  if (ret == 0) {
    ret = picoquic_packet_loop(quic, server_port, 0, 0, 0, 0, NULL, NULL);
  }

  fprintf(log_file, "Server exit, ret = %d\n", ret);

  if (quic != NULL) {
    picoquic_free(quic);
  }

  return ret;
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s port cert_file private_key_file\n", name);
  exit(1);
}

int main(int argc, char **argv) {
  int exit_code = 0;

  if (argc < 4) {
    usage(argv[0]);
  } else {
    log_file = get_log(argc, argv);

    int server_port = get_port(argv[1]);
    exit_code = picoquic_server(server_port, argv[2], argv[3]);
    fclose(log_file);
  }

  exit(exit_code);
}