/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ****************************************************************************/
/**
 * @file test_015.c
 *
 * RTPengine NG "query" response parsing tests.
 */

#include "config.h"
#include <assert.h>
#include <string.h>
#include "../src/rtpengine.h"

static const char query[] =
    "d7:createdi1e6:result2:ok4:tagsd4:tagAd6:mediasld5:indexi1e7:streamsld8:endpointd7:address8:10.0.0.1"
    "6:family4:IPv44:porti4000ee5:flagsl3:RTP6:filled10:kernelizede10:local porti30000e5:statsd5:bytes"
    "i17200e6:errorsi1e7:packetsi100eeed8:endpointd7:address8:10.0.0.16:family4:IPv44:porti4001ee5:flags"
    "l4:RTCPe10:local porti30000e5:statsd5:bytesi400e6:errorsi0e7:packetsi5eeee4:type5:audioee3:tag4:tagAe"
    "4:tagBd6:mediasld5:indexi1e7:streamsld8:endpointd7:address8:10.0.0.26:family4:IPv44:porti5000ee5:flags"
    "l3:RTP10:kernelized17:no kernel supporte10:local porti30000e5:statsd5:bytesi15480e6:errorsi0e7:packetsi90eeee4:type5:audioee3:tag4:tagB"
    "ee6:totalsd3:RTPd5:bytesi0e6:errorsi0e7:packetsi190eeee";

static int
parse(const char *text, size_t len)
{
    rtpengine_call_t call;
    return rtpengine_parse_query(text, len, &call);
}

int
main()
{
    rtpengine_call_t call;
    char deep[256];
    size_t i;

    // Two legs, RTCP stream excluded from packet counters
    memset(&call, 0, sizeof(call));
    assert(rtpengine_parse_query(query, strlen(query), &call) == 0);
    assert(call.nlegs == 2);
    assert(!strcmp(call.legs[0].tag, "tagA"));
    assert(!strcmp(call.legs[0].address, "10.0.0.1"));
    assert(call.legs[0].port == 4000);
    assert(call.legs[0].packets == 100);
    assert(call.legs[0].bytes == 17200);
    assert(call.legs[0].errors == 1);
    assert(call.legs[0].kernelized);
    assert(!strcmp(call.legs[1].address, "10.0.0.2"));
    assert(call.legs[1].packets == 90);
    assert(!call.legs[1].kernelized);

    // Error replies and every truncation are rejected without reading past the end
    assert(parse("d6:result5:error12:error-reason4:nopee", 37) != 0);
    for (i = 0; i < strlen(query); i++)
        parse(query, i);
    assert(parse("d6:result2:ok4:tagsd4:tagA99999999999:xee", 41) != 0);

    // Nesting beyond the depth limit is refused instead of recursing
    memset(deep, 'l', sizeof(deep));
    memcpy(deep, "d6:result2:ok4:tagsd1:a", 23);
    assert(parse(deep, sizeof(deep)) != 0);
    return 0;
}
