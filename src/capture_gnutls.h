/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
 ** In addition, as a special exception, the copyright holders give
 ** permission to link the code of portions of this program with the
 ** OpenSSL library under certain conditions as described in each
 ** individual source file, and distribute linked combinations
 ** including the two.
 ** You must obey the GNU General Public License in all respects
 ** for all of the code used other than OpenSSL.  If you modify
 ** file(s) with this exception, you may extend this exception to your
 ** version of the file(s), but you are not obligated to do so.  If you
 ** do not wish to do so, delete this exception statement from your
 ** version.  If you delete this exception statement from all source
 ** files in the program, then also delete it here.
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
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gcrypt.h>
#include "capture.h"

//! Cast two bytes into decimal (Big Endian)
#define UINT16_INT(i) ((i.x[0] << 8) | i.x[1])
//! Cast three bytes into decimal (Big Endian)
#define UINT24_INT(i) ((i.x[0] << 16) | (i.x[1] << 8) | i.x[2])

//! Three bytes unsigned integer
typedef struct uint16 {
    unsigned char x[2];
} uint16;

//! Three bytes unsigned integer
typedef struct uint24 {
    unsigned char x[3];
} uint24;

//! One byte generic type
typedef unsigned char opaque;

//! SSLConnections states
enum SSLConnectionState {
    //! Initial SYN packet has been received from client
    TCP_STATE_SYN = 0,
    //! SYN/ACK packet has been sent from the server
    TCP_STATE_SYN_ACK,
    //! Client ACK'ed the connection
    TCP_STATE_ACK,
    //! Connection is up, now SSL handshake should start!
    TCP_STATE_ESTABLISHED,
    //! Connection about to end
    TCP_STATE_FIN,
    //! Connection closed
    TCP_STATE_CLOSED
};

//! SSL Encoders algo
enum SSLCipherEncoders {
    ENC_AES         = 1,
    ENC_AES256      = 2
};

//! SSL Digests algo
enum SSLCIpherDigest {
    DIG_SHA1        = 1,
    DIG_SHA256      = 2,
    DIG_SHA384      = 3
};

//! SSL Decode mode
enum SSLCipherMode {
    MODE_CBC,
    MODE_GCM
};

//! ContentType values as defined in RFC5246
enum ContentType {
    change_cipher_spec  = 20,
    alert               = 21,
    handshake           = 22,
    application_data    = 23
};

//! HanshakeType values as defined in RFC5246
enum HandshakeType {
    hello_request       = GNUTLS_HANDSHAKE_HELLO_REQUEST,
    client_hello        = GNUTLS_HANDSHAKE_CLIENT_HELLO,
    server_hello        = GNUTLS_HANDSHAKE_SERVER_HELLO,
    certificate         = GNUTLS_HANDSHAKE_CERTIFICATE_PKT,
    certificate_request = GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST,
    server_hello_done   = GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
    certificate_verify  = GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY,
    client_key_exchange = GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE,
    finished            = GNUTLS_HANDSHAKE_FINISHED
};

//! ProtocolVersion header as defined in RFC5246
struct ProtocolVersion {
    uint8_t major;
    uint8_t minor;
};

//! TLSPlaintext record structure
struct TLSPlaintext {
    uint8_t type;
    struct ProtocolVersion version;
    uint16 length;
};

//! Hanshake record structure
struct Handshake {
    uint8_t type;
    uint24 length;
};

//! Handshake random structure
struct Random {
    uint8_t gmt_unix_time[4];
    uint8_t random_bytes[28];
};

struct CipherSuite {
    uint8_t cs1;
    uint8_t cs2;
};

struct CipherData {
    int num;
    int enc;
    int ivblock;
    int bits;
    int digest;
    int diglen;
    int mode;
};

struct ClientHelloSSLv2 {
    struct ProtocolVersion client_version;
    uint16 cipherlist_len;
    uint16 sessionid_len;
    uint16 random_len;
    // CipherSuite cipher_suite;
    // struct Random random;
};

//! ClientHello type in Handshake records
struct ClientHello {
    struct ProtocolVersion client_version;
    struct Random random;
//    uint8_t session_id_length;
// CipherSuite cipher_suite;
// Extension extensions;
};

//! ServerHello type in Handshake records
struct ServerHello {
    struct ProtocolVersion server_version;
    struct Random random;
    uint8_t session_id_length;
// SessionID session_id;
// CipherSuite cipher_suite;
// CompressionMethod compression_method;
};

struct MasterSecret {
    uint8_t random[48];
};

