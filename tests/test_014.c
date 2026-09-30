/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 * @file test_014.c
 *
 * Unit tests for display filters (filter.c), focused on the call state
 * filter and its combination with the existing regular expression filters.
 *
 * Calls are created and moved through their states with the real sip_call.c
 * functions. Only packet, settings and UI dependencies are stubbed.
 */
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/filter.h"
#include "../src/sip.h"
#include "../src/setting.h"
#include "../src/curses/ui_call_list.h"

/**
 * @brief Test message: SIP message plus the data normally read from its packet
 */
struct test_msg {
    //! Must be the first member, test messages are used as sip_msg_t
    sip_msg_t msg;
    const char *method;
    const char *src;
    const char *dst;
    const char *payload;
};

//! All calls created by the test, iterated by filter_reset_calls
static vector_t *test_calls;

/****************************************************************************
 * Stubs for dependencies outside of the tested code
 ****************************************************************************/
const char *
msg_get_attribute(struct sip_msg *msg, int id, char *value)
{
    struct test_msg *tmsg = (struct test_msg *) msg;

    switch (id) {
        case SIP_ATTR_SIPFROM:
            strcpy(value, msg->sip_from);
            break;
        case SIP_ATTR_SIPTO:
            strcpy(value, msg->sip_to);
            break;
        case SIP_ATTR_SRC:
            strcpy(value, tmsg->src);
            break;
        case SIP_ATTR_DST:
            strcpy(value, tmsg->dst);
            break;
        case SIP_ATTR_METHOD:
            strcpy(value, tmsg->method);
            break;
    }
    return strlen(value) ? value : NULL;
}

const char *
msg_get_payload(sip_msg_t *msg)
{
    return ((struct test_msg *) msg)->payload;
}

struct timeval
msg_get_time(sip_msg_t *msg)
{
    struct timeval t = { 0 };
    return t;
}

void
msg_destroyer(void *msg)
{
    free(msg);
}

void
packet_destroyer(void *packet)
{
}

bool
addressport_equals(address_t addr1, address_t addr2)
{
    return false;
}

int
setting_enabled(int id)
{
    return 0;
}

const char *
sip_transport_str(int transport)
{
    return "UDP";
}

vector_iter_t
sip_calls_iterator()
{
    return vector_iterator(test_calls);
}

ui_t *
ui_find_by_type(enum panel_types type)
{
    return NULL;
}

const char *
call_list_search_text(ui_t *ui, sip_call_t *call, char *text, size_t textlen)
{
    return text;
}

/****************************************************************************
 * Helpers
 ****************************************************************************/

/**
 * @brief Add a message to a call, updating its state like sip_check_packet
 */
static void
add_msg(sip_call_t *call, int reqresp, const char *method, uint32_t cseq)
{
    struct test_msg *tmsg = calloc(1, sizeof(struct test_msg));
    int oldstate;
    assert(tmsg);

    tmsg->msg.reqresp = reqresp;
    tmsg->msg.cseq = cseq;
    tmsg->msg.sip_from = "alice@example.com";
    tmsg->msg.sip_to = "bob@example.com";
    tmsg->method = method;
    tmsg->src = "10.0.0.1:5060";
    tmsg->dst = "10.0.0.2:5060";
    tmsg->payload = (reqresp == SIP_METHOD_INVITE) ? "INVITE ... X-Tag: magic" : "SIP/2.0 ...";

    call_add_message(call, &tmsg->msg);
    if (call_is_invite(call)) {
        oldstate = call->state;
        call_update_state(call, &tmsg->msg);
        if (call->state != oldstate)
            filter_call_state_changed(call);
    }
}

/**
 * @brief Create a call started with the given request
 */
static sip_call_t *
new_call(const char *callid, int reqresp, const char *method)
{
    sip_call_t *call = call_create((char *) callid, "");
    assert(call);
    vector_append(test_calls, call);
    add_msg(call, reqresp, method, 1);
    return call;
}

static sip_call_t *
new_invite(const char *callid)
{
    return new_call(callid, SIP_METHOD_INVITE, "INVITE");
}

