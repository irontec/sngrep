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
 * @file capture_pcap.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture_pcap.h
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */

#include "config.h"
#include <sys/stat.h>
#include <glib.h>
#include <netdb.h>
#include <string.h>
#include <glib-unix.h>
#include "glib-extra/glib.h"
#include "capture.h"
#include "capture_pcap.h"
#include "packet/packet_link.h"
#include "storage/storage.h"

// CapturePcap class definition
G_DEFINE_TYPE(CaptureInputPcap, capture_input_pcap, CAPTURE_TYPE_INPUT)

GQuark
capture_pcap_error_quark()
{
    return g_quark_from_static_string("capture-pcap");
}

static void
capture_input_pcap_parse_packet(CaptureInputPcap *pcap, const struct pcap_pkthdr *header, const guchar *content)
{
    // Ignore packets while capture is paused
    if (capture_is_paused())
        return;

    // Ignore packets if storage limit has been reached
    if (storage_limit_reached())
        return;

    // Create a new packet for this data
    PacketFrame *frame = packet_frame_new();
    frame->ts = header->ts.tv_sec * G_USEC_PER_SEC + header->ts.tv_usec;
    frame->caplen = header->caplen;
    frame->len = header->len;
    frame->data = g_bytes_new(content, header->caplen);

    // Create a new packet
    Packet *packet = packet_new(CAPTURE_INPUT(pcap));
    packet->frames = g_list_append(packet->frames, frame);

    // Increase Capture input parsed bytes
    CaptureInput *input = CAPTURE_INPUT(pcap);
    capture_input_set_loaded_size(
        input,
        capture_input_loaded_size(input) + header->caplen
    );

    // Pass packet data to the first dissector
    PacketDissector *dissector = capture_input_initial_dissector(packet->input);
    GBytes *rest = packet_dissector_dissect(dissector, packet, g_bytes_ref(frame->data));

    // Free packet if not added to storage
    if (rest != NULL) g_bytes_unref(rest);
    packet_unref(packet);
}

static gboolean
capture_input_pcap_read_packet(G_GNUC_UNUSED gint fd,
                               G_GNUC_UNUSED GIOCondition condition, CaptureInputPcap *pcap)
{
    // Get next packet from this input
    struct pcap_pkthdr *header;
    const guchar *data;
    gint ret = pcap_next_ex(pcap->handle, &header, &data);

    // pcap_next_ex() returns 1 if the packet was read without problems
    if (ret == 0)
        return TRUE;

    //  PCAP_ERROR if an error occurred while reading the packet
    if (ret == PCAP_ERROR || ret == PCAP_ERROR_BREAK)
        return FALSE;

    if (data != NULL) {
        // Parse received data
        capture_input_pcap_parse_packet(pcap, header, data);
    }

    return TRUE;
}

static void
capture_input_pcap_stop(CaptureInput *input)
{
    // Get private data
    CaptureInputPcap *pcap = CAPTURE_INPUT_PCAP(input);

    if (pcap->handle == NULL)
        return;

    pcap_breakloop(pcap->handle);

    if (capture_input_mode(input) == CAPTURE_MODE_OFFLINE) {
        pcap_close(pcap->handle);
    }

    pcap->handle = NULL;

    // Mark the input as full loaded
    capture_input_set_loaded_size(input, capture_input_total_size(input));

    // Detach capture source from capture main loop
    if (!g_source_is_destroyed(capture_input_source(input))) {
        g_source_destroy(capture_input_source(input));
    }
}

