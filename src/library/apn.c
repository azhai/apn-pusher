/*
* Copyright (c) 2013, 2014 Anton Dobkin
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <openssl/err.h>

#include "apn_strings.h"
#include "apn_tokens.h"
#include "apn_memory.h"
#include "apn_version.h"
#include "apn.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#define APN_PAYLOAD_MAX_SIZE  2048

enum __apn_apple_errors {
    APN_APNS_ERR_NO_ERRORS = 0,
    APN_APNS_ERR_PROCESSING_ERROR = 1,
    APN_APNS_ERR_MISSING_DEVICE_TOKEN,
    APN_APNS_ERR_MISSING_TOPIC,
    APN_APNS_ERR_MISSING_PAYLOAD,
    APN_APNS_ERR_INVALID_TOKEN_SIZE,
    APN_APNS_ERR_INVALID_TOPIC_SIZE,
    APN_APNS_ERR_INVALID_PAYLOAD_SIZE,
    APN_APNS_ERR_INVALID_TOKEN,
    APN_APNS_ERR_SERVICE_SHUTDOWN = 10,
    APN_APNS_ERR_NONE = 255
};

struct __apn_apple_server {
    char *host;
    int port;
};

static struct __apn_apple_server __apn_apple_servers[4] = {
    {"gateway.sandbox.push.apple.com", 2195},
    {"gateway.push.apple.com", 2195},
    {"feedback.sandbox.push.apple.com", 2196},
    {"feedback.push.apple.com", 2196}
};

static size_t __apn_create_binary_message(uint8_t *token, const char * const payload, uint32_t id, time_t expiry, apn_notification_priority priority, uint8_t ** message);
static int __apn_password_cd(char *buf, int size, int rwflag, void *password);
static apn_return __apn_connect(const apn_ctx_ref ctx, struct __apn_apple_server server);
static int __ssl_write(const apn_ctx_ref ctx, const uint8_t *message, size_t length);
static int __ssl_read(const apn_ctx_ref ctx, char *buff, size_t length);
static void __apn_parse_apns_error(char *apns_error, uint16_t *errcode, uint32_t *id);

apn_return apn_library_init() {
    static uint8_t library_initialized = 0;
#ifdef _WIN32
    WSADATA wsa_data;
#endif
    if (!library_initialized) {
        SSL_load_error_strings();
        SSL_library_init();
        library_initialized = 1;
#ifdef _WIN32
        if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            errno = APN_ERR_FAILED_INIT;
            return APN_ERROR;
        }
#endif
    }
    return APN_SUCCESS;
}

void apn_library_free() {
    ERR_free_strings();
    EVP_cleanup();
#ifdef _WIN32
    WSACleanup();
#endif
}

apn_ctx_ref apn_init(const char *cert, const char *private_key, const char *private_key_pass) {
    if(APN_ERROR == apn_library_init()) {
        return NULL;
    }
    apn_ctx_ref ctx = malloc(sizeof (apn_ctx));
    if (!ctx) {
        errno = ENOMEM;
        return NULL;
    }
    ctx->sock = -1;
    ctx->ssl = NULL;
    ctx->tokens_count = 0;
    ctx->certificate_file = NULL;
    ctx->private_key_file = NULL;
    ctx->tokens = NULL;
    ctx->feedback = 0;
    ctx->private_key_pass = NULL;
    ctx->mode = APN_MODE_PRODUCTION;

    if (APN_SUCCESS != apn_set_certificate(ctx, cert)) {
        apn_free(&ctx);
        return NULL;
    }

    if (APN_SUCCESS != apn_set_private_key(ctx, private_key, private_key_pass)) {
        apn_free(&ctx);
        return NULL;
    }
    return ctx;
}

void apn_free(apn_ctx_ref *ctx) {
    if (ctx && *ctx) {
        apn_close(*ctx);
        if ((*ctx)->certificate_file) {
            free((*ctx)->certificate_file);
        }
        if ((*ctx)->private_key_file) {
            free((*ctx)->private_key_file);
        }
        if ((*ctx)->private_key_pass) {
            free((*ctx)->private_key_pass);
        }
        apn_tokens_array_free((*ctx)->tokens, (*ctx)->tokens_count);
        free((*ctx));
        *ctx = NULL;
    }
}

void apn_close(apn_ctx_ref ctx) {
    assert(ctx);
    if (ctx->ssl) {
        SSL_shutdown(ctx->ssl);
        SSL_free(ctx->ssl);
        ctx->ssl = NULL;
    }
    if (ctx->sock != -1) {
        CLOSE_SOCKET(ctx->sock);
        ctx->sock = -1;
    }
}

void apn_set_invalid_token_cb(apn_ctx_ref ctx, void (*invalid_token_cb)(char *)) {
    assert(ctx);
    ctx->invalid_token_cb = invalid_token_cb ? invalid_token_cb : NULL;
}

apn_return apn_set_certificate(apn_ctx_ref ctx, const char * const cert) {
    assert(ctx);
    if (ctx->certificate_file) {
        apn_strfree(&ctx->certificate_file);
    }
    if (cert && strlen(cert) > 0) {
        if (NULL == (ctx->certificate_file = apn_strndup(cert, strlen(cert)))) {
            errno = ENOMEM;
            return APN_ERROR;
        }
    }
    return APN_SUCCESS;
}

apn_return apn_set_private_key(apn_ctx_ref ctx, const char * const key, const char * const pass) {
    assert(ctx);
    if (ctx->private_key_file) {
        apn_strfree(&ctx->private_key_file);
    }
    if (key && strlen(key) > 0) {
        if (NULL == (ctx->private_key_file = apn_strndup(key, strlen(key)))) {
            errno = ENOMEM;
            return APN_ERROR;
        }
    }
    if (ctx->private_key_pass) {
        apn_strfree(&ctx->private_key_pass);
    }
    if (pass && strlen(pass) > 0) {
        if (NULL == (ctx->private_key_pass = apn_strndup(pass, strlen(pass)))) {
            errno = ENOMEM;
            return APN_ERROR;
        }
    }
    return APN_SUCCESS;
}

void apn_set_mode(apn_ctx_ref ctx, apn_connection_mode mode) {
    assert(ctx);
    if (mode == APN_MODE_SANDBOX) {
        ctx->mode = APN_MODE_SANDBOX;
    } else {
        ctx->mode = APN_MODE_PRODUCTION;
    }
}

apn_return apn_add_token(apn_ctx_ref ctx, const char * const token) {
    assert(ctx);
    assert(token);
    uint8_t *binary_token = NULL;
    uint8_t **tokens = NULL;

    if (ctx->tokens_count >= UINT32_MAX) {
        errno = APN_ERR_TOKEN_TOO_MANY;
        return APN_ERROR;
    }

    if (!apn_hex_token_is_valid(token)) {
        errno = APN_ERR_TOKEN_INVALID;
        return APN_ERROR;
    }

    tokens = (uint8_t **)apn_realloc(ctx->tokens, (ctx->tokens_count + 1) * sizeof(uint8_t *));
    if (!tokens) {
        errno = ENOMEM;
        return APN_ERROR;
    }
    ctx->tokens = tokens;

    if (!(binary_token = apn_token_hex_to_binary(token))) {
        return APN_ERROR;
    }

    ctx->tokens[ctx->tokens_count] = binary_token;
    ctx->tokens_count++;
    return APN_SUCCESS;
}

void apn_remove_all_tokens(apn_ctx_ref ctx) {
    assert(ctx);
    apn_tokens_array_free(ctx->tokens, ctx->tokens_count);
    ctx->tokens = NULL;
    ctx->tokens_count = 0;
}

apn_connection_mode apn_mode(const apn_ctx_ref ctx) {
    assert(ctx);
    return ctx->mode;
}

const char *apn_certificate(const apn_ctx_ref ctx) {
    assert(ctx);
    return  ctx->certificate_file;
}

const char *apn_private_key(const apn_ctx_ref ctx) {
    assert(ctx);
    return ctx->private_key_file;
}

const char *apn_private_key_pass(const apn_ctx_ref ctx) {
    assert(ctx);
    return ctx->private_key_pass;
}

apn_return apn_connect(const apn_ctx_ref ctx) {
    struct __apn_apple_server server;
    if (ctx->mode == APN_MODE_SANDBOX) {
        server = __apn_apple_servers[0];
    } else {
        server = __apn_apple_servers[1];
    }
    return __apn_connect(ctx, server);
}

apn_return apn_send(const apn_ctx_ref ctx, const apn_payload_ref payload) {
    assert(ctx);
    assert(payload);

    char *json = NULL;
    size_t json_size = 0;

    size_t message_size = 0;
    uint8_t *message = NULL;

    uint8_t **tokens = NULL;
    uint8_t *token = NULL;
    char apple_error[6];
    uint16_t apple_errcode = 0;
    int bytes_read = 0;
    ssize_t bytes_written = 0;
    uint32_t tokens_count = 0;
    uint32_t invalid_id = 0;
    fd_set write_set, read_set;
    int select_returned = 0;
    uint32_t i = 0;
    struct timeval timeout = {10, 0};
    char *invalid_token = NULL;

    if (!ctx->ssl || ctx->feedback) {
        errno = APN_ERR_NOT_CONNECTED;
        return APN_ERROR;
    }

    if (payload->tokens_count > 0 && payload->tokens != NULL) {
        tokens = payload->tokens;
        tokens_count = payload->tokens_count;
    } else if (ctx->tokens_count > 0 && ctx->tokens != NULL) {
        tokens = ctx->tokens;
        tokens_count = ctx->tokens_count;
    }

    if (tokens_count == 0) {
        errno = APN_ERR_TOKEN_IS_NOT_SET;
        return APN_ERROR;
    }

    json = apn_create_json_document_from_payload(payload);
    if (!json) {
        return APN_ERROR;
    }
    json_size = strlen(json);
    if (json_size > APN_PAYLOAD_MAX_SIZE) {
        errno = APN_ERR_INVALID_PAYLOAD_SIZE;
        free(json);
        return APN_ERROR;
    }

    while (1) {
        if (i == tokens_count) {
            break;
        }
        token = tokens[i];
        message_size = __apn_create_binary_message(token, json, i, payload->expiry, payload->priority, &message);
        if (message_size == 0) {
            free(json);
            return APN_ERROR;
        }

        FD_ZERO(&write_set);
        FD_ZERO(&read_set);
        FD_SET(ctx->sock, &write_set);
        FD_SET(ctx->sock, &read_set);

        select_returned = select(ctx->sock + 1, &read_set, &write_set, NULL, &timeout);
        if (select_returned <= 0) {
            if (errno == EINTR) {
                continue;
            }
            errno = APN_ERR_SELECT;
            return APN_ERROR;
        }

        if (FD_ISSET(ctx->sock, &read_set)) {
            bytes_read = __ssl_read(ctx, apple_error, sizeof (apple_error));
            if (bytes_read <= 0) {
                if (message) {
                    free(message);
                }
                free(json);
                return APN_ERROR;
            }
            __apn_parse_apns_error(apple_error, &apple_errcode, &invalid_id);
            if (apple_errcode == APN_ERR_TOKEN_INVALID) {
                invalid_token =  apn_token_binary_to_hex(tokens[invalid_id]);
                if(ctx->invalid_token_cb) {
                    ctx->invalid_token_cb(invalid_token);
                }
            } else {
                free(message);
                free(json);
                errno = apple_errcode;
                return APN_ERROR;
            }
        }

        if (FD_ISSET(ctx->sock, &write_set)) {
            bytes_written = __ssl_write(ctx, message, message_size);
            free(message);
            if (bytes_written <= 0) {
                free(json);
                return APN_ERROR;
            }
            i++;
        }
    }

    free(json);

    timeout.tv_sec = 1;
    for (;;) {
        FD_ZERO(&read_set);
        FD_SET(ctx->sock, &read_set);
        select_returned = select(ctx->sock + 1, &read_set, NULL, NULL, &timeout);
        if (select_returned < 0) {
            if (errno == EINTR) {
                continue;
            }
            errno = APN_ERR_SELECT;
            return APN_ERROR;
        }

        if (select_returned == 0) {
            break;
        }

        if (FD_ISSET(ctx->sock, &read_set)) {
            bytes_read = __ssl_read(ctx, apple_error, sizeof (apple_error));
            if (bytes_read > 0) {
                __apn_parse_apns_error(apple_error, &apple_errcode, &invalid_id);
                if (apple_errcode == APN_ERR_TOKEN_INVALID) {
                    invalid_token =  apn_token_binary_to_hex(tokens[invalid_id]);
                    if(ctx->invalid_token_cb) {
                        ctx->invalid_token_cb(invalid_token);
                    }
                } else {
                    errno = apple_errcode;
                    return APN_ERROR;
                }
            } else {
                return APN_ERROR;;
            }
            break;
        }
    }
    return APN_SUCCESS;
}

apn_return apn_feedback(const apn_ctx_ref ctx, char ***tokens_array, uint32_t *tokens_array_count) {
    assert(ctx);
    assert(tokens_array);
    assert(tokens_array_count);

    char buffer[38]; /* Buffer to read data */
    char *buffer_ref = buffer; /* Pointer to buffer */
    fd_set read_set;
    struct timeval timeout = {3, 0};
    uint16_t token_length = 0;
    uint8_t binary_token[APN_TOKEN_BINARY_SIZE];
    int bytes_read = 0; /* Number of bytes read */
    char **tokens = NULL; /* Array of HEX tokens */
    uint32_t tokens_count = 0; /* Tokens count */
    char *token_hex = NULL; /* Token as HEX string */
    int select_returned = 0;

    if (!ctx->ssl || ctx->feedback) {
        errno = APN_ERR_NOT_CONNECTED;
        return APN_ERROR;
    }

    for (;;) {
        FD_ZERO(&read_set);
        FD_SET(ctx->sock, &read_set);

        select_returned = select(ctx->sock + 1, &read_set, NULL, NULL, &timeout);
        if (select_returned < 0) {
            if (errno == EINTR) {
                continue;
            }
            errno = APN_ERR_SELECT;
            return APN_ERROR;
        }

        if (select_returned == 0) {
            /* select() timed out */
            break;
        }

        if (FD_ISSET(ctx->sock, &read_set)) {
            bytes_read = __ssl_read(ctx, buffer, sizeof (buffer));
            if (bytes_read < 0) {
                return APN_ERROR;
            } else if (bytes_read > 0) {
                buffer_ref += sizeof (uint32_t);
                memcpy(&token_length, buffer_ref, sizeof (token_length));
                buffer_ref += sizeof (token_length);
                token_length = ntohs(token_length);
                memcpy(&binary_token, buffer_ref, sizeof (binary_token));
                token_hex = apn_token_binary_to_hex(binary_token);
                if (token_hex == NULL) {
                    apn_feedback_tokens_array_free(tokens, tokens_count);
                    return APN_ERROR;
                }
                tokens = (char **) apn_realloc(tokens, (tokens_count + 1) * sizeof (char *));
                if (!tokens) {
                    apn_feedback_tokens_array_free(tokens, tokens_count);
                    errno = ENOMEM;
                    return APN_ERROR;
                }
                tokens[tokens_count] = token_hex;
                tokens_count++;
            }
            break;
        }
    }

    *tokens_array = tokens;
    *tokens_array_count = tokens_count;

    return APN_SUCCESS;
}

