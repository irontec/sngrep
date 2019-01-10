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
 ****************************************************************************/
/**
 * @file packet_tls.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage TLS transport for messages
 *
 * This file contains the functions and structures to manage SSL protocol.
 *
 */
#include "config.h"
#include <glib.h>
#include <gnutls/gnutls.h>
#include "glib-extra.h"
#include "capture/capture.h"
#include "capture/address.h"
#include "capture/dissector.h"
#include "packet_ip.h"
#include "packet_tcp.h"
#include "packet_tls.h"

struct CipherData ciphers[] = {
    /*  { number, encoder,    ivlen, bits, digest, diglen, mode }, */
    { 0x002F, ENC_AES,    16, 128, DIG_SHA1,   20, MODE_CBC },   /* TLS_RSA_WITH_AES_128_CBC_SHA     */
    { 0x0035, ENC_AES256, 16, 256, DIG_SHA1,   20, MODE_CBC },   /* TLS_RSA_WITH_AES_256_CBC_SHA     */
    { 0x009d, ENC_AES256, 4,  256, DIG_SHA384, 48, MODE_GCM },   /* TLS_RSA_WITH_AES_256_GCM_SHA384  */
    { 0,      0,          0,  0,   0,          0,  0 }
};


GQuark
packet_tls_error_quark()
{
    return g_quark_from_static_string("sngrep-gnutls");
}

#define TLS_DEBUG 0

#if TLS_DEBUG == 1
static void
packet_tls_debug_print_hex(gchar *desc, gpointer ptr, gint len)
{
    gint i;
    guchar buff[17];
    guchar *data = (guchar *) ptr;

    printf("%s [%d]:\n", desc, len);
    if (len == 0) return;
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                printf(" |%s|\n", buff);
            printf("|");
        }
        printf(" %02x", data[i]);
        if ((data[i] < 0x20) || (data[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = data[i];
        buff[(i % 16) + 1] = '\0';
    }
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }
    printf(" |%-16s|\n\n", buff);
}
#else

static void
packet_tls_debug_print_hex(G_GNUC_UNUSED gchar *desc,
                           G_GNUC_UNUSED gpointer ptr,
                           G_GNUC_UNUSED gint len)
{
}

#endif

static int
packet_tls_hash_function(const gchar *digest, guchar *dest, gint dlen, guchar *secret, gint sslen, guchar *seed,
                         int slen)
{
    guchar hmac[48];
    uint32_t hlen;
    gcry_md_hd_t md;
    uint32_t tmpslen;
    guchar tmpseed[slen];
    guchar *out = dest;
    gint pending = dlen;
    gint algo = gcry_md_map_name(digest);
    gint algolen = gcry_md_get_algo_dlen(algo);

    // Copy initial seed
    memcpy(tmpseed, seed, slen);
    tmpslen = slen;

    // Calculate enough data to fill destination
    while (pending > 0) {
        gcry_md_open(&md, algo, GCRY_MD_FLAG_HMAC);
        gcry_md_setkey(md, secret, sslen);
        gcry_md_write(md, tmpseed, tmpslen);
        memcpy(tmpseed, gcry_md_read(md, algo), algolen);
        tmpslen = algolen;
        gcry_md_close(md);

        gcry_md_open(&md, algo, GCRY_MD_FLAG_HMAC);
        gcry_md_setkey(md, secret, sslen);
        gcry_md_write(md, tmpseed, tmpslen);
        gcry_md_write(md, seed, slen);
        memcpy(hmac, gcry_md_read(md, algo), algolen);
        hlen = algolen;

        hlen = ((gint) hlen > pending) ? pending : (gint) hlen;
        memcpy(out, hmac, hlen);
        out += hlen;
        pending -= hlen;
    }

    return hlen;
}

