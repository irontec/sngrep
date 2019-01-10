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
 * @file capture_pcap.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in pcap.h
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */

#include "config.h"
#include <glib.h>
#include <netdb.h>
#include <string.h>
#include <pcap/sll.h>
#include "glib-extra.h"
#include "capture.h"
#include "capture_hep.h"
#include "capture_pcap.h"
#include "capture/packet/packet_link.h"
#include "storage.h"
#include "stream.h"
#include "setting.h"
#include "timeval.h"

GQuark
capture_pcap_error_quark()
{
    return g_quark_from_static_string("capture-pcap");
}

CaptureInput *
capture_input_pcap_online(const gchar *dev, GError **error)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Create a new structure to handle this capture source
    CapturePcap *pcap = g_malloc(sizeof(CapturePcap));

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
    pcap->handle = pcap_open_live(dev, MAXIMUM_SNAPLEN, 1, 1000, errbuf);
    if (pcap->handle == NULL) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_DEVICE_OPEN,
                    "Couldn't open device %s: %s",
                    dev, errbuf);
        return NULL;
    }

    // Get datalink to dissect packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if (proto_link_size(pcap->link) == 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                    "Unknown link type %d",
                    pcap->link);
        return NULL;
    }

    // Create a new structure to handle this capture source
    CaptureInput *input = g_malloc0(sizeof(CaptureInput));
    input->source = dev;
    input->priv = pcap;
    input->tech = CAPTURE_TECH_PCAP;
    input->mode = CAPTURE_MODE_ONLINE;
    input->start = capture_input_pcap_start;
    input->stop = capture_input_pcap_stop;
    input->filter = capture_input_pcap_filter;

    // Ceate packet parser tree
    PacketParser *parser = packet_parser_new(input);
    packet_parser_add_proto(parser, parser->dissector_tree, PACKET_LINK);
    input->parser = parser;

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
    CapturePcap *pcap = g_malloc(sizeof(CapturePcap));

    // Open PCAP file
    if ((pcap->handle = pcap_open_offline(infile, errbuf)) == NULL) {
        gchar *filename = g_path_get_basename(infile);
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_FILE_OPEN,
                    "Couldn't open pcap file %s: %s",
                    filename, errbuf);
        g_free(filename);
        return NULL;
    }

    // Get datalink to dissect packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if (proto_link_size(pcap->link) == 0) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                    "Unknown link type %d",
                    pcap->link);
        return NULL;
    }

    // Create a new structure to handle this capture source
    CaptureInput *input = g_malloc0(sizeof(CaptureInput));
    input->source = infile;
    input->priv = pcap;
    input->tech = CAPTURE_TECH_PCAP;
    input->mode = CAPTURE_MODE_OFFLINE;
    input->start = capture_input_pcap_start;
    input->stop = capture_input_pcap_stop;
    input->filter = capture_input_pcap_filter;

    // Ceate packet parser tree
    PacketParser *parser = packet_parser_new(input);
    packet_parser_add_proto(parser, parser->dissector_tree, PACKET_LINK);
    input->parser = parser;

    return input;
}

gpointer
capture_input_pcap_start(CaptureInput *input)
{
    // Get private data
    CapturePcap *pcap = (CapturePcap *) input->priv;

    // Parse available packets
    pcap_loop(pcap->handle, -1, capture_pcap_parse_packet, (u_char *) input);

    // Close input file in offline mode
    if (input->mode == CAPTURE_MODE_OFFLINE) {
        // Mark as finished reading packets
        input->running = FALSE;
    }

    return NULL;
}

void
capture_input_pcap_stop(CaptureInput *input)
{
    g_return_if_fail(input != NULL);
    CapturePcap *pcap = (CapturePcap *) input->priv;
    g_return_if_fail(pcap != NULL);

    if (pcap->handle == NULL)
        return;

    if (input->mode == CAPTURE_MODE_OFFLINE) {
        pcap_close(pcap->handle);
    } else {
        pcap_breakloop(pcap->handle);
    }

    // Free input information
    g_free(input->priv);
    packet_parser_free(input->parser);

    // Mark as finished reading packets
    input->running = FALSE;
}