uint32_t apn_version() {
    return APN_VERSION_NUM;
}

const char * apn_version_string() {
    return APN_VERSION_STRING;
}

void apn_feedback_tokens_array_free(char **tokens_array, uint32_t tokens_array_count) {
    uint32_t i = 0;
    if (tokens_array != NULL && tokens_array_count > 0) {
        for (i = 0; i < tokens_array_count; i++) {
            char *token = *(tokens_array + i);
            free(token);
        }
        free(tokens_array);
    }
}

static int __apn_password_cd(char *buf, int size, int rwflag, void *password) {
    (void) rwflag;
    if (password == NULL) {
        return 0;
    }
#ifdef _WIN32
    strncpy_s(buf, size, (char *) password, size);
#else
    strncpy(buf, (char *) password, size);
#endif
    buf[size - 1] = '\0';

    return (int)strlen(buf);
}

static size_t __apn_create_binary_message(uint8_t *token, const char * const payload, uint32_t id, time_t expiry, apn_notification_priority priority, uint8_t ** message) {
    uint8_t * frame = NULL;
    uint8_t * frame_ref = NULL;
    size_t frame_size = 0;
    size_t payload_size = 0;

    uint32_t id_n = htonl(id); // ID (network ordered)
    uint32_t expiry_n = htonl(expiry); // expiry time (network ordered)

    uint8_t item_id = 1; // Item ID
    uint16_t item_data_size_n = 0; // Item data size (network ordered)

    size_t binary_message_size = 0;
    uint8_t *binary_message = NULL;
    uint8_t *binary_message_ref = NULL;
    uint32_t frame_size_n; // Frame size (network ordered)

    payload_size = strlen(payload);
    frame_size = ((sizeof (uint8_t) + sizeof (uint16_t)) * 5)
            + APN_TOKEN_BINARY_SIZE
            + payload_size
            + sizeof (uint32_t)
            + sizeof (uint32_t)
            + sizeof (uint8_t);

    frame_size_n = htonl(frame_size);

    frame = (uint8_t *) malloc(frame_size);
    if (!frame) {
        errno = ENOMEM;
        *message = NULL;
        return 0;
    }
    frame_ref = frame;

    binary_message_size = frame_size + sizeof (uint32_t) + sizeof (uint8_t);
    binary_message = (uint8_t *) malloc(binary_message_size);
    if (!binary_message) {
        errno = ENOMEM;
        *message = NULL;
        free(frame);
        return 0;
    }
    binary_message_ref = binary_message;

    /* Token */
    *frame_ref++ = item_id++;
    item_data_size_n = htons(APN_TOKEN_BINARY_SIZE);
    memcpy(frame_ref, &item_data_size_n, sizeof (uint16_t));
    frame_ref += sizeof (uint16_t);
    memcpy(frame_ref, token, APN_TOKEN_BINARY_SIZE);
    frame_ref += APN_TOKEN_BINARY_SIZE;

    /* Payload */
    *frame_ref++ = item_id++;
    item_data_size_n = htons(payload_size);
    memcpy(frame_ref, &item_data_size_n, sizeof (uint16_t));
    frame_ref += sizeof (uint16_t);
    memcpy(frame_ref, payload, payload_size);
    frame_ref += payload_size;

    /* Message ID */
    *frame_ref++ = item_id++;
    item_data_size_n = htons(sizeof (uint32_t));
    memcpy(frame_ref, &item_data_size_n, sizeof (uint16_t));
    frame_ref += sizeof (uint16_t);
    memcpy(frame_ref, &id_n, sizeof (uint32_t));
    frame_ref += sizeof (uint32_t);

    /* Expires */
    *frame_ref++ = item_id++;
    item_data_size_n = htons(sizeof (uint32_t));
    memcpy(frame_ref, &item_data_size_n, sizeof (uint16_t));
    frame_ref += sizeof (uint16_t);
    memcpy(frame_ref, &expiry_n, sizeof (uint32_t));
    frame_ref += sizeof (uint32_t);

    /* Priority */
    *frame_ref++ = item_id;
    item_data_size_n = htons(sizeof (uint8_t));
    memcpy(frame_ref, &item_data_size_n, sizeof (uint16_t));
    frame_ref += sizeof (uint16_t);
    *frame_ref = (uint8_t) priority;

    /* Binary message */
    *binary_message_ref++ = 2;

    memcpy(binary_message_ref, &frame_size_n, sizeof (uint32_t));
    binary_message_ref += sizeof (uint32_t);
    memcpy(binary_message_ref, frame, frame_size);

    free(frame);

    *message = binary_message;
    return binary_message_size;
}