struct PreMasterSecret {
    struct ProtocolVersion client_version;
    uint8_t random[46];
};

struct EncryptedPreMasterSecret {
    uint8_t pre_master_secret[128];
};

//! ClientKeyExchange type in Handshake records
struct ClientKeyExchange {
    uint16 length;
    struct EncryptedPreMasterSecret exchange_keys;
};

/**
 * Structure to store all information from a TLS
 * connection. This is also used as linked list
 * node.
 */
struct SSLConnection {
    //! Connection status
    enum SSLConnectionState state;
    //! Current packet direction
    int direction;
    //! Data is encrypted flag
    int encrypted;
    //! TLS version
    int version;

    //! Client IP address
    struct in_addr client_addr;
    //! Server IP address
    struct in_addr server_addr;
    //! Client port
    uint16_t client_port;
    //! Server port
    uint16_t server_port;

    gnutls_session_t ssl;
    int ciph;
    gnutls_x509_privkey_t server_private_key;
    struct Random client_random;
    struct Random server_random;
    struct CipherSuite cipher_suite;
    struct CipherData cipher_data;
    struct PreMasterSecret pre_master_secret;
    struct MasterSecret master_secret;

    struct tls_data {
        uint8_t *client_write_MAC_key;
        uint8_t *server_write_MAC_key;
        uint8_t *client_write_key;
        uint8_t *server_write_key;
        uint8_t *client_write_IV;
        uint8_t *server_write_IV;
    } key_material;

    gcry_cipher_hd_t client_cipher_ctx;
    gcry_cipher_hd_t server_cipher_ctx;

    struct SSLConnection *next;
};

/**
 * @brief P_hash expansion function as defined in RFC5246
 *
 * This function will expand Secret and Seed into output using digest
 * hash function. The amount of data generated will be determined by output
 * length (dlen).
 *
 * @param digest Digest name to get the hash function
 * @param dest Destination of hash function result. Memory must be already allocated
 * @param dlen Destination length in bytes
 * @param secret Input for the hash function
 * @param sslen Secret length in bytes
 * @param seed Input for the hash function
 * @param slen Seed length in bytes
 * @return Output bytes
 */
int
P_hash(const char *digest, unsigned char *dest, int dlen, unsigned char *secret, int sslen,
       unsigned char *seed, int slen);

/**
 * @brief Pseudorandom Function as defined in RFC2246
 *
 * This function will generate MasterSecret and KeyMaterial data from PreMasterSecret and Seed
 *
 * @param dest Destination of PRF function result. Memory must be already allocated
 * @param dlen Destination length in bytes
 * @param pre_master_secret PreMasterSecret decrypted from ClientKeyExchange Handhsake record
 * @param pslen PreMasterSecret length in bytes
 * @param label Fixed ASCII string
 * @param seed Concatenation of Random data from Hello Handshake records
 * @param slen Seed length in bytes
 * @return destination length in bytes
 */
int
PRF(struct SSLConnection *conn, unsigned char *dest, int dlen, unsigned char *pre_master_secret,
        int plen, unsigned char *label, unsigned char *seed, int slen);

/**
 * @brief Create a new SSLConnection
 *
 * This will allocate enough memory to store all connection data
 * from a detected SSL connection. This will also add this structure to
 * the connections linked list.
 *
 * @param caddr Client address
 * @param cport Client port
 * @param saddr Server address
 * @param sport Server port
 * @return a pointer to a new allocated SSLConnection structure
 */
struct SSLConnection *
tls_connection_create(struct in_addr caddr, uint16_t cport, struct in_addr saddr, uint16_t sport);

/**
 * @brief Destroys an existing SSLConnection
 *
 * This will free all allocated memory of SSLConnection also removing
 * the connection from connections list.
 *
 * @param conn Existing connection pointer
 */
void
tls_connection_destroy(struct SSLConnection *conn);

/**
 * @brief Check if given keyfile is valid
 *
 * This can be used to check if a file contains valid RSA data
 *
 * @param keyfile Absolute path the keyfile
 * @return 1 if file contains RSA private info, 0 otherwise
 */
int
tls_check_keyfile(const char *keyfile);

/**
 * @brief Determines packet direction
 *
 * Determine if the given address is from client or server.
 *
 * @param conn Existing connection pointer
 * @param addr Client or server address
 * @param port Client or server port
 * @return 0 if address belongs to client, 1 to server or -1 otherwise
 */
int
tls_connection_dir(struct SSLConnection *conn, struct in_addr addr, uint16_t port);