gboolean
capture_input_pcap_filter(CaptureInput *input, const gchar *filter, GError **error)
{
    // The compiled filter expression
    struct bpf_program bpf;

    // Capture PCAP private data
    CapturePcap *pcap = (CapturePcap *) input->priv;

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
capture_output_pcap_write(CaptureOutput *output, Packet *packet)
{
    CapturePcap *pcap = output->priv;

    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);

    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        // PCAP Frame Header data
        struct pcap_pkthdr header;
        header.caplen = frame->caplen;
        header.len = frame->len;
        header.ts.tv_sec = frame->ts.tv_sec;
        header.ts.tv_usec = frame->ts.tv_usec;
        // Save this packet
        pcap_dump((u_char *) pcap->dumper, &header, frame->data->data);
    }
}

static void
capture_output_pcap_close(CaptureOutput *output)
{
    CapturePcap *pcap = output->priv;

    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);

    pcap_dump_close(pcap->dumper);
}

CaptureOutput *
capture_output_pcap(const gchar *filename, GError **error)
{

    // PCAP Output is only availble if capture has a single input
    // and thas input is from PCAP thech
    CaptureManager *manager = capture_manager();
    g_return_val_if_fail(manager != NULL, NULL);

    if (g_slist_length(manager->inputs) != 1) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_SAVE_MULTIPLE,
                    "Save is only supported with a single capture input.");
        return NULL;
    }

    CaptureInput *input = g_slist_nth_data(manager->inputs, 0);
    g_return_val_if_fail(input != NULL, NULL);

    if (input->tech != CAPTURE_TECH_PCAP) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_SAVE_NOT_PCAP,
                    "Save is only supported from PCAP capture inputs.");
        return NULL;
    }

    CapturePcap *input_pcap = input->priv;

    // Create a new structure to handle this capture source
    CapturePcap *pcap = g_malloc0(sizeof(CapturePcap));

    pcap->dumper = pcap_dump_open(input_pcap->handle, filename);
    if (pcap->dumper == NULL) {
        g_set_error(error,
                    CAPTURE_PCAP_ERROR,
                    CAPTURE_PCAP_ERROR_DUMP_OPEN,
                    "Error while opening dump file: %s",
                    pcap_geterr(input_pcap->handle));
        return NULL;
    }

    // Create a new structure to handle this capture dumper
    CaptureOutput *output = g_malloc0(sizeof(CaptureOutput));
    output->priv = pcap;
    output->write = capture_output_pcap_write;
    output->close = capture_output_pcap_close;
    return output;
}

void
capture_pcap_parse_packet(u_char *info, const struct pcap_pkthdr *header, const guchar *content)
{

    // Capture Input info
    CaptureInput *input = (CaptureInput *) info;
    // Capture manager
    CaptureManager *manager = input->manager;
    // Packet dissectors parser
    PacketParser *parser = input->parser;

    // Ignore packets while capture is paused
    if (manager->paused)
        return;

    // Convert packet data
    GByteArray *data = g_byte_array_new();
    g_byte_array_append(data, content, header->caplen);

    // Create a new packet for this data
    Packet *packet = packet_new();
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    frame->ts.tv_sec = header->ts.tv_sec;
    frame->ts.tv_usec = header->ts.tv_usec;
    frame->caplen = header->caplen;
    frame->len = header->len;
    frame->data = g_byte_array_new();
    g_byte_array_append(frame->data, data->data, data->len);
    packet->frames = g_list_append(packet->frames, frame);

    // Initialize parser dissector to first one
    parser->current = parser->dissector_tree;

    // Request initial dissector parsing
    data = packet_parser_next_dissector(parser, packet, data);

    // Free not parsed packet data
    if (data != NULL) {
        g_byte_array_free(data, TRUE);
        g_free(frame);
        g_list_free(packet->frames);
        g_free(packet);
    }
}

gint
capture_packet_time_sorter(const Packet **a, const Packet **b)
{
    return timeval_is_older(packet_time(*a), packet_time(*b));
}

const gchar *
capture_input_pcap_file(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "Multiple files";

    CaptureInput *input = manager->inputs->data;
    if (input->tech == CAPTURE_TECH_PCAP && input->mode == CAPTURE_MODE_OFFLINE)
        return input->source;

    return NULL;
}

const gchar *
capture_input_pcap_device(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "multi";

    CaptureInput *input = manager->inputs->data;
    if (input->tech == CAPTURE_TECH_PCAP && input->mode == CAPTURE_MODE_ONLINE)
        return input->source;

    return NULL;
}