static apn_return __apn_connect(const apn_ctx_ref ctx, struct __apn_apple_server server) {
    struct hostent * hostent = NULL;
    struct sockaddr_in socket_address;
    SOCKET sock;
    int sock_flags = 0;
    SSL_CTX *ssl_ctx = NULL;
    char *password = NULL;

    if (!ctx->certificate_file) {
        errno = APN_ERR_CERTIFICATE_IS_NOT_SET;
        return APN_ERROR;
    }
    if (!ctx->private_key_file) {
        errno = APN_ERR_PRIVATE_KEY_IS_NOT_SET;
        return APN_ERROR;
    }

    if (ctx->sock == -1) {
        hostent = gethostbyname(server.host);
        if (!hostent) {
            errno = APN_ERR_COULD_NOT_RESOLVE_HOST;
            return APN_ERROR;
        }

        memset(&socket_address, 0, sizeof (socket_address));
        socket_address.sin_addr = *(struct in_addr*) hostent->h_addr_list[0];
        socket_address.sin_family = AF_INET;
        socket_address.sin_port = htons(server.port);

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            errno = APN_ERR_COULD_NOT_CREATE_SOCKET;
            return APN_ERROR;
        }

        if (connect(sock, (struct sockaddr *) &socket_address, sizeof (socket_address)) < 0) {
            errno = APN_ERR_COULD_NOT_INITIALIZE_CONNECTION;
            return APN_ERROR;
        }

        ctx->sock = sock;
        ssl_ctx = SSL_CTX_new(TLSv1_client_method());

        if (!SSL_CTX_use_certificate_file(ssl_ctx, ctx->certificate_file, SSL_FILETYPE_PEM)) {
            errno = APN_ERR_UNABLE_TO_USE_SPECIFIED_CERTIFICATE;
            SSL_CTX_free(ssl_ctx);
            return APN_ERROR;
        }

        SSL_CTX_set_default_passwd_cb(ssl_ctx, __apn_password_cd);

        if (ctx->private_key_pass) {
            password = apn_strndup(ctx->private_key_pass, strlen(ctx->private_key_pass));
            if (password == NULL) {
                errno = ENOMEM;
                SSL_CTX_free(ssl_ctx);
                return APN_ERROR;
            }
            SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, password);
        } else {
            SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, NULL);
        }

        if (!SSL_CTX_use_PrivateKey_file(ssl_ctx, ctx->private_key_file, SSL_FILETYPE_PEM)) {
            errno = APN_ERR_UNABLE_TO_USE_SPECIFIED_PRIVATE_KEY;
            SSL_CTX_free(ssl_ctx);
            if (password) {
                free(password);
            }
            return APN_ERROR;
        }

        if (password) {
            free(password);
        }

        if (!SSL_CTX_check_private_key(ssl_ctx)) {
            errno = APN_ERR_UNABLE_TO_USE_SPECIFIED_PRIVATE_KEY;
            SSL_CTX_free(ssl_ctx);
            return APN_ERROR;
        }

        ctx->ssl = SSL_new(ssl_ctx);
        SSL_CTX_free(ssl_ctx);

        if (!ctx->ssl) {
            errno = APN_ERR_COULD_NOT_INITIALIZE_SSL_CONNECTION;
            return APN_ERROR;
        }

        if (-1 == SSL_set_fd(ctx->ssl, ctx->sock) ||
                SSL_connect(ctx->ssl) < 1 ) {
            errno = APN_ERR_COULD_NOT_INITIALIZE_SSL_CONNECTION;
            return APN_ERROR;
        }

