/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
 * @file capture_tls.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP TLS transport for messages
 *
 * This file contains the functions and structures to manage the SIP messages
 * that use TLS as transport.
 *
 */

#include <unistd.h>
#include "capture.h"
#include "capture_openssl.h"
#include "option.h"
#include "util.h"
#include "sip.h"

struct SSLConnection *connections;

struct CipherSuite TLS_RSA_WITH_AES_128_CBC_SHA =
{ 0x00, 0x2F };
struct CipherSuite TLS_RSA_WITH_AES_256_CBC_SHA =
{ 0x00, 0x35 };

#define TLS_DEBUG 0

void
tls_debug_print_hex (char *desc, const void *ptr, int len) {
#if TLS_DEBUG == 1
    int i;
    uint8_t buff[17];
    uint8_t *data = (uint8_t*)ptr;

    printf ("%s [%d]:\n", desc, len);
    if (len == 0) return;
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                printf (" |%s|\n", buff);
            printf ("|");
        }
        printf (" %02x", data[i]);
        if ((data[i] < 0x20) || (data[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = data[i];
        buff[(i % 16) + 1] = '\0';
    }
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }
    printf (" |%-16s|\n\n", buff);
#endif
}

int
P_hash(const char *digest, unsigned char *dest, int dlen, unsigned char *secret, int sslen,
       unsigned char *seed, int slen)
{
    unsigned char hmac[48];
    uint32_t hlen;
    HMAC_CTX hm;
    const EVP_MD *md = EVP_get_digestbyname(digest);
    uint32_t tmpslen;
    unsigned char tmpseed[slen];
    unsigned char *out = dest;
    int pending = dlen;

    // Copy initial seed
    memcpy(tmpseed, seed, slen);
    tmpslen = slen;

    // Calculate enough data to fill destination
    while (pending > 0) {
        HMAC_Init(&hm, secret, sslen, md);
        HMAC_Update(&hm, tmpseed, tmpslen);
        HMAC_Final(&hm, tmpseed, &tmpslen);

        HMAC_Init(&hm, secret, sslen, md);
        HMAC_Update(&hm, tmpseed, tmpslen);
        HMAC_Update(&hm, seed, slen);
        HMAC_Final(&hm, hmac, &hlen);

        hlen = (hlen > pending) ? pending : hlen;
        memcpy(out, hmac, hlen);
        out += hlen;
        pending -= hlen;
    }
    HMAC_cleanup(&hm);

    return hlen;
}

int
PRF(unsigned char *dest, int dlen, unsigned char *pre_master_secret,
    int plen, unsigned char *label, unsigned char *seed, int slen)
{
    int i;

    // Split the secret by half to generate MD5 and SHA secret parts
    int hplen = plen / 2 + plen % 2;
    unsigned char md5_secret[hplen];
    unsigned char sha_secret[hplen];
    memcpy(md5_secret, pre_master_secret, hplen);
    memcpy(sha_secret, pre_master_secret + plen / 2, plen / 2);

    // This vars will store the values of P_MD5 and P_SHA-1
    unsigned char h_md5[dlen];
    unsigned char h_sha[dlen];

    // Concatenate given seed to the label to get the final seed
    int llen = strlen((const char*) label);
    unsigned char fseed[slen + llen];
    memcpy(fseed, label, llen);
    memcpy(fseed + llen, seed, slen);
    tls_debug_print_hex("hash seed",fseed, slen + llen);

    // Get enough MD5 and SHA1 data to fill output len
    P_hash("MD5", h_md5, dlen, pre_master_secret, hplen, fseed, slen + llen);
    P_hash("SHA1", h_sha, dlen, pre_master_secret + hplen, hplen, fseed, slen + llen);

    // Final output will be MD5 and SHA1 X-ORed
    for (i = 0; i < dlen; i++)
        dest[i] = h_md5[i] ^ h_sha[i];

    tls_debug_print_hex("PRF out", dest, dlen);
    return dlen;
}

int
PRF12(unsigned char *dest, int dlen, unsigned char *pre_master_secret,
        int plen, unsigned char *label, unsigned char *seed, int slen) {

    // Concatenate given seed to the label to get the final seed
    int llen = strlen((const char*) label);
    unsigned char fseed[slen + llen];
    memcpy(fseed, label, llen);
    memcpy(fseed + llen, seed, slen);
    tls_debug_print_hex("hash seed",fseed, slen + llen);

    // Get enough SHA256 data to fill output len
    P_hash("SHA256", dest, dlen, pre_master_secret, plen, fseed, slen + llen);
    tls_debug_print_hex("PRF out", dest, dlen);
    return dlen;
}