static int
packet_tls_prf_function(SSLConnection *conn,
                        unsigned char *dest, int dlen, unsigned char *pre_master_secret,
                        int plen, unsigned char *label, unsigned char *seed, int slen)
{
    int i;

    if (conn->version < 3) {
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
        int llen = strlen((const char *) label);
        unsigned char fseed[slen + llen];
        memcpy(fseed, label, llen);
        memcpy(fseed + llen, seed, slen);

        // Get enough MD5 and SHA1 data to fill output len
        packet_tls_hash_function("MD5", h_md5, dlen, pre_master_secret, hplen, fseed, slen + llen);
        packet_tls_hash_function("SHA1", h_sha, dlen, pre_master_secret + hplen, hplen, fseed, slen + llen);

        // Final output will be MD5 and SHA1 X-ORed
        for (i = 0; i < dlen; i++)
            dest[i] = h_md5[i] ^ h_sha[i];

    } else {
        // Concatenate given seed to the label to get the final seed
        int llen = strlen((const char *) label);
        unsigned char fseed[slen + llen];
        memcpy(fseed, label, llen);
        memcpy(fseed + llen, seed, slen);

        // Get enough SHA data to fill output len
        switch (conn->cipher_data.digest) {
            case DIG_SHA1:
                packet_tls_hash_function("SHA256", dest, dlen, pre_master_secret, plen, fseed, slen + llen);
                break;
            case DIG_SHA256:
                packet_tls_hash_function("SHA256", dest, dlen, pre_master_secret, plen, fseed, slen + llen);
                break;
            case DIG_SHA384:
                packet_tls_hash_function("SHA384", dest, dlen, pre_master_secret, plen, fseed, slen + llen);
                break;
            default:
                break;
        }
    }

    packet_tls_debug_print_hex("PRF out", dest, dlen);
    return dlen;
}

