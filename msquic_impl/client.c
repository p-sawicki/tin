// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 22.05.2021
// Autor: Marcin Białek

#include "common.h"
#include "utils.h"
#include <stdio.h>
#include <msquic.h>
#include <pthread.h>

int ifDelay = 0;

typedef struct {
    FILE* log;
    const char* address;
    unsigned short port;
    int scenario;
    unsigned int nbStreams;
    const QUIC_API_TABLE* msQuic;
    QUIC_REGISTRATION_CONFIG regConfig;
    HQUIC registration;
    QUIC_BUFFER alpn;
    HQUIC configuration;
    unsigned int totalReceived;
} MsQuicClientContext;


static void printUsage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "ms_client server_address server_port scenario\n");
}


static QUIC_STATUS initMsQuic(MsQuicClientContext* ctx) {
    QUIC_STATUS status; 
    ctx->regConfig = (QUIC_REGISTRATION_CONFIG){ 
        "ms_client", 
        QUIC_EXECUTION_PROFILE_LOW_LATENCY 
    };
    ctx->alpn = (QUIC_BUFFER){
        sizeof(PROTOCOL_NAME) - 1, 
        (uint8_t*)PROTOCOL_NAME
    };

    if(QUIC_FAILED(status = MsQuicOpen(&ctx->msQuic))) {
        return status;
    }

    if(QUIC_FAILED(status = ctx->msQuic->RegistrationOpen(&ctx->regConfig, &ctx->registration))) {
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}


static void deinitMsQuic(MsQuicClientContext* ctx) {
    if(ctx->msQuic != NULL) {
        if(ctx->configuration != NULL) {
            ctx->msQuic->ConfigurationClose(ctx->configuration);
        }

        if(ctx->registration != NULL) {
            ctx->msQuic->RegistrationClose(ctx->registration);
        }

        MsQuicClose(ctx->msQuic);
    }
}


static QUIC_STATUS loadConfiguration(MsQuicClientContext* ctx, int argc, char* argv[]) {
    QUIC_STATUS status = QUIC_STATUS_SUCCESS;

    if(argc < 4) {
        printUsage();
        return QUIC_STATUS_INVALID_PARAMETER;
    }

    errno = 0;
    ctx->address = argv[1];
    ctx->port = (unsigned short)strtoul(argv[2], NULL, 0);
    ctx->scenario = strtol(argv[3], NULL, 0);

    if(errno != 0 || ctx->scenario < 1 || 4 < ctx->scenario ) {
        printUsage();
        return QUIC_STATUS_INVALID_PARAMETER;
    }

    ctx->nbStreams = (unsigned int[]){
        1, 1, 10, 10
    }[ctx->scenario - 1];

    QUIC_SETTINGS settings = {
        .IdleTimeoutMs = TIMEOUT,
        .IsSet.IdleTimeoutMs = 1,
        .DesiredVersionsList = (uint32_t[]){ 0xff00001dU },
        .DesiredVersionsListLength = 1,
        .IsSet.DesiredVersionsList = 1
    };

    QUIC_CREDENTIAL_CONFIG cred = {
        .Type = QUIC_CREDENTIAL_TYPE_NONE,
        .Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION
    };

    if(QUIC_FAILED(status = ctx->msQuic->ConfigurationOpen(ctx->registration, &ctx->alpn, 1, &settings, sizeof(settings), (void*)ctx, &ctx->configuration))) {
        fprintf(ctx->log, "ConfigurationOpen failed - 0x%x\n", status);
        return status;
    }

    if(QUIC_FAILED(status = ctx->msQuic->ConfigurationLoadCredential(ctx->configuration, &cred))) {
        fprintf(ctx->log, "ConfigurationLoadCredential failed - 0x%x\n", status);
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}


static QUIC_STATUS streamCallback(HQUIC stream, MsQuicClientContext* ctx, QUIC_STREAM_EVENT* event) {
    switch(event->Type) {
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        free(event->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_RECEIVE:
        ctx->totalReceived += event->RECEIVE.TotalBufferLength;
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        break;

    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        fprintf(ctx->log, "Total bytes received: %u\n", ctx->totalReceived);
        ctx->msQuic->StreamClose(stream);
        break;
    
    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}


static QUIC_STATUS sendRequest(MsQuicClientContext* ctx, HQUIC connection) {
    QUIC_STATUS status;
    HQUIC stream = NULL;

    if(QUIC_FAILED(status = ctx->msQuic->StreamOpen(connection, QUIC_STREAM_OPEN_FLAG_NONE, (void*)streamCallback, ctx, &stream))) {
        fprintf(ctx->log, "StreamOpen failed - 0x%x\n", status);
        return status;
    }

    if(QUIC_FAILED(status = ctx->msQuic->StreamStart(stream, QUIC_STREAM_START_FLAG_NONE))) {
        fprintf(ctx->log, "StreamStart failed - 0x%x\n", status);
        ctx->msQuic->StreamClose(stream);
        return status;
    }

    QUIC_BUFFER* buffer = malloc(sizeof(QUIC_BUFFER) + sizeof(ctx->scenario));
    buffer->Length = sizeof(ctx->scenario);
    buffer->Buffer = (void*)buffer + sizeof(QUIC_BUFFER);
    memcpy(buffer->Buffer, &ctx->scenario, sizeof(ctx->scenario));

    if(QUIC_FAILED(status = ctx->msQuic->StreamSend(stream, buffer, 1, QUIC_SEND_FLAG_FIN, buffer))) {
        fprintf(ctx->log, "StreamSend failed - 0x%x\n", status);
        free(buffer);
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}


static void* sendRequestsAsyncWorker(void* args[]) {
    HQUIC connection = args[0];
    MsQuicClientContext* ctx = args[1];

    for(unsigned int i = 0; i < ctx->nbStreams; i++) {
        sendRequest(ctx, connection);
        if (ifDelay) {
            sleep(rand() % 6 + 1);
        }
    }

    free(args);
    return NULL;
}


static void sendRequestsAsync(HQUIC connection, MsQuicClientContext* ctx) {
    pthread_t thread;
    void** args = malloc(2 * sizeof(void*));
    args[0] = connection;
    args[1] = ctx;
    pthread_create(&thread, NULL, (void*)sendRequestsAsyncWorker, args);
}


static QUIC_STATUS connectionCallback(HQUIC connection, MsQuicClientContext* ctx, QUIC_CONNECTION_EVENT* event) {
    switch(event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        fprintf(ctx->log, "Connected to %s:%d\n", ctx->address, ctx->port);
        ctx->totalReceived = 0;
        sendRequestsAsync(connection, ctx);
        break;
    
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        if(!event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
            ctx->msQuic->ConnectionClose(connection);
        }

        break;

    default:
        break;
    }

    return QUIC_STATUS_SUCCESS; 
}


static QUIC_STATUS runMsQuicClient(MsQuicClientContext* ctx) {
    QUIC_STATUS status;
    HQUIC connection = NULL;

    if(QUIC_FAILED(status = ctx->msQuic->ConnectionOpen(ctx->registration, (void*)connectionCallback, ctx, &connection))) {
        fprintf(ctx->log, "ConnectionOpen failed - 0x%x\n", status);
        return status;
    }

    fprintf(ctx->log, "Connecting to %s:%d\n", ctx->address, ctx->port);

    if(QUIC_FAILED(status = ctx->msQuic->ConnectionStart(connection, ctx->configuration, QUIC_ADDRESS_FAMILY_UNSPEC, ctx->address, ctx->port))) {
        fprintf(ctx->log, "ConnectionStart failed - 0x%x\n", status);
        ctx->msQuic->ConnectionClose(connection);
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}


int main(int argc, char* argv[]) {
    MsQuicClientContext ctx;
    QUIC_STATUS status = QUIC_STATUS_SUCCESS;

    if((ctx.log = get_log(argc, argv)) == NULL) {
        return 1;
    }
    
    if(QUIC_FAILED(status = initMsQuic(&ctx))) {
        fprintf(ctx.log, "initMsQuic failed - 0x%x\n", status);
        deinitMsQuic(&ctx);
        fclose(ctx.log);
        return (int)status;
    }

    if(QUIC_FAILED(status = loadConfiguration(&ctx, argc, argv))) {
        deinitMsQuic(&ctx);
        fclose(ctx.log);
        return (int)status;
    }

    ifDelay = delay(argc, argv, ctx.nbStreams);

    status = runMsQuicClient(&ctx);
    deinitMsQuic(&ctx);
    fclose(ctx.log);
    return (int)status;
}