struct SSLConnection *
tls_connection_create(struct in_addr caddr, uint16_t cport, struct in_addr saddr, uint16_t sport) {
    struct SSLConnection *conn = NULL;
    conn = sng_malloc(sizeof(struct SSLConnection));

    memcpy(&conn->client_addr, &caddr, sizeof(struct in_addr));
    memcpy(&conn->server_addr, &saddr, sizeof(struct in_addr));
    memcpy(&conn->client_port, &cport, sizeof(uint16_t));
    memcpy(&conn->server_port, &sport, sizeof(uint16_t));

    SSL_library_init();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (!(conn->ssl_ctx = SSL_CTX_new(SSLv23_server_method())))
        return NULL;

    SSL_CTX_use_PrivateKey_file(conn->ssl_ctx, capture_keyfile(),
                                SSL_FILETYPE_PEM);
    if (!(conn->ssl = SSL_new(conn->ssl_ctx)))
        return NULL;

    conn->server_private_key = SSL_get_privatekey(conn->ssl);

    // Add this connection to the list
    conn->next = connections;
    connections = conn;

    return conn;
}

void
tls_connection_destroy(struct SSLConnection *conn)
{
    struct SSLConnection *c;

    // Remove connection from connections list
    if (conn == connections) {
        connections = conn->next;
    } else {
        for (c = connections; c; c = c->next) {
            if (c->next == conn) {
                c->next = conn->next;
                break;
            }
        }
    }

    // Deallocate connection memory
    SSL_CTX_free(conn->ssl_ctx);
    SSL_free(conn->ssl);
    sng_free(conn);
}

/**
 * FIXME Replace this with a tls_load_key function and use it
 * in tls_connection_create.
 *
 * Most probably we only need one context and key for all connections
 */
int
tls_check_keyfile(const char *keyfile)
{
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    char errbuf[1024];

    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (access(capture_keyfile(), R_OK) != 0)
        return 0;

    if (!(ssl_ctx = SSL_CTX_new(SSLv23_server_method())))
        return 0;

    if (!(SSL_CTX_use_PrivateKey_file(ssl_ctx, capture_keyfile(), SSL_FILETYPE_PEM))) {
        unsigned long n = ERR_get_error();
        fprintf(stderr, "%s\n", ERR_error_string(n, errbuf));
        return 0;
    }

    if (!(ssl = SSL_new(ssl_ctx)))
        return 0;

    if (!SSL_get_privatekey(ssl))
        return 0;

    return 1;
}

int
tls_connection_dir(struct SSLConnection *conn, struct in_addr addr, uint16_t port)
{
    if (conn->client_addr.s_addr == addr.s_addr && conn->client_port == port)
        return 0;
    if (conn->server_addr.s_addr == addr.s_addr && conn->server_port == port)
        return 1;
    return -1;
}

struct SSLConnection*
tls_connection_find(struct in_addr src, uint16_t sport, struct in_addr dst, uint16_t dport) {
    struct SSLConnection *conn;

    for (conn = connections; conn; conn = conn->next) {
        if (tls_connection_dir(conn, src, sport) == 0 &&
                tls_connection_dir(conn, dst, dport) == 1) {
            return conn;
        }
        if (tls_connection_dir(conn, src, sport) == 1 &&
                tls_connection_dir(conn, dst, dport) == 0) {
            return conn;
        }
    }
    return NULL;
}

