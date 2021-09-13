/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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

#ifndef __SNGREP_CAPTURE_TLS_H__
#define __SNGREP_CAPTURE_TLS_H__

#include "config.h"
#include <glib.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gcrypt.h>

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_TLS packet_dissector_tls_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorTls, packet_dissector_tls, PACKET_DISSECTOR, TLS, PacketDissector)

//! Error reporting domain
#define TLS_ERROR packet_tls_error_quark()
//! Cast two bytes into decimal (Big Endian)
#define UINT16_INT(i) ((i.x[0] << 8) | i.x[1])
//! Cast three bytes into decimal (Big Endian)
#define UINT24_INT(i) ((i.x[0] << 16) | (i.x[1] << 8) | i.x[2])

enum tls_errors
{
    TLS_ERROR_KEYFILE_EMTPY,
    TLS_ERROR_PRIVATE_INIT,
    TLS_ERROR_PRIVATE_LOAD
};

typedef struct _DissectorTlsData DissectorTlsData;
typedef struct _SSLConnection SSLConnection;

struct _PacketDissectorTls
{
    //! Parent structure
    PacketDissector parent;
    //! List of known TLS connections
    GSList *connections;
};

//! Three bytes unsigned integer
typedef struct uint16
{
    unsigned char x[2];
} uint16;

//! Three bytes unsigned integer
typedef struct uint24
{
    unsigned char x[3];
} uint24;

//! One byte generic type
typedef unsigned char opaque;

//! SSLConnections states
enum SSLConnectionState
{
    TCP_STATE_SYN = 0,        //! Initial SYN packet has been received from client
    TCP_STATE_SYN_ACK,        //! SYN/ACK packet has been sent from the server
    TCP_STATE_ACK,            //! Client ACK'ed the connection
    TCP_STATE_ESTABLISHED,    //! Connection is up, now SSL handshake should start!
    TCP_STATE_FIN,            //! Connection about to end
    TCP_STATE_CLOSED          //! Connection closed
};

//! SSL Encoders algorithm
enum SSLCipherEncoders
{
    ENC_AES = 1,
    ENC_AES256 = 2
};

//! SSL Digests algorithm
enum SSLCIpherDigest
{
    DIG_SHA1 = 1,
    DIG_SHA256 = 2,
    DIG_SHA384 = 3
};

//! SSL Decode mode
enum SSLCipherMode
{
    MODE_UNKNOWN,
    MODE_CBC,
    MODE_GCM
};

//! ContentType values as defined in RFC5246
enum ContentType
{
    CHANGE_CIPHER_SPEC = 20,
    ALERT = 21,
    HANDSHAKE = 22,
    APPLICATION_DATA = 23
};

//! ProtocolVersion header as defined in RFC5246
struct ProtocolVersion
{
    guint8 major;
    guint8 minor;
};

//! TLSPlaintext record structure
struct TLSPlaintext
{
    guint8 type;
    struct ProtocolVersion version;
    uint16 length;
};

//! Hanshake record structure
struct Handshake
{
    guint8 type;
    uint24 length;
};

//! Handshake random structure
struct Random
{
    guint8 gmt_unix_time[4];
    guint8 random_bytes[28];
};

struct CipherSuite
{
    guint8 cs1;
    guint8 cs2;
};

struct CipherData
{
    guint num;
    int enc;
    int ivblock;
    int bits;
    int digest;
    guint diglen;
    int mode;
};

struct ClientHelloSSLv2
{
    struct ProtocolVersion client_version;
    uint16 cipherlist_len;
    uint16 sessionid_len;
    uint16 random_len;
    // CipherSuite cipher_suite;
    // struct Random random;
};

//! ClientHello type in Handshake records
struct ClientHello
{
    struct ProtocolVersion client_version;
    struct Random random;
//    guint8 session_id_length;
// CipherSuite cipher_suite;
// Extension extensions;
};

//! ServerHello type in Handshake records
struct ServerHello
{
    struct ProtocolVersion server_version;
    struct Random random;
    guint8 session_id_length;
// SessionID session_id;
// CipherSuite cipher_suite;
// CompressionMethod compression_method;
};

struct MasterSecret
{
    guint8 random[48];
};

struct PreMasterSecret
{
    struct ProtocolVersion client_version;
    guint8 random[46];
};

//! ClientKeyExchange type in Handshake records
struct ClientKeyExchange
{
    uint16 length;
    guint8 pre_master_secret[];
};

/**
 * Structure to store all information from a TLS
 * connection. This is also used as linked list
 * node.
 */
struct _SSLConnection
{
    //! Connection status
    enum SSLConnectionState state;
    //! Current packet direction
    int direction;
    //! Data is encrypted flag
    int encrypted;
    //! TLS version
    int version;

    //! Client IP address and port
    Address client_addr;
    //! Server IP address and port
    Address server_addr;

    gnutls_session_t ssl;
    int ciph;
    gnutls_x509_privkey_t server_private_key;
    struct Random client_random;
    struct Random server_random;
    struct CipherSuite cipher_suite;
    struct CipherData cipher_data;
    struct PreMasterSecret pre_master_secret;
    struct MasterSecret master_secret;

    struct tls_data
    {
        guint8 *client_write_MAC_key;
        guint8 *server_write_MAC_key;
        guint8 *client_write_key;
        guint8 *server_write_key;
        guint8 *client_write_IV;
        guint8 *server_write_IV;
    } key_material;

    gcry_cipher_hd_t client_cipher_ctx;
    gcry_cipher_hd_t server_cipher_ctx;

    struct SSLConnection *next;
};

/**
 * @brief Check if given keyfile is valid
 *
 * This can be used to check if a file contains valid RSA data
 *
 * @param keyfile Absolute path the keyfile
 * @return 1 if file contains RSA private info, 0 otherwise
 */
gboolean
packet_tls_privkey_check(const gchar *keyfile, GError **error);

PacketDissector *
packet_dissector_tls_new();

G_END_DECLS

#endif  /* __SNGREP_CAPTURE_TLS_H__ */
