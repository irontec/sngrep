/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file capture_tls.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP TLS messages
 *
 * This file contains the functions and structures to manage the SIP messages
 * that use TLS as transport.
 * 
 */

#ifndef __SNGREP_CAPTURE_TLS_
#define __SNGREP_CAPTURE_TLS_

#include "config.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include "capture.h"

#define UINT16_INT(i) ((i.x[0] << 8) | i.x[1])
#define UINT24_INT(i) ((i.x[0] << 16) | (i.x[1] << 8) | i.x[2])

typedef unsigned char uint8;

typedef struct uint16
{
    unsigned char x[2];
} uint16;

typedef struct uint24
{
    unsigned char x[3];
} uint24;

typedef struct uint32
{
    unsigned char x[4];
} uint32;
typedef unsigned char opaque;

enum SSLConnectionState
{
    TCP_STATE_SYN = 0,
    TCP_STATE_SYN_ACK,
    TCP_STATE_ACK,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN,
    TCP_STATE_CLOSED
};

enum ContentType
{
    change_cipher_spec = SSL3_RT_CHANGE_CIPHER_SPEC,
    alert = SSL3_RT_ALERT,
    handshake = SSL3_RT_HANDSHAKE,
    application_data = SSL3_RT_APPLICATION_DATA
};

enum HandshakeType
{
    hello_request = SSL3_MT_HELLO_REQUEST,
    client_hello = SSL3_MT_CLIENT_HELLO,
    server_hello = SSL3_MT_SERVER_HELLO,
    certificate = SSL3_MT_CERTIFICATE,
    certificate_request = SSL3_MT_CERTIFICATE_REQUEST,
    server_hello_done = SSL3_MT_SERVER_DONE,
    certificate_verify = SSL3_MT_CERTIFICATE_VERIFY,
    client_key_exchange = SSL3_MT_CLIENT_KEY_EXCHANGE,
    finished = SSL3_MT_FINISHED
};

struct ProtocolVersion
{
    uint8 major;
    uint8 minor;
};

struct TLSPlaintext
{
    uint8 type;
    struct ProtocolVersion version;
    uint16 length;
};

struct Handshake
{
    uint8 type;
    uint24 length;
};

struct Random
{
    uint32 gmt_unix_time;
    opaque random_bytes[28];
};

struct ClientHello
{
    struct ProtocolVersion client_version;
    struct Random random;
// SessionID session_id;
// CipherSuite cipher_suite;
// Extension extensions;
};

struct ServerHello
{
    struct ProtocolVersion server_version;
    struct Random random;
// SessionID session_id;
// CipherSuite cipher_suite;
// CompressionMethod compression_method;
};

struct MasterSecret
{
    uint8 random[48];
};

struct PreMasterSecret
{
    struct ProtocolVersion client_version;
    uint8 random[46];
};

struct EncryptedPreMasterSecret
{
    uint8 pre_master_secret[128];
};

struct ClientKeyExchange
{
    uint16 length;
    struct EncryptedPreMasterSecret exchange_keys;
};

struct SSLConnection
{
    //! Connection status
    enum SSLConnectionState state;
    //! Current packet direction
    int direction;
    //! Data is encrypted flag
    int encrypted;

    //! Source and Destiny IP:port 
    struct in_addr client_addr;
    struct in_addr server_addr;
    u_short client_port;
    u_short server_port;

    SSL *ssl;
    SSL_CTX *ssl_ctx;
    EVP_PKEY *server_private_key;
    struct Random client_random;
    struct Random server_random;
    struct PreMasterSecret pre_master_secret;
    struct PreMasterSecret master_secret;

    struct tls_data
    {
        uint8 client_write_MAC_key[20];
        uint8 server_write_MAC_key[20];
        uint8 client_write_key[32];
        uint8 server_write_key[32];
        uint8 client_write_IV[16];
        uint8 server_write_IV[16];
    } key_material;

    EVP_CIPHER_CTX client_cipher_ctx;
    EVP_CIPHER_CTX server_cipher_ctx;

    struct SSLConnection *next;
};

int
P_hash(const char *digest, unsigned char *dest, int dlen, unsigned char *secret, int sslen,
       unsigned char *seed, int slen);

int
PRF(unsigned char *dest, int dlen, unsigned char *pre_master_secret, int plen, unsigned char *label,
    unsigned char *seed, int slen);

struct SSLConnection *
tls_connection_create(struct in_addr caddr, u_short cport, struct in_addr saddr, u_short sport);

void
tls_connection_destroy(struct SSLConnection *conn);

bool
tls_check_keyfile(const char *keyfile);

int
tls_connection_dir(struct SSLConnection *conn, struct in_addr addr, u_short port);

struct SSLConnection*
tls_connection_find(struct in_addr addr, u_short port);

int
tls_process_segment(const struct nread_ip *ip, uint8 **out, int *outl);

int
tls_process_record(struct SSLConnection *conn, const uint8 *payload, const int len, uint8 **out,
                   int *outl);

int
tls_process_record_handshake(struct SSLConnection *conn, const opaque *fragment);

int
tls_process_record_data(struct SSLConnection *conn, const opaque *fragment, const int len,
                        uint8 **out, int *outl);

#endif
