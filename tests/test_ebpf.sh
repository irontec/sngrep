#!/bin/sh
#
# Integration test for eBPF TLS capture.
#
# Exits 77, automake's SKIP code, rather than failing when the environment can
# not support the feature, so make check stays green in containers, on
# non-Linux hosts and for unprivileged users.

SNGREP="${SNGREP:-../src/sngrep}"
PORT="${SNGREP_TEST_PORT:-15061}"

if [ ! -x "$SNGREP" ]; then
    echo "SKIP: sngrep binary not found at $SNGREP"; exit 77
fi
if ! "$SNGREP" --help 2>&1 | grep -q ebpf; then
    echo "SKIP: built without --with-bpf"; exit 77
fi
if [ "$(id -u)" != "0" ]; then
    echo "SKIP: needs root to load eBPF programs"; exit 77
fi
if [ ! -r /sys/kernel/btf/vmlinux ]; then
    echo "SKIP: kernel has no BTF"; exit 77
fi
if ! command -v openssl >/dev/null 2>&1; then
    echo "SKIP: openssl not available"; exit 77
fi

WORK=$(mktemp -d) || exit 77

cleanup() {
    [ -n "$SNG_PID" ] && kill "$SNG_PID" 2>/dev/null
    [ -n "$SRV_PID" ] && kill "$SRV_PID" 2>/dev/null
    exec 3>&- 2>/dev/null
    exec 4>&- 2>/dev/null
    rm -rf "$WORK"
}
trap cleanup EXIT

openssl req -x509 -newkey rsa:2048 -keyout "$WORK/k.pem" -out "$WORK/c.pem" \
    -days 1 -nodes -subj "/CN=sngrep-ebpf-test" >/dev/null 2>&1 || {
    echo "SKIP: could not generate a test certificate"; exit 77
}

# The server has to be running before sngrep starts, because libssl is
# discovered once at startup
mkfifo "$WORK/srvin" || exit 77
openssl s_server -quiet -accept "$PORT" \
    -cert "$WORK/c.pem" -key "$WORK/k.pem" <"$WORK/srvin" >/dev/null 2>&1 &
SRV_PID=$!
exec 3>"$WORK/srvin"
sleep 1

if ! kill -0 "$SRV_PID" 2>/dev/null; then
    echo "SKIP: could not start a TLS server on port $PORT"; exit 77
fi

"$SNGREP" --ebpf -N -q -T "$WORK/calls.txt" >"$WORK/err.txt" 2>&1 &
SNG_PID=$!
sleep 3

if ! kill -0 "$SNG_PID" 2>/dev/null; then
    echo "FAIL: sngrep exited during startup"
    cat "$WORK/err.txt"
    exit 1
fi

mkfifo "$WORK/cliin" || exit 77
timeout 15 openssl s_client -quiet -connect "127.0.0.1:$PORT" \
    <"$WORK/cliin" >/dev/null 2>&1 &
exec 4>"$WORK/cliin"
sleep 1

printf 'INVITE sip:bob@example.net SIP/2.0\r\nVia: SIP/2.0/TLS 127.0.0.1:%s;branch=z9hG4bKtest\r\nFrom: <sip:alice@example.net>;tag=1\r\nTo: <sip:bob@example.net>\r\nCall-ID: ebpf-integration-test\r\nCSeq: 1 INVITE\r\nContent-Length: 0\r\n\r\n' \
    "$PORT" >&4
sleep 2

printf 'SIP/2.0 200 OK\r\nVia: SIP/2.0/TLS 127.0.0.1:%s;branch=z9hG4bKtest\r\nFrom: <sip:alice@example.net>;tag=1\r\nTo: <sip:bob@example.net>;tag=2\r\nCall-ID: ebpf-integration-test\r\nCSeq: 1 INVITE\r\nContent-Length: 0\r\n\r\n' \
    "$PORT" >&3
sleep 2

exec 4>&-
kill "$SNG_PID" 2>/dev/null
wait "$SNG_PID" 2>/dev/null
SNG_PID=""

if ! grep -q "ebpf-integration-test" "$WORK/calls.txt" 2>/dev/null; then
    echo "FAIL: the captured dialog is missing from sngrep output"
    echo "--- sngrep stderr ---"; cat "$WORK/err.txt"
    echo "--- sngrep output ---"; cat "$WORK/calls.txt" 2>/dev/null
    exit 1
fi

if ! grep -q "INVITE sip:bob@example.net" "$WORK/calls.txt"; then
    echo "FAIL: the request was captured but its start line is wrong"
    exit 1
fi

if ! grep -q "SIP/2.0 200 OK" "$WORK/calls.txt"; then
    echo "FAIL: the response was not captured, only one direction worked"
    exit 1
fi

echo "test_ebpf: OK"
exit 0