int
tls_process_segment(packet_t *packet, struct tcphdr *tcp)
{
    struct SSLConnection *conn;
    const u_char *payload = packet_payload(packet);
    uint32_t size_payload = packet_payloadlen(packet);
    uint8_t *out;
    uint32_t outl = packet->payload_len;
    out = sng_malloc(outl);
    struct in_addr ip_src, ip_dst;
    uint16_t sport = packet->src.port;
    uint16_t dport = packet->dst.port;

    // Convert addresses
    inet_pton(AF_INET, packet->src.ip, &ip_src);
    inet_pton(AF_INET, packet->dst.ip, &ip_dst);

    // Try to find a session for this ip
    if ((conn = tls_connection_find(ip_src, sport, ip_dst, dport))) {
        // Update last connection direction
        conn->direction = tls_connection_dir(conn, ip_src, sport);

        // Check current connection state
        switch (conn->state) {
            case TCP_STATE_SYN:
                // First SYN received, this package must be SYN/ACK
                if (tcp->th_flags & TH_SYN & ~TH_ACK)
                    conn->state = TCP_STATE_SYN_ACK;
                break;
            case TCP_STATE_SYN_ACK:
                // We expect an ACK packet here
                if (tcp->th_flags & ~TH_SYN & TH_ACK)
                    conn->state = TCP_STATE_ESTABLISHED;
                break;
            case TCP_STATE_ACK:
            case TCP_STATE_ESTABLISHED:
                // Check if we have a SSLv2 Handshake
                if(tls_record_handshake_is_ssl2(conn, payload, size_payload)) {
                    if (tls_process_record_ssl2(conn, payload, size_payload, &out, &outl) != 0)
                        outl = 0;

                } else {
                    // Process data segment!
                    if (tls_process_record(conn, payload, size_payload, &out, &outl) != 0)
                        outl = 0;
                }

                // This seems a SIP TLS packet ;-)
                if ((int32_t) outl > 0) {
                    packet_set_payload(packet, out, outl);
                    packet_set_type(packet, PACKET_SIP_TLS);
                    return 0;
                }
                break;
            case TCP_STATE_FIN:
            case TCP_STATE_CLOSED:
                // We can delete this connection
                tls_connection_destroy(conn);
                break;
        }
    } else {
        if (tcp->th_flags & TH_SYN & ~TH_ACK) {
            // New connection, store it status and leave
            tls_connection_create(ip_src, sport, ip_dst, dport);
        }
    }

    sng_free(out);
    return 0;
}

int
tls_record_handshake_is_ssl2(struct SSLConnection *conn, const uint8_t *payload,
                   const int len)
{
    // This magic belongs to wireshark people <3
    if (len < 3) return 0;
    // v2 client hello should start this way
    if (payload[0] != 0x80) return 0;
    // v2 client hello msg type
    if (payload[2] != 0x01) return 0;
    // Seems SSLv2
    return 1;
}

int
tls_process_record_ssl2(struct SSLConnection *conn, const uint8_t *payload,
                   const int len, uint8_t **out, uint32_t *outl)
{
    int record_len_len;
    uint16 record_len16;
    uint24 record_len24;
    uint8_t record_type;
    const opaque *fragment;
    int flen;

    // No record data here!
    if (len == 0)
        return 0;

    // Record header length
    record_len_len = (payload[0] & 0x80) ? 2 : 3;

    // Two bytes SSLv2 record length field
    if (record_len_len == 2) {
        record_len16.x[0] = (payload[0] & 0x7f) << 8;
        record_len16.x[1] = (payload[1]);
        record_type = payload[2];
        fragment = payload + 3;
        flen = UINT16_INT(record_len16) - 1 /* record type */;
    } else {
        record_len24.x[0] = (payload[0] & 0x3f) << 8;
        record_len24.x[1] = payload[1];
        record_len24.x[2] = payload[2];
        record_type = payload[3];
        fragment = payload + 4;
        flen = UINT24_INT(record_len24) - 1 /* record type */;
    }

    // We only handle Client Hello handshake SSLv2 records
    if (record_type == 0x01 && flen > sizeof(struct ClientHelloSSLv2)) {
        // Client Hello SSLv2
        struct ClientHelloSSLv2 *clienthello = (struct ClientHelloSSLv2 *) fragment;

        // Check we have a TLS handshake
        if (clienthello->client_version.major != 0x03) {
            tls_connection_destroy(conn);
            return 1;
        }

        // Only TLS 1.0, 1.1 or 1.2 connections
        if (clienthello->client_version.minor != 0x01
                && clienthello->client_version.minor != 0x02
                && clienthello->client_version.minor != 0x03) {
            tls_connection_destroy(conn);
            return 1;
        }

        // Store TLS version
        conn->version = clienthello->client_version.minor;

        // Calculate where client random starts
        const opaque *random = fragment + sizeof(struct ClientHelloSSLv2)
                            + UINT16_INT(clienthello->cipherlist_len)
                            + UINT16_INT(clienthello->sessionid_len);

        // Get Client random
        memcpy(&conn->client_random, random, sizeof(struct Random));
    }

    return 0;
}

