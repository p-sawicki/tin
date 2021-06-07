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
int if_delay = 0;

typedef struct stream_ctx_t {
  struct stream_ctx_t *next_stream;
  size_t bytes_received;
  enum packet_size_t packet_size;
  uint64_t stream_id, remote_error;
  unsigned int is_packet_size_sent : 1;
  unsigned int is_stream_reset : 1;
  unsigned int is_stream_finished : 1;
} stream_ctx_t;

typedef struct ctx_t {
  stream_ctx_t *first_stream, *last_stream;
  int nb_packets, nb_packets_received, nb_packets_failed, is_disconnected;
  enum packet_size_t packet_size;
} ctx_t;

static int create_stream(picoquic_cnx_t *cnx, ctx_t *client_ctx,
                         int packet_rank) {
  int ret = 0;
  stream_ctx_t *stream_ctx = (stream_ctx_t *)malloc(sizeof(stream_ctx_t));

  if (stream_ctx == NULL) {
    fprintf(log_file,
            "Memory error, cannot create stream for packet number %d\n",
            (int)packet_rank);
    ret = -1;
  } else {
    memset(stream_ctx, 0, sizeof(stream_ctx_t));

    if (client_ctx->first_stream == NULL) {
      client_ctx->first_stream = client_ctx->last_stream = stream_ctx;
    } else {
      client_ctx->last_stream->next_stream = stream_ctx;
      client_ctx->last_stream = stream_ctx;
    }

    stream_ctx->stream_id = (uint64_t)4 * packet_rank;
    stream_ctx->packet_size = client_ctx->packet_size;

    ret =
        picoquic_mark_active_stream(cnx, stream_ctx->stream_id, 1, stream_ctx);

    if (ret != 0) {
      fprintf(log_file,
              "Error %d, cannot initialize stream for packet number %d\n", ret,
              (int)packet_rank);
    } else {
      fprintf(log_file, "Opened stream %d\n", 4 * packet_rank);
    }
  }
  return ret;
}

static void report(ctx_t *client_ctx) {
  stream_ctx_t *stream_ctx = client_ctx->first_stream;

  while (stream_ctx != NULL) {
    char const *status;

    if (stream_ctx->is_stream_finished) {
      status = "complete";
    } else if (stream_ctx->is_stream_reset) {
      status = "reset";
    } else {
      status = "unknown status";
    }

    fprintf(log_file, "%ld: %s, received %zu bytes", stream_ctx->stream_id,
            status, stream_ctx->bytes_received);

    if (stream_ctx->is_stream_reset &&
        stream_ctx->remote_error != PICOQUIC_NO_ERROR) {
      char const *error_text = "unknown error";

      switch (stream_ctx->remote_error) {
      case PICOQUIC_INTERNAL_ERROR:
        error_text = "internal error";
        break;
      case PICOQUIC_UNKNOWN_PACKET_SIZE_ERROR:
        error_text = "unknown packet size";
        break;
      case PICOQUIC_PACKET_CANCEL_ERROR:
        error_text = "cancelled";
        break;
      default:
        break;
      }

      fprintf(log_file, ", error 0x%" PRIx64 " -- %s", stream_ctx->remote_error,
              error_text);
    }
    fprintf(log_file, "\n");
    stream_ctx = stream_ctx->next_stream;
  }
}

static void free_context(ctx_t *client_ctx) {
  stream_ctx_t *stream_ctx;

  while ((stream_ctx = client_ctx->first_stream) != NULL) {
    client_ctx->first_stream = stream_ctx->next_stream;

    free(stream_ctx);
  }

  client_ctx->last_stream = NULL;
}