/**
 * @brief Find a connection
 *
 * Try to find connection data for a given address and port.
 * This address:port convination can be the client or server one.
 *
 * @param addr Client or server address
 * @param port Client or server port
 * @return an existing Connection pointer or NULL if not found
 */
struct SSLConnection*
tls_connection_find(struct in_addr src, uint16_t sport, struct in_addr dst, uint16_t dport);

/**
 * @brief Process a TCP segment to check TLS data
 *
 * Check if a TCP segment contains TLS data. In case a TLS record is found
 * process it and return decrypted data if case of application_data record.
 *
 * @param tcp Pointer to tcp header of the packet
 * @param out Pointer to the output char array. Memory must be already allocated
 * @param out Number of bytes returned by this function
 * @return 0 in all cases
 */
int
tls_process_segment(packet_t *packet, struct tcphdr *tcp);

/**
 * @brief Process TLS record data
 *
 * Process a TLS record
 *  - If the record type is Handshake process it in tls_process_record_handshake
 *  - If the record type is Application Data process it in tls_process_record_data
 *
 * @param conn Existing connection pointer
 * @param payload Packet peyload
 * @param len Payload length
 * @param out pointer to store decryted data
 * @param outl decrypted data length
 * @return Decrypted data length
 */
int
tls_process_record(struct SSLConnection *conn, const uint8_t *payload, const int len, uint8_t **out,
                   uint32_t *outl);

/**
 * @brief Check if this Record looks like SSLv2
 *
 * Some devices send the initial ClientHello in a SSLv2 record for compatibility
 * with only SSLv2 protocol.
 *
 * We will only parse SSLv2 Client hello fragments that have TLSv1/SSLv3 content
 * so this does not make us SSLv2 compatible ;-p
 * @param conn Existing connection pointer
 * @param payload Packet peyload
 * @param len Payload length
 * @return 1 if payload seems a SSLv2 record, 0 otherwise
 */
int
tls_record_handshake_is_ssl2(struct SSLConnection *conn, const uint8_t *payload,
                   const int len);
/**
 * @brief Process TLS Handshake SSLv2 record types
 *
 * Process all types of Handshake records to store and compute all required
 * data to decrypt application data packets
 *
 * @param conn Existing connection pointer
 * @param fragment Handshake record data
 * @param len Decimal length of the fragment
 * @return 0 on valid record processed, 1 otherwise
 */
int
tls_process_record_ssl2(struct SSLConnection *conn, const uint8_t *payload,
                   const int len, uint8_t **out, uint32_t *outl);

/**
 * @brief Process TLS Handshake record types
 *
 * Process all types of Handshake records to store and compute all required
 * data to decrypt application data packets
 *
 * @param conn Existing connection pointer
 * @param fragment Handshake record data
 * @param len Decimal length of the fragment
 * @return 0 on valid record processed, 1 otherwise
 */
int
tls_process_record_handshake(struct SSLConnection *conn, const opaque *fragment, const int len);

/**
 * @brief Process TLS ApplicationData record types
 *
 * Process application data record, trying to decrypt it with connection
 * information
 *
 * @param conn Existing connection pointer
 * @param fragment Application record data
 * @param len record length in bytes
 * @param out pointer to store decryted data
 * @param outl decrypted data length
 * @return decoded data length
 */
int
tls_process_record_data(struct SSLConnection *conn, const opaque *fragment, const int len,
                        uint8_t **out, uint32_t *outl);


/**
 * @brief Get the cipher data from the given connection
 *
 * Load cipher pointer depending on the selected cipher in
 * Handshake messages.
 *
 * This function can be used to test is a cipher decrypting is supported
 * @param conn Existing connection pointer
 * @return 0 on valid cipher, 1 otherwise
 */
int
tls_connection_load_cipher(struct SSLConnection *conn);

/**
 * @brief Determine if the given version is valid for us
 *
 * We only handle some SSL/TLS versions. This function will filter out
 * records from unsupported versions.
 *
 * @return 0 if the version is supported, 1 otherwise
 */
int
tls_valid_version(struct ProtocolVersion version);

/**
 * @brief Decrypt data using private RSA key
 *
 * This function code has been taken from wireshark.
 * Because wireshark simply rocks.
 *
 * @param key Imported RSA key data
 * @param flags decrpyt flag (no used)
 * @param ciphertext Encrypted data
 * @param plaintext Decrypted data
 * @return number of bytes of decrypted data
 */
int
tls_privkey_decrypt_data(gnutls_x509_privkey_t key, unsigned int flags,
                const gnutls_datum_t * ciphertext, gnutls_datum_t * plaintext);

#endif