static void
answer(sip_call_t *call)
{
    add_msg(call, 200, "INVITE", 1);
    add_msg(call, SIP_METHOD_ACK, "ACK", 1);
}

static void
hangup(sip_call_t *call)
{
    add_msg(call, SIP_METHOD_BYE, "BYE", 2);
}

//! Return true if the call is displayed with current filters
static int
shown(sip_call_t *call)
{
    return filter_check_call(call);
}

//! Set call state filter as the filter dialog does
static void
apply_states(unsigned int states)
{
    filter_set_callstates(states);
    filter_reset_calls();
}

#define BIT(state) FILTER_CALLSTATE_BIT(SIP_CALLSTATE_ ## state)

/****************************************************************************
 * Tests
 ****************************************************************************/
static void
test_parse_states()
{
    assert(filter_callstates_from_str(NULL) == 0);
    assert(filter_callstates_from_str("") == 0);
    assert(filter_callstates_from_str("IN CALL") == BIT(INCALL));
    assert(filter_callstates_from_str("IN CALL,COMPLETED") == (BIT(INCALL) | BIT(COMPLETED)));
    assert(filter_callstates_from_str(" in call , Completed ") == (BIT(INCALL) | BIT(COMPLETED)));
    assert(filter_callstates_from_str("CALL SETUP,CANCELLED,REJECTED,DIVERTED,BUSY")
           == (BIT(CALLSETUP) | BIT(CANCELLED) | BIT(REJECTED) | BIT(DIVERTED) | BIT(BUSY)));
    // Only exact state names are accepted, not substrings
    assert(filter_callstates_from_str("CALL") == 0);
    assert(filter_callstates_from_str("IN") == 0);
    assert(filter_callstates_from_str("INCALL") == 0);
    assert(filter_callstates_from_str("FOO,BUSY") == BIT(BUSY));
}

static void
test_no_states_selected(sip_call_t **calls, sip_call_t *options)
{
    int i;

    apply_states(0);
    assert(filter_get_callstates() == 0);
    for (i = 0; calls[i]; i++)
        assert(shown(calls[i]));
    assert(shown(options));
}

static void
test_single_state(sip_call_t *setup, sip_call_t *incall, sip_call_t *completed, sip_call_t *options)
{
    apply_states(BIT(INCALL));
    assert(!shown(setup));
    assert(shown(incall));
    assert(!shown(completed));
    assert(!shown(options));

    apply_states(BIT(COMPLETED));
    assert(!shown(setup));
    assert(!shown(incall));
    assert(shown(completed));
    assert(!shown(options));

    apply_states(BIT(CALLSETUP));
    assert(shown(setup));
    assert(!shown(incall));
    assert(!shown(completed));
}

static void
test_multiple_states(sip_call_t *setup, sip_call_t *incall, sip_call_t *completed, sip_call_t *options)
{
    apply_states(BIT(INCALL) | BIT(COMPLETED));
    assert(!shown(setup));
    assert(shown(incall));
    assert(shown(completed));
    assert(!shown(options));
}

static void
test_combined_filters(sip_call_t *incall, sip_call_t *completed)
{
    int i;
    // Each filter matches both calls: result only depends on the state filter
    int types[] = { FILTER_SIPFROM, FILTER_SIPTO, FILTER_SOURCE, FILTER_DESTINATION,
                    FILTER_CALLID, FILTER_PAYLOAD, FILTER_METHOD };
    const char *match[] = { "alice", "bob", "10.0.0.1", "10.0.0.2", "call-", "magic", "(INVITE|OPTIONS)" };
    const char *nomatch[] = { "carol", "dave", "10.0.0.9", "10.0.0.9", "nomatch", "nomatch", "(REGISTER)" };

    for (i = 0; i < (int) (sizeof(types) / sizeof(types[0])); i++) {
        // Matching regex filter AND state filter
        assert(filter_set(types[i], match[i]) == 0);
        apply_states(BIT(INCALL));
        assert(shown(incall));
        assert(!shown(completed));

        // Not matching regex filter hides the call despite its state
        assert(filter_set(types[i], nomatch[i]) == 0);
        apply_states(BIT(INCALL));
        assert(!shown(incall));
        assert(!shown(completed));

        // Regex filter alone
        apply_states(0);
        assert(!shown(incall));

        assert(filter_set(types[i], NULL) == 0);
        filter_reset_calls();
        assert(shown(incall));
        assert(shown(completed));
    }
}

