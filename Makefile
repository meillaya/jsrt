ZIG ?= zig
ZIG_BUILD ?= $(ZIG) build

ITERATIONS ?= 5
WARMUPS ?= 1
FIXTURE ?=
FILTER ?=

BENCH_OPTS := -Diterations=$(ITERATIONS) -Dwarmups=$(WARMUPS)
BENCH_FILTER_OPTS :=
ifneq ($(strip $(FIXTURE)),)
BENCH_OPTS += -Dfixture=$(FIXTURE)
BENCH_FILTER_OPTS += -Dfixture=$(FIXTURE)
endif
ifneq ($(strip $(FILTER)),)
BENCH_OPTS += -Dfilter=$(FILTER)
BENCH_FILTER_OPTS += -Dfilter=$(FILTER)
endif

.PHONY: all clean test debug conformance test262-adapter vm-parity verify-production bench bench-vm bench-compare bench-smoke asan

all:
	$(ZIG_BUILD) all

debug:
	$(ZIG_BUILD) debug

test:
	$(ZIG_BUILD) test

conformance:
	$(ZIG_BUILD) conformance

test262-adapter:
	$(ZIG_BUILD) test262-adapter

vm-parity:
	$(ZIG_BUILD) vm-parity

verify-production:
	$(ZIG_BUILD) verify-production

bench:
	$(ZIG_BUILD) bench $(BENCH_OPTS)

bench-vm:
	$(ZIG_BUILD) bench-vm $(BENCH_OPTS)

bench-compare:
	$(ZIG_BUILD) bench-compare $(BENCH_OPTS)

bench-smoke:
	$(ZIG_BUILD) bench-smoke $(BENCH_FILTER_OPTS)

asan:
	$(ZIG_BUILD) asan

clean:
	$(ZIG_BUILD) clean
