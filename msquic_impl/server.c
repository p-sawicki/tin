#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <msquic.h>


typedef struct {
    unsigned short port;
    const QUIC_API_TABLE* msQuic;
    QUIC_REGISTRATION_CONFIG regConfig;
    HQUIC registration;
    QUIC_BUFFER alpn;
    HQUIC configuration;
} MsQuicServerContext;


static void printUsage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "ms_server port cert_file key_file\n");
}


static QUIC_STATUS initMsQuic(MsQuicServerContext* ctx) {
    QUIC_STATUS status; 
    ctx->regConfig = (QUIC_REGISTRATION_CONFIG){ 
        "ms_server", 
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


static void deinitMsQuic(MsQuicServerContext* ctx) {
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


static QUIC_STATUS loadConfiguration(MsQuicServerContext* ctx, int argc, char* argv[]) {
    QUIC_STATUS status = QUIC_STATUS_SUCCESS;

    if(argc < 4) {
        printUsage();
        return QUIC_STATUS_INVALID_PARAMETER;
    }

    errno = 0;
    ctx->port = (unsigned short)strtoul(argv[1], NULL, 0);

    if(errno != 0) {
        printUsage();
        return QUIC_STATUS_INVALID_PARAMETER;
    }

    QUIC_SETTINGS settings = {
        .IdleTimeoutMs = TIMEOUT,
        .IsSet.IdleTimeoutMs = 1,
        .ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT,
        .IsSet.ServerResumptionLevel = 1,
        .PeerBidiStreamCount = 1,
        .IsSet.PeerBidiStreamCount = 1
    };

    QUIC_CERTIFICATE_FILE cert = {
        .CertificateFile = argv[2],
        .PrivateKeyFile = argv[3]
    };

    QUIC_CREDENTIAL_CONFIG cred = {
        .Flags = QUIC_CREDENTIAL_FLAG_NONE,
        .Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE,
        .CertificateFile = &cert
    };
    
    if(QUIC_FAILED(status = ctx->msQuic->ConfigurationOpen(ctx->registration, &ctx->alpn, 1, &settings, sizeof(settings), NULL, &ctx->configuration))) {
        fprintf(stderr, "ConfigurationOpen failed - 0x%x\n", status);
        return status;
    }

    if(QUIC_FAILED(status = ctx->msQuic->ConfigurationLoadCredential(ctx->configuration, &cred))) {
        fprintf(stderr, "ConfigurationLoadCredential failed - 0x%x\n", status);
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}


static QUIC_STATUS handleRequest(HQUIC stream, MsQuicServerContext* ctx, QUIC_STREAM_EVENT* event) {
    QUIC_STATUS status;

    if(event->RECEIVE.BufferCount != 1 || event->RECEIVE.Buffers[0].Length < 4) {
        fprintf(stderr, "Malformed request\n");
        return QUIC_STATUS_PROTOCOL_ERROR;
    }

    const int scenario = *(int*)event->RECEIVE.Buffers[0].Buffer;
    
    if(scenario < 1 || 4 < scenario) {
        fprintf(stderr, "Unknown scenario %d\n", scenario);
        return QUIC_STATUS_PROTOCOL_ERROR;
    }
    
    const unsigned int packetSize = (unsigned int[]){
        100, 1073741824, 100, 10485760
    }[scenario - 1];

    QUIC_BUFFER* buffer = malloc(sizeof(QUIC_BUFFER) + packetSize);
    buffer->Length = packetSize;
    buffer->Buffer = (void*)buffer + sizeof(QUIC_BUFFER);
    memset(buffer->Buffer, 0xff, packetSize);

    if(QUIC_FAILED(status = ctx->msQuic->StreamSend(stream, buffer, 1, QUIC_SEND_FLAG_FIN, buffer))) {
        printf("StreamSend failed - 0x%x\n", status);
        free(buffer);
        return status;
    }

    printf("Bytes sent: %u\n", packetSize);
    return QUIC_STATUS_SUCCESS;
}


static QUIC_STATUS streamCallback(HQUIC stream, MsQuicServerContext* ctx, QUIC_STREAM_EVENT* event) {
    switch(event->Type) {
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        free(event->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_RECEIVE:
        return handleRequest(stream, ctx, event);

    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        ctx->msQuic->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        break;

    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        ctx->msQuic->StreamClose(stream);
        break;
    
    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}


static QUIC_STATUS connectionCallback(HQUIC connection, MsQuicServerContext* ctx, QUIC_CONNECTION_EVENT* event) {
    switch(event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        printf("[connection][%p] Connected\n", connection);
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
    case QUIC_CONNECTION_EVENT_RESUMED:
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        printf("[connection][%p] Disconnected\n", connection);
        ctx->msQuic->ConnectionClose(connection);
        break;

    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        ctx->msQuic->SetCallbackHandler(event->PEER_STREAM_STARTED.Stream, (void*)streamCallback, (void*)ctx);
        break;

    default:
        break;
    }
    
    return QUIC_STATUS_SUCCESS;
};


static QUIC_STATUS listenerCallback(HQUIC listener, MsQuicServerContext* ctx, QUIC_LISTENER_EVENT* event) {
    (void)listener;
    QUIC_STATUS status = QUIC_STATUS_NOT_SUPPORTED;

    switch(event->Type) {
    case QUIC_LISTENER_EVENT_NEW_CONNECTION:
        ctx->msQuic->SetCallbackHandler(event->NEW_CONNECTION.Connection, (void*)connectionCallback, (void*)ctx);
        status = ctx->msQuic->ConnectionSetConfiguration(event->NEW_CONNECTION.Connection, ctx->configuration);
        break;
        
    default:
        break;
    }

    return status;
}


static QUIC_STATUS runMsQuicServer(MsQuicServerContext* ctx) {
    QUIC_STATUS status;
    HQUIC listener = NULL;
    QUIC_ADDR address = {};
    QuicAddrSetFamily(&address, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&address, ctx->port);

    if(QUIC_FAILED(status = ctx->msQuic->ListenerOpen(ctx->registration, (void*)listenerCallback, (void*)ctx, &listener))) {
        fprintf(stderr, "ListenerOpen failed - 0x%x\n", status);
        return status;
    }

    if(QUIC_FAILED(status = ctx->msQuic->ListenerStart(listener, &ctx->alpn, 1, &address))) {
        printf("ListenerStart failed - 0x%x\n", status);
        ctx->msQuic->ListenerClose(listener);
        return status;
    }

    printf("MsQuic server is listening on port %d...\n", ctx->port);
    getchar();

    ctx->msQuic->ListenerClose(listener);
    return QUIC_STATUS_SUCCESS;
} 


int main(int argc, char* argv[]) {
    MsQuicServerContext ctx;
    QUIC_STATUS status = QUIC_STATUS_SUCCESS;
    
    if(QUIC_FAILED(status = initMsQuic(&ctx))) {
        fprintf(stderr, "initMsQuic failed - 0x%x\n", status);
        deinitMsQuic(&ctx);
        return (int)status;
    }

    if(QUIC_FAILED(status = loadConfiguration(&ctx, argc, argv))) {
        deinitMsQuic(&ctx);
        return (int)status;
    }

    status = runMsQuicServer(&ctx);
    deinitMsQuic(&ctx);
    return (int)status;
}