static void
test_state_transitions()
{
    sip_call_t *call = new_invite("call-transition");

    // Filter set before the call changes its state
    apply_states(BIT(CALLSETUP));
    assert(call->state == SIP_CALLSTATE_CALLSETUP);
    assert(shown(call));

    // Answered call leaves CALL SETUP: cached result must not be reused
    answer(call);
    assert(call->state == SIP_CALLSTATE_INCALL);
    assert(!shown(call));

    // And enters a displayed state again
    apply_states(BIT(COMPLETED));
    assert(!shown(call));
    hangup(call);
    assert(call->state == SIP_CALLSTATE_COMPLETED);
    assert(shown(call));

    // Messages that don't change the state keep the cached result
    add_msg(call, 200, "BYE", 2);
    assert(call->filtered == 0);

    // Without call state filter, state changes don't discard cached results
    apply_states(0);
    call = new_invite("call-transition-nofilter");
    assert(shown(call));
    call->filtered = 1;
    answer(call);
    assert(call->filtered == 1);
}

static void
test_apply_and_clear(sip_call_t *setup, sip_call_t *incall)
{
    apply_states(BIT(INCALL));
    assert(!shown(setup));
    assert(shown(incall));

    // Clearing the filter reevaluates previously hidden calls
    apply_states(0);
    assert(shown(setup));
    assert(shown(incall));
}

int
main()
{
    sip_call_t *setup, *incall, *completed, *cancelled, *busy, *rejected, *diverted, *options;

    test_calls = vector_create(10, 10);

    // One call on each call state
    setup = new_invite("call-setup");
    incall = new_invite("call-incall");
    answer(incall);
    completed = new_invite("call-completed");
    answer(completed);
    hangup(completed);
    cancelled = new_invite("call-cancelled");
    add_msg(cancelled, SIP_METHOD_CANCEL, "CANCEL", 1);
    busy = new_invite("call-busy");
    add_msg(busy, 486, "INVITE", 1);
    rejected = new_invite("call-rejected");
    add_msg(rejected, 403, "INVITE", 1);
    diverted = new_invite("call-diverted");
    add_msg(diverted, 302, "INVITE", 1);
    // Dialog without call state
    options = new_call("call-options", SIP_METHOD_OPTIONS, "OPTIONS");

    assert(setup->state == SIP_CALLSTATE_CALLSETUP);
    assert(incall->state == SIP_CALLSTATE_INCALL);
    assert(completed->state == SIP_CALLSTATE_COMPLETED);
    assert(cancelled->state == SIP_CALLSTATE_CANCELLED);
    assert(busy->state == SIP_CALLSTATE_BUSY);
    assert(rejected->state == SIP_CALLSTATE_REJECTED);
    assert(diverted->state == SIP_CALLSTATE_DIVERTED);
    assert(options->state == 0);

    sip_call_t *all[] = { setup, incall, completed, cancelled, busy, rejected, diverted, NULL };

    test_parse_states();
    test_no_states_selected(all, options);
    test_single_state(setup, incall, completed, options);
    test_multiple_states(setup, incall, completed, options);
    test_combined_filters(incall, completed);
    test_state_transitions();
    test_apply_and_clear(setup, incall);

    // Each state can be selected on its own
    apply_states(BIT(CANCELLED));
    assert(shown(cancelled) && !shown(busy) && !shown(rejected) && !shown(diverted));
    apply_states(BIT(BUSY));
    assert(!shown(cancelled) && shown(busy) && !shown(rejected) && !shown(diverted));
    apply_states(BIT(REJECTED));
    assert(!shown(cancelled) && !shown(busy) && shown(rejected) && !shown(diverted));
    apply_states(BIT(DIVERTED));
    assert(!shown(cancelled) && !shown(busy) && !shown(rejected) && shown(diverted));

    vector_destroy(test_calls);
    return 0;
}
