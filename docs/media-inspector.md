# Standalone RTP inspection (experimental)

This feature adds a **SIP-independent**, passive RTP packet monitor for media-only
hosts, such as RTPengine nodes. It does not require SIP or SDP.

## Usage

```sh
# Live interface with the Media Inspector dashboard.
sudo sngrep --media-only -d eth0 udp

# Headless summary output (e.g. automated diagnostics).
sudo sngrep --media-only -N -d eth0 udp

# Offline packet capture. Prints the observed flow count when complete.
sngrep --media-only -I media.pcap

# Save matching captured RTP frames while inspecting.
sudo sngrep --media-only -d eth0 -O media-only.pcap udp
```

This stack has two independently reviewable PRs: the foundation adds
capture/aggregation, and the UI follow-up adds the new dashboard.
Press **F6** to switch between the classic call list/flow and Media Inspector.
Press **R** to open the existing raw SIP message view (F6 previously did this).
In a regular SIP session, F6 activates passive RTP sampling from that moment
onward without disabling SIP capture. It does not reconstruct RTP packets
captured before F6 was pressed. No RTPengine session
API adapter, Call-ID association, bidirectional leg correlation, codec-specific
MOS estimate, or definitive one-way-audio diagnosis exists in this PR.

## Metrics and confidence

Each RTP stream is identified by source and destination IP/port and SSRC.
Packets are validated for RTP v2 and CSRC/extension/padding bounds.
The tracker keeps only header-derived aggregate fields. Memory is capped
at 2048 flow entries; when full, the least recently seen entry is replaced.

* Sequence-gap loss is an **estimate at the capture point**, not a measured
  end-to-end loss rate. Recent duplicates and reordered packets are tracked.
* Jitter follows RFC 3550; milliseconds are shown only when a static RTP
  clock rate is known. Dynamic payload types require an external SDP mapping.
* A single passive observation point cannot prove that a packet was delivered
  to the remote endpoint, or whether the caller heard audio.
* Packet captures can miss packets under load and cannot reliably see every
  kernel-offloaded RTPengine path. Treat missing observed RTP as **insufficient
  evidence**, not conclusive one-way audio.
* An RTP stream is not a call. Correlating two legs through RTPengine requires
  a separate read-only control-plane integration.

## Scope and security

The feature is opt-in. Normal SIP capture and its UI remain unchanged.
It neither opens control sockets to RTPengine nor changes live media sessions.
RTP payload data is not stored in the tracker. The optional `-O` output
file contains packets and may include sensitive audio; handle it accordingly.

## Acceptance tests to run before merge

1. Build with Autotools and CMake on Linux.
2. `sngrep --media-only -I` an RTP-only PCAP: non-zero flow count.
3. The same PCAP in regular SIP mode: unchanged dialog behavior.
4. Verify `F6` switches dashboards, `R` opens raw SIP view, and `ESC` returns.
5. Resize from 80 to 120 columns, navigate with arrows, toggle sorting using `S`.
4. Inject malformed UDP, RTCP, RTP with malformed extensions and padding:
   parser rejects it without crashing.
5. Check SSRC separation, wraparound at 65535, 1/2/3 missing sequence
   values, duplicate and out-of-order arrivals.
6. Inspect behavior at 2048 concurrent streams and repeated reuse of slots.
7. On a live RTPengine host compare packet counters with `rtpengine-ctl`,
   including a kernel-forwarding configuration; do not assume PCAP visibility.
