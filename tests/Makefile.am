AUTOMAKE_OPTIONS=subdir-objects

check_PROGRAMS=test-001 test-002 test-003 test-004 test-005
check_PROGRAMS+=test-006 test-007 test-008 test-009 test-010
check_PROGRAMS+=test-011 test-012 test-013 test-014 test-015
check_PROGRAMS+=test-016

test_001_SOURCES=test_001.c
test_002_SOURCES=test_002.c
test_003_SOURCES=test_003.c
test_004_SOURCES=test_004.c
test_005_SOURCES=test_005.c
test_006_SOURCES=test_006.c
test_007_SOURCES=test_007.c ../src/vector.c ../src/util.c
test_008_SOURCES=test_008.c
test_009_SOURCES=test_009.c
test_010_SOURCES=test_010.c ../src/hash.c
test_011_SOURCES=test_011.c
test_012_SOURCES=test_012.c ../src/packet.c ../src/vector.c ../src/util.c ../src/address.c ../src/rtp.c
test_013_SOURCES=test_013.c ../src/capture_esp.c
test_014_SOURCES=test_014.c
test_015_SOURCES=test_015.c ../src/capture_bpf_util.c ../src/vector.c ../src/util.c
test_016_SOURCES=test_016.c ../src/capture_bpf_util.c ../src/vector.c ../src/util.c

TESTS = $(check_PROGRAMS) test_ebpf.sh

# Skips itself unless run as root on a BTF capable kernel with eBPF built in
EXTRA_DIST = test_ebpf.sh