static gboolean
packet_tls_valid_version(struct ProtocolVersion version)
{

    switch (version.major) {
        case 0x03:
            switch (version.minor) {
                case 0x01:
                case 0x02:
                case 0x03:
                    return TRUE;
                default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
}

/**
 * FIXME Replace this with a tls_load_key function and use it
 * in tls_connection_create.
 *
 * Most probably we only need one context and key for all connections
 */
gboolean
packet_tls_privkey_check(const gchar *keyfile, GError **error)
{
    gnutls_x509_privkey_t key;
    gnutls_datum_t keycontent = { NULL, 0 };
    int ret;

    gnutls_global_init();

    if (!g_file_get_contents(keyfile, (gchar **) &keycontent.data, (gsize *) &keycontent.size, error)) {
        return FALSE;
    }

    // Check we have read something from keyfile
    if (keycontent.size == 0) {
        g_set_error(error,
                    TLS_ERROR,
                    TLS_ERROR_KEYFILE_EMTPY,
                    "Unable to read keyfile contents");
        return FALSE;
    }

    // Initialize keyfile structure
    ret = gnutls_x509_privkey_init(&key);
    if (ret < GNUTLS_E_SUCCESS) {
        g_set_error(error,
                    TLS_ERROR,
                    TLS_ERROR_PRIVATE_INIT,
                    "Unable to initializing keyfile: %s",
                    gnutls_strerror(ret));
        return FALSE;
    }

    // Import RSA keyfile
    ret = gnutls_x509_privkey_import(key, &keycontent, GNUTLS_X509_FMT_PEM);
    if (ret < GNUTLS_E_SUCCESS) {
        g_set_error(error,
                    TLS_ERROR,
                    TLS_ERROR_PRIVATE_LOAD,
                    "Unable to loading keyfile: %s",
                    gnutls_strerror(ret));
        return FALSE;
    }

    return TRUE;
}

static int
packet_tls_privkey_decrypt_data(gnutls_x509_privkey_t key, G_GNUC_UNUSED unsigned int flags,
                                const gnutls_datum_t *ciphertext, gnutls_datum_t *plaintext)
{
    gsize decr_len = 0, i = 0;
    gcry_sexp_t s_data = NULL, s_plain = NULL;
    gcry_mpi_t encr_mpi = NULL, text = NULL;
    gsize tmp_size;
    gnutls_datum_t rsa_datum[6];
    gcry_mpi_t rsa_params[6];
    gcry_sexp_t rsa_priv_key = NULL;

    // Extract data from RSA key
    gnutls_x509_privkey_export_rsa_raw(key,
                                       &rsa_datum[0], &rsa_datum[1], &rsa_datum[2],
                                       &rsa_datum[3], &rsa_datum[4], &rsa_datum[5]);

    // Convert to RSA params
    for (i = 0; i < 6; i++) {
        gcry_mpi_scan(&rsa_params[i], GCRYMPI_FMT_USG, rsa_datum[i].data, rsa_datum[i].size, &tmp_size);
    }

    if (gcry_mpi_cmp(rsa_params[3], rsa_params[4]) > 0)
        gcry_mpi_swap(rsa_params[3], rsa_params[4]);

    // Convert to sexp
    gcry_mpi_invm(rsa_params[5], rsa_params[3], rsa_params[4]);
    gcry_sexp_build(&rsa_priv_key, NULL,
                    "(private-key(rsa((n%m)(e%m)(d%m)(p%m)(q%m)(u%m))))",
                    rsa_params[0], rsa_params[1], rsa_params[2],
                    rsa_params[3], rsa_params[4], rsa_params[5]);

    // Free not longer required data
    for (i = 0; i < 6; i++) {
        gcry_mpi_release(rsa_params[i]);
        gnutls_free(rsa_datum[i].data);
    }

    gcry_mpi_scan(&encr_mpi, GCRYMPI_FMT_USG, ciphertext->data, ciphertext->size, NULL);
    gcry_sexp_build(&s_data, NULL, "(enc-val(rsa(a%m)))", encr_mpi);
    gcry_pk_decrypt(&s_plain, s_data, rsa_priv_key);
    text = gcry_sexp_nth_mpi(s_plain, 0, 0);
    gcry_mpi_print(GCRYMPI_FMT_USG, NULL, 0, &decr_len, text);
    gcry_mpi_print(GCRYMPI_FMT_USG, ciphertext->data, ciphertext->size, &decr_len, text);

    int pad = 0;
    for (i = 1; i < decr_len; i++) {
        if (ciphertext->data[i] == 0) {
            pad = i + 1;
            break;
        }
    }

    plaintext->size = decr_len - pad;
    plaintext->data = gnutls_malloc(plaintext->size);
    memmove(plaintext->data, ciphertext->data + pad, plaintext->size);
    gcry_sexp_release(s_data);
    gcry_sexp_release(s_plain);
    gcry_mpi_release(encr_mpi);
    gcry_mpi_release(text);
    return (int) decr_len;
}

static gboolean
packet_tls_connection_load_cipher(SSLConnection *conn)
{
    guint ciphnum = (conn->cipher_suite.cs1 << 8) | conn->cipher_suite.cs2;

    // Check if this is one of the supported ciphers
    for (guint i = 0; ciphers[i].enc; i++) {
        if (ciphnum == ciphers[i].num) {
            conn->cipher_data = ciphers[i];
            break;
        }
    }

    // Set proper cipher encoder
    switch (conn->cipher_data.enc) {
        case ENC_AES:
            conn->ciph = gcry_cipher_map_name("AES");
            break;
        case ENC_AES256:
            conn->ciph = gcry_cipher_map_name("AES256");
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

static SSLConnection *
packet_tls_connection_create(Address caddr, Address saddr)
{
    SSLConnection *conn = NULL;
    gnutls_datum_t keycontent = { NULL, 0 };
    FILE *keyfp;
    gnutls_x509_privkey_t spkey;
    int ret;

    // Allocate memory for this connection
    conn = g_malloc0(sizeof(SSLConnection));

    conn->client_addr = caddr;
    conn->server_addr = saddr;

    gnutls_global_init();

    if (gnutls_init(&conn->ssl, GNUTLS_SERVER) < GNUTLS_E_SUCCESS)
        return NULL;

    if (!(keyfp = fopen(capture_keyfile(capture_manager()), "rb")))
        return NULL;

    fseek(keyfp, 0, SEEK_END);
    keycontent.size = ftell(keyfp);
    fseek(keyfp, 0, SEEK_SET);
    keycontent.data = g_malloc0(keycontent.size);
    fread(keycontent.data, 1, keycontent.size, keyfp);
    fclose(keyfp);

    gnutls_x509_privkey_init(&spkey);

    // Import PEM key data
    ret = gnutls_x509_privkey_import(spkey, &keycontent, GNUTLS_X509_FMT_PEM);
    if (ret != GNUTLS_E_SUCCESS)
        return NULL;

    g_free(keycontent.data);

    // Check this is a valid RSA key
    if (gnutls_x509_privkey_get_pk_algorithm(spkey) != GNUTLS_PK_RSA)
        return NULL;

    // Store this key into the connection
    conn->server_private_key = spkey;

    return conn;
}

static void
packet_tls_connection_destroy(SSLConnection *conn)
{
    // Deallocate connection memory
    gnutls_deinit(conn->ssl);
    g_free(conn->key_material.client_write_MAC_key);
    g_free(conn->key_material.server_write_MAC_key);
    g_free(conn->key_material.client_write_IV);
    g_free(conn->key_material.server_write_IV);
    g_free(conn->key_material.client_write_key);
    g_free(conn->key_material.server_write_key);
    g_free(conn);
}

static int
packet_tls_connection_dir(SSLConnection *conn, Address addr)
{
    if (addressport_equals(conn->client_addr, addr))
        return 0;
    if (addressport_equals(conn->server_addr, addr))
        return 1;
    return -1;
}

static SSLConnection *
packet_tls_connection_find(PacketParser *parser, Address src, Address dst)
{
    DissectorTlsData *priv = g_ptr_array_index(parser->dissectors, PACKET_TLS);
    g_return_val_if_fail(priv != NULL, NULL);

    for (GSList *l = priv->connections; l != NULL; l = l->next) {
        SSLConnection *conn = l->data;

        if (packet_tls_connection_dir(conn, src) == 0 &&
            packet_tls_connection_dir(conn, dst) == 1) {
            return conn;
        }
        if (packet_tls_connection_dir(conn, src) == 1 &&
            packet_tls_connection_dir(conn, dst) == 0) {
            return conn;
        }
    }
    return NULL;
}


static gboolean
packet_tls_record_handshake_is_ssl2(G_GNUC_UNUSED SSLConnection *conn,
                                    const GByteArray *data)
{
    g_return_val_if_fail(data != NULL, FALSE);

    // This magic belongs to wireshark people <3
    if (data->len < 3) return 0;
    // v2 client hello should start this way
    if (data->data[0] != 0x80) return 0;
    // v2 client hello msg type
    if (data->data[2] != 0x01) return 0;
    // Seems SSLv2
    return 1;
}


static GByteArray *
packet_tls_process_record_decode(SSLConnection *conn, GByteArray *data)
{
    gcry_cipher_hd_t *evp;
    guint8 nonce[16] = { 0 };

    packet_tls_debug_print_hex("Ciphertext", data->data, data->len);

    if (conn->direction == 0) {
        evp = &conn->client_cipher_ctx;
    } else {
        evp = &conn->server_cipher_ctx;
    }

    if (conn->cipher_data.mode == MODE_CBC) {
        // TLS 1.1 and later extract explicit IV
        if (conn->version >= 2 && data->len > 16) {
            gcry_cipher_setiv(*evp, data->data, 16);
            g_byte_array_remove_range(data, 0, 16);
        }
    }

    if (conn->cipher_data.mode == MODE_GCM) {
        if (conn->direction == 0) {
            memcpy(nonce, conn->key_material.client_write_IV, conn->cipher_data.ivblock);
            memcpy(nonce + conn->cipher_data.ivblock, data->data, 8);
            nonce[15] = 2;
        } else {
            memcpy(nonce, conn->key_material.server_write_IV, conn->cipher_data.ivblock);
            memcpy(nonce + conn->cipher_data.ivblock, data->data, 8);
            nonce[15] = 2;
        }
        gcry_cipher_setctr(*evp, nonce, sizeof(nonce));
        g_byte_array_remove_range(data, 0, 8);
    }

    GByteArray *out = g_byte_array_sized_new(data->len);
    g_byte_array_set_size(out, data->len);
    gcry_cipher_decrypt(*evp, out->data, out->len, data->data, data->len);
    packet_tls_debug_print_hex("Plaintext", out->data, out->len);

    // Strip mac from the decoded data
    if (conn->cipher_data.mode == MODE_CBC) {
        // Get padding counter and remove from data
        guint8 pad = out->data[out->len - 1];
        g_byte_array_set_size(out, out->len - pad - 1);

        // Remove mac from data
        guint mac_len = conn->cipher_data.diglen;
        packet_tls_debug_print_hex("Mac", out->data + out->len - mac_len - 1, mac_len);
        g_byte_array_set_size(out, out->len - mac_len);
    }

    // Strip auth tag from decoded data
    if (conn->cipher_data.mode == MODE_GCM) {
        g_byte_array_remove_range(out, out->len - 16, 16);
    }

    // Return decoded data
    return out;
}

static GByteArray *
packet_tls_process_record_ssl2(SSLConnection *conn, GByteArray *data)
{
    int record_len_len;
    uint32_t record_len;
    uint8_t record_type;
    const opaque *fragment;
    int flen;

    // No record data here!
    if (data->len == 0)
        return NULL;

    // Record header length
    record_len_len = (data->data[0] & 0x80) ? 2 : 3;

    // Two bytes SSLv2 record length field
    if (record_len_len == 2) {
        record_len = (data->data[0] & 0x7f) << 8;
        record_len += (data->data[1]);
        record_type = data->data[2];
        fragment = data->data + 3;
        flen = record_len - 1 /* record type */;
    } else {
        record_len = (data->data[0] & 0x3f) << 8;
        record_len += data->data[1];
        record_len += data->data[2];
        record_type = data->data[3];
        fragment = data->data + 4;
        flen = record_len - 1 /* record type */;
    }

    // We only handle Client Hello handshake SSLv2 records
    if (record_type == 0x01 && (guint) flen > sizeof(struct ClientHelloSSLv2)) {
        // Client Hello SSLv2
        struct ClientHelloSSLv2 *clienthello = (struct ClientHelloSSLv2 *) fragment;

        // Store TLS version
        conn->version = clienthello->client_version.minor;

        // Calculate where client random starts
        const opaque *random = fragment + sizeof(struct ClientHelloSSLv2)
                               + UINT16_INT(clienthello->cipherlist_len)
                               + UINT16_INT(clienthello->sessionid_len);

        // Get Client random
        memcpy(&conn->client_random, random, sizeof(struct Random));
    }

    return data;
}

static gboolean
packet_tls_process_record_client_hello(SSLConnection *conn, GByteArray *data)
{
    // Store client random
    struct ClientHello clienthello;
    memcpy(&clienthello, data->data, sizeof(struct ClientHello));

    // Store client random
    memcpy(&conn->client_random, &clienthello.random, sizeof(struct Random));

    // Check we have a TLS handshake
    if (!packet_tls_valid_version(clienthello.client_version)) {
        return FALSE;
    }

    // Store TLS version
    conn->version = clienthello.client_version.minor;

    return TRUE;
}

static gboolean
packet_tls_process_record_server_hello(SSLConnection *conn, GByteArray *data)
{
    // Store server random
    struct ServerHello serverhello;
    memcpy(&serverhello, data->data, sizeof(struct ServerHello));

    memcpy(&conn->server_random, &serverhello.random, sizeof(struct Random));

    // Get the selected cipher
    memcpy(&conn->cipher_suite,
           data->data + sizeof(struct ServerHello) + serverhello.session_id_length,
           sizeof(guint16));

    // Check if we have a handled cipher
    return packet_tls_connection_load_cipher(conn);
}

static gboolean
packet_tls_process_record_key_exchange(SSLConnection *conn, GByteArray *data)
{
    // Decrypt PreMasterKey
    struct ClientKeyExchange clientkeyex;
    memcpy(&clientkeyex, data->data, sizeof(struct ClientKeyExchange));

    gnutls_datum_t exkeys, pms;
    exkeys.size = UINT16_INT(clientkeyex.length);
    exkeys.data = (unsigned char *) &clientkeyex.exchange_keys;
    packet_tls_debug_print_hex("exchange keys", exkeys.data, exkeys.size);

    packet_tls_privkey_decrypt_data(conn->server_private_key, 0, &exkeys, &pms);
    if (!pms.data) return FALSE;

    memcpy(&conn->pre_master_secret, pms.data, pms.size);
    packet_tls_debug_print_hex("pre_master_secret", pms.data, pms.size);
    packet_tls_debug_print_hex("client_random", &conn->client_random, sizeof(struct Random));
    packet_tls_debug_print_hex("server_random", &conn->server_random, sizeof(struct Random));

    // Get MasterSecret
    uint8_t *seed = g_malloc0(sizeof(struct Random) * 2);
    memcpy(seed, &conn->client_random, sizeof(struct Random));
    memcpy(seed + sizeof(struct Random), &conn->server_random, sizeof(struct Random));
    packet_tls_prf_function(conn, (unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                            (unsigned char *) &conn->pre_master_secret, sizeof(struct PreMasterSecret),
                            (unsigned char *) "master secret", seed, sizeof(struct Random) * 2);

    packet_tls_debug_print_hex("master_secret", conn->master_secret.random, sizeof(struct MasterSecret));

    memcpy(seed, &conn->server_random, sizeof(struct Random) * 2);
    memcpy(seed + sizeof(struct Random), &conn->client_random, sizeof(struct Random));

    int key_material_len = 0;
    key_material_len += conn->cipher_data.diglen * 2;
    key_material_len += conn->cipher_data.ivblock * 2;
    key_material_len += conn->cipher_data.bits / 4;

    // Generate MACs, Write Keys and IVs
    uint8_t *key_material = g_malloc0(key_material_len);
    packet_tls_prf_function(conn, key_material, key_material_len,
                            (unsigned char *) &conn->master_secret, sizeof(struct MasterSecret),
                            (unsigned char *) "key expansion", seed, sizeof(struct Random) * 2);

    // Get write mac keys
    if (conn->cipher_data.mode == MODE_GCM) {
        // AEAD ciphers
        conn->key_material.client_write_MAC_key = 0;
        conn->key_material.server_write_MAC_key = 0;
    } else {
        // Copy prf output to ssl connection key material
        int mk_len = conn->cipher_data.diglen;
        conn->key_material.client_write_MAC_key = g_malloc0(mk_len);
        memcpy(conn->key_material.client_write_MAC_key, key_material, mk_len);
        packet_tls_debug_print_hex("client_write_MAC_key", key_material, mk_len);
        key_material += mk_len;
        conn->key_material.server_write_MAC_key = g_malloc0(mk_len);
        packet_tls_debug_print_hex("server_write_MAC_key", key_material, mk_len);
        memcpy(conn->key_material.server_write_MAC_key, key_material, mk_len);
        key_material += mk_len;
    }

    // Get write keys
    int wk_len = conn->cipher_data.bits / 8;
    conn->key_material.client_write_key = g_malloc0(wk_len);
    memcpy(conn->key_material.client_write_key, key_material, wk_len);
    packet_tls_debug_print_hex("client_write_key", key_material, wk_len);
    key_material += wk_len;

    conn->key_material.server_write_key = g_malloc0(wk_len);
    memcpy(conn->key_material.server_write_key, key_material, wk_len);
    packet_tls_debug_print_hex("server_write_key", key_material, wk_len);
    key_material += wk_len;

    // Get IV blocks
    conn->key_material.client_write_IV = g_malloc0(conn->cipher_data.ivblock);
    memcpy(conn->key_material.client_write_IV, key_material, conn->cipher_data.ivblock);
    packet_tls_debug_print_hex("client_write_IV", key_material, conn->cipher_data.ivblock);
    key_material += conn->cipher_data.ivblock;
    conn->key_material.server_write_IV = g_malloc0(conn->cipher_data.ivblock);
    memcpy(conn->key_material.server_write_IV, key_material, conn->cipher_data.ivblock);
    packet_tls_debug_print_hex("server_write_IV", key_material, conn->cipher_data.ivblock);
    /* key_material+=conn->cipher_data.ivblock; */

    // Free temporally allocated memory
    g_free(seed);

    int mode = 0;
    if (conn->cipher_data.mode == MODE_CBC) {
        mode = GCRY_CIPHER_MODE_CBC;
    } else if (conn->cipher_data.mode == MODE_GCM) {
        mode = GCRY_CIPHER_MODE_CTR;
    } else {
        return FALSE;
    }

    // Create Client decoder
    gcry_cipher_open(&conn->client_cipher_ctx, conn->ciph, mode, 0);
    gcry_cipher_setkey(conn->client_cipher_ctx,
                       conn->key_material.client_write_key,
                       gcry_cipher_get_algo_keylen(conn->ciph));
    gcry_cipher_setiv(conn->client_cipher_ctx,
                      conn->key_material.client_write_IV,
                      gcry_cipher_get_algo_blklen(conn->ciph));

    // Create Server decoder
    gcry_cipher_open(&conn->server_cipher_ctx, conn->ciph, mode, 0);
    gcry_cipher_setkey(conn->server_cipher_ctx,
                       conn->key_material.server_write_key,
                       gcry_cipher_get_algo_keylen(conn->ciph));
    gcry_cipher_setiv(conn->server_cipher_ctx,
                      conn->key_material.server_write_IV,
                      gcry_cipher_get_algo_blklen(conn->ciph));

    return TRUE;
}

static gboolean
packet_tls_process_record_handshake(SSLConnection *conn, GByteArray *data)
{
    // Get Handshake data
    struct Handshake handshake;
    memcpy(&handshake, data->data, sizeof(struct Handshake));
    g_byte_array_remove_range(data, 0, sizeof(struct Handshake));

    if (UINT24_INT(handshake.length) < 0) {
        return FALSE;
    }

    switch (handshake.type) {
        case GNUTLS_HANDSHAKE_HELLO_REQUEST:
            break;
        case GNUTLS_HANDSHAKE_CLIENT_HELLO:
            return packet_tls_process_record_client_hello(conn, data);
        case GNUTLS_HANDSHAKE_SERVER_HELLO:
            return packet_tls_process_record_server_hello(conn, data);
        case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:
        case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
        case GNUTLS_HANDSHAKE_SERVER_HELLO_DONE:
        case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
            break;
        case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
            return packet_tls_process_record_key_exchange(conn, data);
        case GNUTLS_HANDSHAKE_FINISHED:
            break;
        default:
            break;
    }

    return TRUE;
}

static GByteArray *
packet_tls_process_record(SSLConnection *conn, GByteArray *data)
{
    // No record data here!
    if (data->len == 0)
        return data;

    // Get Record data
    struct TLSPlaintext record;
    memcpy(&record, data->data, sizeof(struct TLSPlaintext));
    g_byte_array_remove_range(data, 0, sizeof(struct TLSPlaintext));

    // Process record fragment
    if (UINT16_INT(record.length) > 0) {
        // TLSPlaintext fragment pointer
        GByteArray *fragment = g_byte_array_new();
        g_byte_array_append(fragment, data->data, UINT16_INT(record.length));
        g_byte_array_remove_range(data, 0, UINT16_INT(record.length));

        switch (record.type) {
            case HANDSHAKE:
                // Decode before parsing
                if (conn->encrypted) {
                    fragment = packet_tls_process_record_decode(conn, fragment);
                }
                // Hanshake Record, Try to get MasterSecret data
                if (!packet_tls_process_record_handshake(conn, fragment))
                    return NULL;
                break;
            case CHANGE_CIPHER_SPEC:
                // From now on, this connection will be encrypted using MasterSecret
                if (conn->client_cipher_ctx && conn->server_cipher_ctx)
                    conn->encrypted = 1;
                break;
            case APPLICATION_DATA:
                if (conn->encrypted) {
                    // Decrypt application data using MasterSecret
                    return packet_tls_process_record_decode(conn, fragment);
                }
                break;
            default:
                return data;
        }
    }

    return data;
}

static GByteArray *
packet_tls_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    SSLConnection *conn = NULL;
    GByteArray *out = NULL;

    // Get capture input from this parser
    CaptureInput *input = parser->input;
    g_assert(input != NULL);

    // Get manager information
    CaptureManager *manager = input->manager;
    if (capture_keyfile(manager) == NULL) {
        return data;
    }

    Address tlsserver = capture_tls_server(capture_manager());

    DissectorTlsData *priv = g_ptr_array_index(parser->dissectors, PACKET_TLS);
    g_return_val_if_fail(priv != NULL, NULL);

    // Get TCP/IP data from this packet
    PacketTcpData *tcpdata = g_ptr_array_index(packet->proto, PACKET_TCP);
    g_return_val_if_fail(tcpdata != NULL, NULL);

    // Get packet addresses
    Address src = packet_src_address(packet);
    Address dst = packet_dst_address(packet);

    // Try to find a session for this ip
    if ((conn = packet_tls_connection_find(parser, src, dst))) {
        // Update last connection direction
        conn->direction = packet_tls_connection_dir(conn, src);

        // Check current connection state
        switch (conn->state) {
            case TCP_STATE_SYN:
                // First SYN received, this package must be SYN/ACK
                if (tcpdata->syn == 1 && tcpdata->ack == 1)
                    conn->state = TCP_STATE_SYN_ACK;
                break;
            case TCP_STATE_SYN_ACK:
                // We expect an ACK packet here
                if (tcpdata->syn == 0 && tcpdata->ack == 1)
                    conn->state = TCP_STATE_ESTABLISHED;
                break;
            case TCP_STATE_ACK:
            case TCP_STATE_ESTABLISHED:
                // Check if we have a SSLv2 Handshake
                if (packet_tls_record_handshake_is_ssl2(conn, data)) {
                    out = packet_tls_process_record_ssl2(conn, data);
                    if (out == NULL) {
                        priv->connections = g_slist_remove(priv->connections, conn);
                        packet_tls_connection_destroy(conn);
                    }
                } else {
                    // Process data segment!
                    while (data->len > 0) {
                        out = packet_tls_process_record(conn, data);
                        if (out == NULL) {
                            priv->connections = g_slist_remove(priv->connections, conn);
                            packet_tls_connection_destroy(conn);
                        }
                    }
                }

                // This seems a SIP TLS packet ;-)
                if (out != NULL && out->len > 0) {
                    data = g_byte_array_new_take(out->data, out->len);
                    return packet_parser_next_dissector(parser, packet, data);
                }
                break;
            case TCP_STATE_FIN:
            case TCP_STATE_CLOSED:
                // We can delete this connection
                priv->connections = g_slist_remove(priv->connections, conn);
                packet_tls_connection_destroy(conn);
                break;
        }
    } else {
        if (tcpdata->syn != 0 && tcpdata->ack == 0) {
            // Only create new connections whose destination is tlsserver
            if (tlsserver.port) {
                if (addressport_equals(tlsserver, dst)) {
                    // New connection, store it status and leave
                    priv->connections =
                        g_slist_append(priv->connections, packet_tls_connection_create(src, dst));
                }
            } else {
                // New connection, store it status and leave
                priv->connections =
                    g_slist_append(priv->connections, packet_tls_connection_create(src, dst));
            }
        } else {
            return data;
        }
    }

    return NULL;
}

static void
packet_tls_init(PacketParser *parser)
{
    DissectorTlsData *tls_data = g_malloc0(sizeof(DissectorTlsData));
    g_ptr_array_set(parser->dissectors, PACKET_TLS, tls_data);
}

PacketDissector *
packet_tls_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_TLS;
    proto->init = packet_tls_init;
    proto->dissect = packet_tls_parse;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_WS));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    return proto;
}