CaptureInput *
capture_input_pcap_online(const gchar *dev, GError **error)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Create a new structure to handle this capture source
    CaptureInputPcap *pcap = g_object_new(CAPTURE_TYPE_INPUT_PCAP, NULL);

    // Try to find capture device information
    if (pcap_lookupnet(dev, &pcap->net, &pcap->mask, errbuf) == -1) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_DEVICE_LOOKUP,
                    "Can't get netmask for device %s",
                    dev);
        return NULL;
    }

    // Open capture device
    pcap->handle = pcap_create(dev, errbuf);
    if (pcap->handle == NULL) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_DEVICE_CREATE,
                    "Couldn't open device %s: %s",
                    dev, errbuf);
        return NULL;
    }

    gint status = pcap_set_promisc(pcap->handle, 1);
    if (status != 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_PROMISC,
                    "Error setting promiscuous mode on %s: %s\n",
                    dev, pcap_statustostr(status));
        return NULL;
    }

    status = pcap_set_timeout(pcap->handle, 1000);
    if (status != 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_TIMEOUT,
                    "Error setting capture timeout on %s: %s\n",
                    dev, pcap_statustostr(status));
        return NULL;
    }

    status = pcap_set_snaplen(pcap->handle, MAXIMUM_SNAPLEN);
    if (status != 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_SNAPLEN,
                    "Error setting snaplen on %s: %s\n",
                    dev, pcap_statustostr(status));
        return NULL;
    }

    status = pcap_set_buffer_size(pcap->handle, 10 * G_BYTES_PER_MEGABYTE);
    if (status != 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_BUFFER_SIZE,
                    "Error setting buffer size on %s: %s\n",
                    dev, pcap_statustostr(status));
        return NULL;
    }

    // Get datalink to dissect packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check link types sngrep known before start parsing packets
    if (packet_link_size(pcap->link) == 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                    "Unknown link type %d",
                    pcap->link);
        return NULL;
    }

    // Create a new structure to handle this capture source
    CaptureInput *input = CAPTURE_INPUT(pcap);
    capture_input_set_mode(input, CAPTURE_MODE_ONLINE);
    capture_input_set_source_str(input, dev);
    capture_input_set_initial_dissector(input, packet_dissector_find_by_id(PACKET_PROTO_LINK));

    // Create GSource for main loop
    capture_input_set_source(
        input,
        g_unix_fd_source_new(
            pcap_fileno(pcap->handle),
            G_IO_IN | G_IO_ERR | G_IO_HUP
        )
    );

    g_source_set_callback(
        capture_input_source(input),
        (GSourceFunc) G_CALLBACK(capture_input_pcap_read_packet),
        pcap,
        (GDestroyNotify) capture_input_pcap_stop
    );

    return input;
}

CaptureInput *
capture_input_pcap_offline(const gchar *infile, GError **error)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Check if file is standard input
    if (strlen(infile) == 1 && *infile == '-') {
        infile = "/dev/stdin";
        G_GNUC_UNUSED FILE *tty = freopen("/dev/tty", "r", stdin);
    }

    // Create a new structure to handle this capture source
    CaptureInputPcap *pcap = g_object_new(CAPTURE_TYPE_INPUT_PCAP, NULL);

    // Open PCAP file
    if ((pcap->handle = pcap_open_offline(infile, errbuf)) == NULL) {
        g_autofree gchar *filename = g_path_get_basename(infile);
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_FILE_OPEN,
                    "Couldn't open pcap file %s: %s",
                    filename, errbuf);
        return NULL;
    }

    // Get datalink to dissect packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check link types sngrep known before start parsing packets
    if (packet_link_size(pcap->link) == 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                    "Unknown link type %d",
                    pcap->link);
        return NULL;
    }

    // Create a new structure to handle this capture source
    g_autofree gchar *basename = g_path_get_basename(infile);
    CaptureInput *input = CAPTURE_INPUT(pcap);
    capture_input_set_mode(input, CAPTURE_MODE_OFFLINE);
    capture_input_set_source_str(input, basename);
    capture_input_set_initial_dissector(input, packet_dissector_find_by_id(PACKET_PROTO_LINK));

    // Get File
    if (g_file_test(infile, G_FILE_TEST_IS_REGULAR)) {
        struct stat st = { 0 };
        stat(infile, &st);
        capture_input_set_total_size(input, st.st_size);
    }

    // Create GSource for main loop
    capture_input_set_source(
        input,
        g_unix_fd_source_new(
            pcap_get_selectable_fd(pcap->handle),
            G_IO_IN | G_IO_ERR | G_IO_HUP
        )
    );

    g_source_set_callback(
        capture_input_source(CAPTURE_INPUT(pcap)),
        (GSourceFunc) G_CALLBACK(capture_input_pcap_read_packet),
        pcap,
        (GDestroyNotify) capture_input_pcap_stop
    );

    return input;
}

gint
capture_input_pcap_datalink(CaptureInput *input)
{
    // Capture PCAP private data
    CaptureInputPcap *pcap = CAPTURE_INPUT_PCAP(input);
    g_return_val_if_fail(pcap != NULL, -1);

    return pcap->link;
}

const gchar *
capture_input_pcap_file(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "Multiple files";

    CaptureInput *input = manager->inputs->data;
    if (capture_input_tech(input) == CAPTURE_TECH_PCAP
        && capture_input_mode(input) == CAPTURE_MODE_OFFLINE) {
        return capture_input_source_str(input);
    }

    return NULL;
}

const gchar *
capture_input_pcap_device(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "multi";

    CaptureInput *input = manager->inputs->data;
    if (capture_input_tech(input) == CAPTURE_TECH_PCAP
        && capture_input_mode(input) == CAPTURE_MODE_ONLINE) {
        return capture_input_source_str(input);
    }
    return NULL;
}