int
tls_process_record(struct SSLConnection *conn, const uint8_t *payload,
                   const int len, uint8_t **out, uint32_t *outl)
{
    struct TLSPlaintext *record;
    int record_len;
    const opaque *fragment;

    // No record data here!
    if (len == 0)
        return 0;

    // Get Record data
    record = (struct TLSPlaintext *) payload;
    record_len = sizeof(struct TLSPlaintext) + UINT16_INT(record->length);

    // Process record fragment
    if (UINT16_INT(record->length) > 0) {
        // TLSPlaintext fragment pointer
        fragment = (opaque *) payload + sizeof(struct TLSPlaintext);

        switch (record->type) {
            case handshake:
                // Hanshake Record, Try to get MasterSecret data
                if (tls_process_record_handshake(conn, fragment, UINT16_INT(record->length)) != 0)
                    return 1;
                break;
            case change_cipher_spec:
                // From now on, this connection will be encrypted using MasterSecret
                if (conn->client_cipher_ctx.cipher && conn->server_cipher_ctx.cipher)
                    conn->encrypted = 1;
                break;
            case application_data:
                if (conn->encrypted) {
                    // Decrypt application data using MasterSecret
                    tls_process_record_data(conn, fragment, UINT16_INT(record->length), out, outl);
                }
                break;
            default:
                break;
        }
    }

    // MultiRecord packet
    if (len > record_len) {
        *outl = len;
        return tls_process_record(conn, payload + record_len, len - record_len, out, outl);
    }

    return 0;
}