#ifndef _WIN32
        sock_flags = fcntl(ctx->sock, F_GETFL, 0);
        fcntl(ctx->sock, F_SETFL, sock_flags | O_NONBLOCK);
#else
        sock_flags = 1;
        ioctlsocket(ctx->sock, FIONBIO, (u_long *) & sock_flags);
#endif
    }

    return APN_SUCCESS;
}

static int __ssl_write(const apn_ctx_ref ctx, const uint8_t *message, size_t length) {
    int bytes_written = 0;
    int bytes_written_total = 0;

    while (length > 0) {
        bytes_written = SSL_write(ctx->ssl, message, (int) length);
        if (bytes_written <= 0) {
            switch (SSL_get_error(ctx->ssl, bytes_written)) {
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_READ:
                    continue;
                case SSL_ERROR_SYSCALL:
                    switch (errno) {
                        case EINTR:
                            continue;
                        case EPIPE:
                            errno = APN_ERR_NETWORK_UNREACHABLE;
                            return -1;
                        case ETIMEDOUT:
                            errno = APN_ERR_CONNECTION_TIMEDOUT;
                            return -1;
                        default:
                            errno = APN_ERR_SSL_WRITE_FAILED;
                            return -1;
                    }
                case SSL_ERROR_ZERO_RETURN:
                    errno = APN_ERR_CONNECTION_CLOSED;
                    return -1;
                default:
                    errno = APN_ERR_SSL_WRITE_FAILED;
                    return -1;
            }
        }
        message += bytes_written;
        bytes_written_total += bytes_written;
        length -= bytes_written;
    }
    return bytes_written_total;
}