static gboolean
capture_input_pcap_filter(CaptureInput *input, const gchar *filter, GError **error)
{
    // The compiled filter expression
    struct bpf_program bpf;

    // Capture PCAP private data
    CaptureInputPcap *pcap = CAPTURE_INPUT_PCAP(input);

    //! Check if filter compiles
    if (pcap_compile(pcap->handle, &bpf, filter, 0, pcap->mask) == -1) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_FILTER_COMPILE,
                    "Couldn't compile filter '%s': %s",
                    filter, pcap_geterr(pcap->handle));
        return FALSE;
    }

    // Set capture filter
    if (pcap_setfilter(pcap->handle, &bpf) == -1) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_FILTER_APPLY,
                    "Couldn't set filter '%s': %s",
                    filter, pcap_geterr(pcap->handle));
        return FALSE;
    }

    // Deallocate BPF program data
    pcap_freecode(&bpf);

    return TRUE;
}

static void
capture_input_pcap_class_init(CaptureInputPcapClass *klass)
{
    CaptureInputClass *input_class = CAPTURE_INPUT_CLASS(klass);
    input_class->filter = capture_input_pcap_filter;
    input_class->stop = capture_input_pcap_stop;
}

static void
capture_input_pcap_init(CaptureInputPcap *self)
{
    capture_input_set_tech(CAPTURE_INPUT(self), CAPTURE_TECH_PCAP);
}

G_DEFINE_TYPE(CaptureOutputPcap, capture_output_pcap, CAPTURE_TYPE_OUTPUT)

static void
capture_output_pcap_write(CaptureOutput *self, Packet *packet)
{
    CaptureOutputPcap *pcap = CAPTURE_OUTPUT_PCAP(self);

    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);

    // Check if the input has the same datalink as the output
    gint datalink = capture_input_pcap_datalink(packet_get_input(packet));
    gint datalink_size = 0;
    if (datalink != pcap->link) {
        // Strip datalink header from all packets
        datalink_size = packet_link_size(datalink);
    }

    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        // PCAP Frame Header data
        struct pcap_pkthdr header;
        header.caplen = frame->caplen - datalink_size;
        header.len = frame->len - datalink_size;
        header.ts.tv_sec = packet_frame_seconds(frame);
        header.ts.tv_usec = packet_frame_microseconds(frame);
        // Save this packet
        g_autoptr(GBytes) data = g_bytes_new_from_bytes(
            frame->data,
            datalink_size,
            g_bytes_get_size(frame->data) - datalink_size
        );
        pcap_dump((u_char *) pcap->dumper, &header, g_bytes_get_data(data, NULL));
    }
}

static void
capture_output_pcap_close(CaptureOutput *self)
{
    CaptureOutputPcap *pcap = CAPTURE_OUTPUT_PCAP(self);
    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);
    pcap_dump_close(pcap->dumper);
}

CaptureOutput *
capture_output_pcap(const gchar *filename, GError **error)
{
    // PCAP Output is only available if capture has a single input
    // and that input is from PCAP tech
    CaptureManager *manager = capture_manager_get_instance();
    g_return_val_if_fail(manager != NULL, NULL);

    CaptureInput *input = g_slist_first_data(manager->inputs);
    g_return_val_if_fail(input != NULL, NULL);

    if (capture_input_tech(input) != CAPTURE_TECH_PCAP) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_SAVE_NOT_PCAP,
                    "Save is only supported from PCAP capture inputs.");
        return NULL;
    }

    // Create a new structure to handle this capture source
    CaptureOutputPcap *pcap = g_object_new(CAPTURE_TYPE_OUTPUT_PCAP, NULL);

    pcap->link = capture_input_pcap_datalink(input);
    if (g_slist_length(manager->inputs) > 1) {
        for (GSList *l = manager->inputs; l != NULL; l = l->next) {
            if (capture_input_pcap_datalink(l->data) != pcap->link) {
                pcap->link = DLT_RAW;
            }
        }
    }

    pcap->handle = pcap_open_dead(pcap->link, MAXIMUM_SNAPLEN);
    pcap->dumper = pcap_dump_open(pcap->handle, filename);
    if (pcap->dumper == NULL) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_DUMP_OPEN,
                    "Error while opening dump file: %s",
                    pcap_geterr(pcap->handle));
        return NULL;
    }

    // Create a new structure to handle this capture dumper
    return CAPTURE_OUTPUT(pcap);
}

static void
capture_output_pcap_finalize(GObject *object)
{
    CaptureOutputPcap *pcap = CAPTURE_OUTPUT_PCAP(object);
    pcap_close(pcap->handle);
    G_OBJECT_CLASS (capture_output_pcap_parent_class)->finalize(object);
}

static void
capture_output_pcap_class_init(CaptureOutputPcapClass *klass)
{
    CaptureOutputClass *capture_output_class = CAPTURE_OUTPUT_CLASS(klass);
    capture_output_class->write = capture_output_pcap_write;
    capture_output_class->close = capture_output_pcap_close;

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = capture_output_pcap_finalize;
}

static void
capture_output_pcap_init(CaptureOutputPcap *self)
{
    capture_output_set_tech(CAPTURE_OUTPUT(self), CAPTURE_TECH_PCAP);
}
