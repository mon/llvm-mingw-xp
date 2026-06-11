# Self-contained tests for XP-specific patches. Kept out of the upstream
# test/Makefile to minimise rebase pain when llvm-mingw updates.
#
# Usage:
#   make -f test/xp/Makefile ARCH=i686 RUN=wine test
# or run via xp-tests.sh.

SRC_PATH := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
vpath %.c $(SRC_PATH)
vpath %.cpp $(SRC_PATH)

ifeq ($(ARCH),)
$(error ARCH must be set (e.g. i686 or x86_64))
endif

CROSS = $(ARCH)-w64-mingw32-
CC    = $(CROSS)gcc
CXX   = $(CROSS)g++
READOBJ = llvm-readobj

ifneq ($(RUN),)
    DO_RUN = $(RUN) # trailing whitespace is intentional
else
    DO_RUN = ./
endif

TESTS_CPP = filesystem locale-cpp hw-concurrency codecvt

EXES = $(addsuffix .exe, $(TESTS_CPP))

all: $(EXES)

$(EXES): %.exe: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 $< -o $@

TESTED = $(addprefix .tested., $(EXES))

test: all $(TESTED) subsystem-version.tested

.tested.%.exe: %.exe
	$(DO_RUN)$<
	@touch $@

# Verify lld writes OSVersion/SubsystemVersion = 5.1 into the PE header, the
# minimum required for Windows XP to accept an image. Uses one of the built
# test exes so we don't have to compile a dedicated one.
subsystem-version.tested: filesystem.exe
	@set -e; \
	out=$$($(READOBJ) --file-headers $<); \
	echo "$$out" | awk '/MajorOperatingSystemVersion/ { osmaj=$$2 } \
	                    /MinorOperatingSystemVersion/ { osmin=$$2 } \
	                    /MajorSubsystemVersion/       { ssmaj=$$2 } \
	                    /MinorSubsystemVersion/       { ssmin=$$2 } \
	                    END { \
	                      if (osmaj != 5 || osmin != 1 || ssmaj != 5 || ssmin != 1) { \
	                        printf "subsystem-version: FAIL os=%s.%s subsys=%s.%s\n", osmaj, osmin, ssmaj, ssmin; \
	                        exit 1 \
	                      } \
	                      print "subsystem-version: OK (5.1)" \
	                    }'
	@touch $@

clean:
	rm -f $(EXES) .tested.*.exe subsystem-version.tested

.PHONY: all test clean