int callback(picoquic_cnx_t *cnx, uint64_t stream_id, uint8_t *bytes,
             size_t length, picoquic_call_back_event_t fin_or_event,
             void *callback_ctx, void *v_stream_ctx) {
  int ret = 0;
  ctx_t *client_ctx = (ctx_t *)callback_ctx;
  stream_ctx_t *stream_ctx = (stream_ctx_t *)v_stream_ctx;

  if (client_ctx == NULL) {
    return -1;
  }

  switch (fin_or_event) {
  case picoquic_callback_stream_data:
  case picoquic_callback_stream_fin:
    if (stream_ctx == NULL || !stream_ctx->is_packet_size_sent ||
        (stream_ctx->is_stream_reset || stream_ctx->is_stream_finished)) {
      return -1;
    } else {
      if (length > 0) {
        for (size_t i = 0; i < length; ++i) {
          if (bytes[i] != PICOQUIC_PACKET_BYTE) {
            fprintf(log_file, "Received incorrect data.\n");
            ret = -1;
            break;
          }
        }
        stream_ctx->bytes_received += length;
      }

      if (ret == 0 && fin_or_event == picoquic_callback_stream_fin) {
        stream_ctx->is_stream_finished = 1;
        ++client_ctx->nb_packets_received;

        if (client_ctx->nb_packets_received + client_ctx->nb_packets_failed >=
            client_ctx->nb_packets) {
          ret = picoquic_close(cnx, 0);
        }

        if (client_ctx->nb_packets > 1 && if_delay) {
          sleep(6);
        }
      }
    }
    break;
  case picoquic_callback_stop_sending:
    picoquic_reset_stream(cnx, stream_id, 0);
    // fall-through
  case picoquic_callback_stream_reset:
    if (stream_ctx == NULL || stream_ctx->is_stream_reset ||
        stream_ctx->is_stream_finished) {
      return -1;
    } else {
      stream_ctx->remote_error =
          picoquic_get_remote_stream_error(cnx, stream_id);
      stream_ctx->is_stream_reset = 1;
      ++client_ctx->nb_packets_failed;

      if (client_ctx->nb_packets_received + client_ctx->nb_packets_failed >=
          client_ctx->nb_packets) {
        fprintf(log_file, "All done, closing the connection.\n");
        ret = picoquic_close(cnx, 0);
      }
    }
    break;
  case picoquic_callback_stateless_reset:
  case picoquic_callback_close:
  case picoquic_callback_application_close:
    fprintf(log_file, "Connection closed.\n");
    client_ctx->is_disconnected = 1;
    picoquic_set_callback(cnx, NULL, NULL);
    break;
  case picoquic_callback_version_negotiation:
    fprintf(log_file, "Received a version negotiation request:");
    for (size_t byte_index = 0; byte_index + 4 <= length; byte_index += 4) {
      uint32_t vn = 0;

      for (int i = 0; i < 4; ++i) {
        vn <<= 8;
        vn += bytes[byte_index + i];
      }

      fprintf(log_file, "%s%08x", (byte_index == 0) ? " " : ", ", vn);
    }

    fprintf(log_file, "\n");
    break;
  case picoquic_callback_prepare_to_send:
    if (stream_ctx == NULL) {
      return -1;
    } else {
      uint8_t *buffer = picoquic_provide_stream_data_buffer(
          bytes, sizeof(enum packet_size_t), 1, 0);

      if (buffer != NULL) {
        memcpy(buffer, &stream_ctx->packet_size,
               sizeof(stream_ctx->packet_size));
        stream_ctx->is_packet_size_sent = 1;
      } else {
        ret = -1;
      }
    }
    break;
  case picoquic_callback_almost_ready:
    fprintf(log_file, "Connection to the server completed, almost ready.\n");
    break;
  case picoquic_callback_ready:
    fprintf(log_file, "Connection to the server confirmed.\n");
    break;
  default:
    break;
  }

  return ret;
}