static int __ssl_read(const apn_ctx_ref ctx, char *buff, size_t length) {
    int read;
    for (;;) {
        read = SSL_read(ctx->ssl, buff, (int) length);
        if (read > 0) {
            break;
        }
        switch (SSL_get_error(ctx->ssl, read)) {
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
                continue;
            case SSL_ERROR_SYSCALL:
                switch (errno) {
                    case EINTR:
                        continue;
                    case EPIPE:
                        errno = APN_ERR_NETWORK_UNREACHABLE;
                        return -1;
                    case ETIMEDOUT:
                        errno = APN_ERR_CONNECTION_TIMEDOUT;
                        return -1;
                    default:
                        errno = APN_ERR_SSL_READ_FAILED;
                        return -1;
                }
            case SSL_ERROR_ZERO_RETURN:
                errno = APN_ERR_CONNECTION_CLOSED;
                return -1;
            default:
                errno = APN_ERR_CONNECTION_TIMEDOUT;
                return -1;
        }
    }
    return read;
}

static void __apn_parse_apns_error(char *apns_error, uint16_t *errcode, uint32_t *id) {
    uint8_t cmd = 0;
    uint8_t apple_error_code = 0;
    uint32_t notification_id = 0;
    memcpy(&cmd, apns_error, sizeof (uint8_t));
    apns_error += sizeof (cmd);
    if (cmd == 8) {
        memcpy(&apple_error_code, apns_error, sizeof (uint8_t));
        apns_error += sizeof (apple_error_code);
        switch (apple_error_code) {
            case APN_APNS_ERR_PROCESSING_ERROR:
                *errcode = APN_ERR_PROCESSING_ERROR;
                break;
            case APN_APNS_ERR_INVALID_PAYLOAD_SIZE:
                *errcode = APN_ERR_INVALID_PAYLOAD_SIZE;
                break;
            case APN_APNS_ERR_SERVICE_SHUTDOWN:
                *errcode = APN_ERR_SERVICE_SHUTDOWN;
                break;
            case APN_APNS_ERR_INVALID_TOKEN:
                *errcode = APN_ERR_TOKEN_INVALID;
                memcpy(&notification_id, apns_error, sizeof (uint32_t));
                *id = notification_id;
                break;
            default: break;
        }
    }
}