int
tls_process_record_handshake(struct SSLConnection *conn, const opaque *fragment, const int len)
{
    struct Handshake *handshake;
    struct ClientHello *clienthello;
    struct ServerHello *serverhello;
    struct ClientKeyExchange *clientkeyex;
    const opaque *body;

    // Get Handshake data
    handshake = (struct Handshake *) fragment;

    if (UINT24_INT(handshake->length) > 0) {
        // Hanshake body pointer
        body = fragment + sizeof(struct Handshake);

        switch (handshake->type) {
            case hello_request:
                break;
            case client_hello:
                // Store client random
                clienthello = (struct ClientHello *) body;
                memcpy(&conn->client_random, &clienthello->random, sizeof(struct Random));

                // Check we have a TLS handshake
                if (!(clienthello->client_version.major == 0x03)) {
                    tls_connection_destroy(conn);
                    return 1;
                }

                // Only TLS 1.0, 1.1 or 1.2 connections
                if (clienthello->client_version.minor != 0x01
                        && clienthello->client_version.minor != 0x02
                        && clienthello->client_version.minor != 0x03) {
                    tls_connection_destroy(conn);
                    return 1;
                }

                // Store TLS version
                conn->version = clienthello->client_version.minor;
                break;
            case server_hello:
                // Store server random
                serverhello = (struct ServerHello *) body;
                memcpy(&conn->server_random, &serverhello->random, sizeof(struct Random));
                // Get the selected cipher
                memcpy(&conn->cipher_suite,
                       body + sizeof(struct ServerHello) + serverhello->session_id_length,
                       sizeof(uint16_t));
                // Check if we have a handled cipher
                if (tls_connection_load_cipher(conn) != 0) {
                    tls_connection_destroy(conn);
                    return 1;
                }
                break;
            case certificate:
            case certificate_request:
            case server_hello_done:
            case certificate_verify:
                break;
            case client_key_exchange:
                // Decrypt PreMasterKey
                clientkeyex = (struct ClientKeyExchange *) body;

                RSA_private_decrypt(UINT16_INT(clientkeyex->length),
                                    (const unsigned char *) &clientkeyex->exchange_keys,
                                    (unsigned char *) &conn->pre_master_secret,
                                    conn->server_private_key->pkey.rsa, RSA_PKCS1_PADDING);

                tls_debug_print_hex("client_random", &conn->client_random, 32);
                tls_debug_print_hex("server_random", &conn->server_random, 32);

                uint8_t *seed = sng_malloc(sizeof(struct Random) * 2);
                memcpy(seed, &conn->client_random, sizeof(struct Random));
                memcpy(seed + sizeof(struct Random), &conn->server_random, sizeof(struct Random));

                if (conn->version < 3) {
                    // Get MasterSecret
                    PRF((unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                        (unsigned char *) &conn->pre_master_secret, sizeof(struct PreMasterSecret),
                        (unsigned char *) "master secret", seed, sizeof(struct Random) * 2);
                } else {
                    PRF12((unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                          (unsigned char *) &conn->pre_master_secret, sizeof(struct PreMasterSecret),
                          (unsigned char *) "master secret", seed, sizeof(struct Random) * 2);
                }
                tls_debug_print_hex("master_secret", conn->master_secret.random, 48);

                memcpy(seed, &conn->server_random, sizeof(struct Random) * 2);
                memcpy(seed + sizeof(struct Random), &conn->client_random, sizeof(struct Random));

                // Generate MACs, Write Keys and IVs
                if (conn->version < 3) {
                    PRF((unsigned char *) &conn->key_material, sizeof(struct tls_data),
                        (unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                        (unsigned char *) "key expansion", seed, sizeof(struct Random) * 2);
                } else {
                    PRF12((unsigned char *) &conn->key_material, sizeof(struct tls_data),
                          (unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                          (unsigned char *) "key expansion", seed, sizeof(struct Random) * 2);
                }

                tls_debug_print_hex("client_write_MAC_key", conn->key_material.client_write_MAC_key, 20);
                tls_debug_print_hex("server_write_MAC_key", conn->key_material.server_write_MAC_key, 20);
                tls_debug_print_hex("client_write_key",     conn->key_material.client_write_key, 32);
                tls_debug_print_hex("server_write_key",     conn->key_material.server_write_key, 32);
                tls_debug_print_hex("client_write_IV",      conn->key_material.client_write_IV, 16);
                tls_debug_print_hex("server_write_IV",      conn->key_material.server_write_IV, 16);

                // Done with the seed
                sng_free(seed);

                // Create Client decoder
                EVP_CIPHER_CTX_init(&conn->client_cipher_ctx);
                EVP_CipherInit(&conn->client_cipher_ctx, conn->ciph,
                               conn->key_material.client_write_key, conn->key_material.client_write_IV,
                               0);

                EVP_CIPHER_CTX_init(&conn->server_cipher_ctx);
                EVP_CipherInit(&conn->server_cipher_ctx, conn->ciph,
                               conn->key_material.server_write_key, conn->key_material.server_write_IV,
                               0);

                break;
#ifndef OLD_OPENSSL_VERSION
            case new_session_ticket:
#endif
            case finished:
                break;
            default:
                if (conn->encrypted) {
                    // Encrypted Hanshake Message
                    uint8_t *decoded = sng_malloc(len);
                    uint32_t decodedlen = len;
                    tls_process_record_data(conn, fragment, len, &decoded, &decodedlen);
                    sng_free(decoded);
                }
                break;
        }
    }

    return 0;
}

int
tls_process_record_data(struct SSLConnection *conn, const opaque *fragment,
                        const int len, uint8_t **out, uint32_t *outl)
{
    EVP_CIPHER_CTX *evp;
    uint8_t pad;
    size_t flen = len;

    tls_debug_print_hex("Ciphertext", fragment, len);

    if (conn->direction == 0) {
        evp = &conn->client_cipher_ctx;
    } else {
        evp = &conn->server_cipher_ctx;
    }

    // TLS 1.1 and later extract explicit IV
    if (conn->version >= 2 && len > 16) {
        if (conn->direction == 0) {
            EVP_CipherInit(evp, conn->ciph,
                           conn->key_material.client_write_key,
                           fragment, 0);
        } else {
            EVP_CipherInit(evp, conn->ciph,
                           conn->key_material.server_write_key,
                           fragment, 0);
        }
        flen -= 16;
        fragment += 16;
    }

    size_t dlen = len;
    uint8_t *decoded = sng_malloc(dlen);
    EVP_Cipher(evp, decoded, (unsigned char *) fragment, flen);
    tls_debug_print_hex("Plaintext", decoded, flen);

    // Get padding counter and remove from data
    pad = decoded[flen - 1];
    dlen = flen - (pad + 1);
    tls_debug_print_hex("Mac", decoded + (dlen - 20), 20);

    if ((int32_t)dlen > 0 && dlen <= *outl) {
        memcpy(*out, decoded, dlen);
        *outl = dlen - 20 /* Trailing MAC */;
    }

    // Clenaup decoded memory
    sng_free(decoded);
    return *outl;
}

int
tls_connection_load_cipher(struct SSLConnection *conn)
{
    if (conn->cipher_suite.cs1 != 0x00)
        return 1;

    if (conn->cipher_suite.cs2 == TLS_RSA_WITH_AES_256_CBC_SHA.cs2) {
        conn->ciph = EVP_get_cipherbyname("AES256");
    } else if (conn->cipher_suite.cs2 == TLS_RSA_WITH_AES_128_CBC_SHA.cs2) {
        conn->ciph = EVP_get_cipherbyname("AES128");
    } else {
        return 1;
    }

    return 0;
}