static int loop_cb(picoquic_quic_t *quic, picoquic_packet_loop_cb_enum cb_mode,
                   void *callback_ctx) {
  int ret = 0;
  ctx_t *cb_ctx = (ctx_t *)callback_ctx;

  if (cb_ctx == NULL) {
    ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
  } else {
    switch (cb_mode) {
    case picoquic_packet_loop_ready:
      fprintf(log_file, "Waiting for packets.\n");
      break;
    case picoquic_packet_loop_after_receive:
      break;
    case picoquic_packet_loop_after_send:
      if (cb_ctx->is_disconnected) {
        ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
      }
      break;
    default:
      ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
      break;
    }
  }
  return ret;
}

int picoquic_client(char const *server_name, int server_port, int nb_packets,
                    enum packet_size_t packet_size) {
  int ret = 0;
  struct sockaddr_storage server_address;
  char const *sni = PICOQUIC_SNI;
  picoquic_quic_t *quic = NULL;

  ctx_t client_ctx = {0};

  picoquic_cnx_t *cnx = NULL;
  uint64_t current_time = picoquic_current_time();

  int is_name = 0;
  ret = picoquic_get_server_address(server_name, server_port, &server_address,
                                    &is_name);
  if (ret != 0) {
    fprintf(log_file, "Cannot get the IP address for <%s> port <%d>",
            server_name, server_port);
  } else if (is_name) {
    sni = server_name;
  }

  if (ret == 0) {
    quic = picoquic_create(1, NULL, NULL, NULL, PICOQUIC_ALPN, NULL, NULL, NULL,
                           NULL, NULL, current_time, NULL, NULL, NULL, 0);

    if (quic == NULL) {
      fprintf(log_file, "Could not create quic context\n");
      ret = -1;
    } else {
      picoquic_set_default_congestion_algorithm(quic, picoquic_bbr_algorithm);
    }
  }

  if (ret == 0) {
    client_ctx.packet_size = packet_size;
    client_ctx.nb_packets = nb_packets;

    fprintf(log_file, "Starting connection to %s, port %d\n", server_name,
            server_port);

    cnx = picoquic_create_cnx(quic, picoquic_null_connection_id,
                              picoquic_null_connection_id,
                              (struct sockaddr *)&server_address, current_time,
                              0, sni, PICOQUIC_ALPN, 1);

    if (cnx == NULL) {
      fprintf(log_file, "Could not create connection context\n");
      ret = -1;
    } else {
      picoquic_set_callback(cnx, callback, &client_ctx);
      ret = picoquic_start_client_cnx(cnx);

      if (ret < 0) {
        fprintf(log_file, "Could not activate connection\n");
      } else {
        picoquic_connection_id_t icid = picoquic_get_initial_cnxid(cnx);
        fprintf(log_file, "Initial connection ID: ");

        for (uint8_t i = 0; i < icid.id_len; ++i) {
          fprintf(log_file, "%02x", icid.id[i]);
        }
        fprintf(log_file, "\n");
      }
    }

    for (int i = 0; ret == 0 && i < client_ctx.nb_packets; ++i) {
      ret = create_stream(cnx, &client_ctx, i);

      if (ret < 0) {
        fprintf(log_file, "Could not initiate stream for fi\n");
      }
    }
  }

  ret = picoquic_packet_loop(quic, 0, server_address.ss_family, 0, 0, 0,
                             loop_cb, &client_ctx);
  report(&client_ctx);

  if (quic != NULL) {
    picoquic_free(quic);
  }

  free_context(&client_ctx);
  return ret;
}

void usage(char const *name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "    %s server_name port scenario\n", name);
  exit(1);
}

int main(int argc, char **argv) {
  int exit_code = 0;

  if (argc < 4) {
    usage(argv[0]);
  } else {
    log_file = get_log(argc, argv);

    int server_port = get_port(argv[2]);
    int scenario = atoi(argv[3]);

    int nb_packets;
    enum packet_size_t packet_size;
    get_packet_info(scenario, &nb_packets, &packet_size);

    if_delay = delay(argc, argv, nb_packets);

    exit_code = picoquic_client(argv[1], server_port, nb_packets, packet_size);
    fclose(log_file);
  }

  exit(exit_code);
}