# Copyright (c) 2015-2018, The Kovri I2P Router Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Get custom Kovri data path + set appropriate CMake generator.
# If no path is given, set default path
SHELL := $(shell which bash)
system := $(shell uname)
ifeq ($(KOVRI_DATA_PATH),)
  ifeq ($(system), Linux)
    data-path = $(HOME)/.kovri
  endif
  ifeq ($(system), Darwin)
    data-path = $(HOME)/Library/Application\ Support/Kovri
  endif
  ifneq (, $(findstring BSD, $(system))) # We should support other BSD's
    data-path = $(HOME)/.kovri
  endif
  ifeq ($(system), DragonFly)
    data-path = $(HOME)/.kovri
  endif
  ifneq (, $(findstring MINGW, $(system)))
    data-path = "$(APPDATA)"\\Kovri
    cmake-gen = -G 'MSYS Makefiles'
  endif
else
  data-path = $(KOVRI_DATA_PATH)
endif

# By default cotire enabled
COTIRE ?= 1

ifeq ($(COTIRE), 1)
  cmake_target = all_unity
  cmake_cotire = -D WITH_COTIRE=ON
else
  cmake_target = all
  cmake_cotire = -D WITH_COTIRE=OFF
endif

# Our base cmake command
cmake = cmake $(cmake-gen) $(cmake_cotire)

# Release types
cmake-debug = $(cmake) -D CMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake-release = $(cmake) -D CMAKE_BUILD_TYPE=Release

# Current off-by-default Kovri build options
cmake-optimize   = -D WITH_OPTIMIZE=ON
cmake-hardening  = -D WITH_HARDENING=ON
cmake-tests      = -D WITH_TESTS=ON
cmake-fuzz-tests = -D WITH_FUZZ_TESTS=ON
cmake-static     = -D WITH_STATIC=ON
cmake-static-deps= -D WITH_STATIC_DEPS=ON
cmake-shared-deps= -D WITH_SHARED_DEPS=ON
cmake-doxygen    = -D WITH_DOXYGEN=ON
cmake-coverage   = -D WITH_COVERAGE=ON
cmake-python     = -D WITH_PYTHON=ON
cmake-kovri-util = -D WITH_KOVRI_UTIL=ON

# Android-specific
cmake-android = -D ANDROID=1 -D KOVRI_DATA_PATH="/data/local/tmp/.kovri"

# Native
cmake-native = -DCMAKE_CXX_FLAGS="-march=native"
cryptopp-native = CXXFLAGS="-march=native -DCRYPTOPP_NO_CPU_FEATURE_PROBES=1"  # Refs #699

# Filesystem
build = build/
build-cpp-netlib = deps/cpp-netlib/$(build)
build-cryptopp = deps/cryptopp/  # No longer using CMake
build-doxygen = doc/Doxygen
build-fuzzer = contrib/Fuzzer/$(build)

# CMake builder macros
define CMAKE
  cmake -E make_directory $1
  cmake -E chdir $1 $2 ../
endef

define MAKE_CRYPTOPP
  @echo "=== Building cryptopp ==="
  # Remove as many unnecessary ciphers/files as possible,
  # without breaking the build, in order to reduce build-time.
  cd $(build-cryptopp) && \
  rm -f \
  3way.cpp \
  3way.h \
  arc4.cpp \
  arc4.h \
  aria.cpp \
  aria.h \
  aria_simd.cpp \
  ariatab.cpp \
  bench.h \
  bench1.cpp \
  bench2.cpp \
  bfinit.cpp \
  blake2.cpp \
  blake2.h \
  blake2b_simd.cpp \
  blake2s_simd.cpp \
  blowfish.cpp \
  blowfish.h \
  blumshub.cpp \
  blumshub.h \
  camellia.cpp \
  camellia.h \
  cast.cpp \
  cast.h \
  casts.cpp \
  chacha.cpp \
  chacha.h \
  chacha_avx.cpp \
  chacha_simd.cpp \
  cham.cpp \
  cham.h \
  cham_simd.cpp \
  darn.cpp \
  darn.h \
  datatest.cpp \
  default.cpp \
  default.h \
  eax.cpp \
  eax.h \
  esign.cpp \
  esign.h \
  fipsalgt.cpp \
  fipstest.cpp \
  gost.cpp \
  gost.h \
  hc128.cpp \
  hc128.h \
  hc256.cpp \
  hc256.h \
  hight.cpp \
  hight.h \
  ida.cpp \
  ida.h \
  idea.cpp \
  idea.h \
  interhash* \
  kalyna.cpp \
  kalyna.h \
  kalynatab.cpp \
  keccak.cpp \
  keccak.h \
  keccakc.cpp \
  lea.cpp \
  lea.h \
  lea_simd.cpp \
  luc.cpp \
  luc.h \
  mars.cpp \
  mars.h \
  marss.cpp \
  md2.cpp \
  md2.h \
  md4.cpp \
  md4.h \
  panama.cpp \
  panama.h \
  poly1305.cpp \
  poly1305.h \
  ppc_power7.cpp \
  ppc_power8.cpp \
  ppc_power9.cpp \
  ppc_simd.cpp \
  ppc_simd.h \
  rabbit.cpp \
  rabbit.h \
  rabin.cpp \
  rabin.h \
  rc2.cpp \
  rc2.h \
  rc5.cpp \
  rc5.h \
  rc6.cpp \
  rc6.h \
  safer.cpp \
  safer.h \
  salsa.cpp \
  salsa.h \
  scrypt.cpp \
  scrypt.h \
  seal.cpp \
  seal.h \
  serpent.cpp \
  serpent.h \
  serpentp.h \
  sha3.cpp \
  sha3.h \
  shacal2.cpp \
  shacal2.h \
  shacal2_simd.cpp \
  shark.cpp \
  shark.h \
  sharkbox.cpp \
  simeck.cpp \
  simeck.h \
  simeck_simd.cpp \
  simon.cpp \
  simon.h \
  simon128_simd.cpp \
  simon64_simd.cpp \
  sm3.cpp \
  sm3.h \
  sm4.cpp \
  sm4.h \
  sm4_simd.cpp \
  sosemanuk.cpp \
  sosemanuk.h \
  speck-simd.cpp \
  speck.cpp \
  speck.h \
  speck128_simd.cpp \
  speck64_simd.cpp \
  square.cpp \
  square.h \
  squaretb.cpp \
  tea.cpp \
  tea.h \
  tftables.cpp \
  threefish.cpp \
  threefish.h \
  tiger.cpp \
  tiger.h \
  tigertab.cpp \
  ttmac.cpp \
  ttmac.h \
  twofish.cpp \
  twofish.h \
  vmac.cpp \
  vmac.h \
  wake.cpp \
  wake.h \
  xtr.cpp \
  xtr.h \
  xtrcrypt.cpp \
  xtrcrypt.h \
  && $1
endef

define CMAKE_FUZZER
  @echo "=== Building fuzzer ==="
  $(eval cmake-fuzzer = $(cmake-release) -DLLVM_USE_SANITIZER=Address -DLLVM_USE_SANITIZE_COVERAGE=YES \
      -DCMAKE_CXX_FLAGS="-g -O2 -fno-omit-frame-pointer -std=c++11" $1)
  $(call CMAKE,$(build-fuzzer),$(cmake-fuzzer))
endef

# Targets
all: dynamic

#--------------------------------#
# Dependency build types/options #
#--------------------------------#

deps:
	$(call MAKE_CRYPTOPP, $(MAKE) $(cryptopp-native) static)

shared-deps:
	$(eval cmake-debug += $(cmake-shared-deps))
	$(call MAKE_CRYPTOPP, $(MAKE) shared)

release-deps:
	$(call MAKE_CRYPTOPP, $(MAKE) static)

release-static-deps:
	$(eval cmake-release += $(cmake-static-deps))
	$(call MAKE_CRYPTOPP, $(MAKE) static)

#-----------------------------------#
# For local, end-user cloned builds #
#-----------------------------------#

dynamic: shared-deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-native))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

static: deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-native) $(cmake-static))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

debug: deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-native) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

#-----------------------------------#
# For dynamic distribution release #
#-----------------------------------#

release: release-deps
        # TODO(unassigned): optimizations/hardening when we're out of alpha
	$(eval cmake-kovri += $(cmake-release) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

#--------------------------------------------------------------#
# For static distribution release (website and nightly builds) #
#--------------------------------------------------------------#

release-static: release-static-deps
        # TODO(unassigned): optimizations/hardening when we're out of alpha
	$(eval cmake-kovri += $(cmake-release) $(cmake-static) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

release-static-android: release-static-deps
	$(eval cmake-kovri += $(cmake-release) $(cmake-static) $(cmake-android) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

#-----------------#
# Optional builds #
#-----------------#

# Utility binary
util: deps
	$(eval $(cmake-kovri) += $(cmake-debug) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# For API/testnet development
python: shared-deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-python))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce optimized, hardened binary
all-options: deps
	$(eval cmake-kovri += $(cmake-release) $(cmake-optimize) $(cmake-hardening) $(cmake-kovri-util))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce optimized, hardened binary. Note: we need (or very much should have) optimizations with hardening
optimized-hardened: deps
	$(eval cmake-kovri += $(cmake-release) $(cmake-optimize) $(cmake-hardening))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce all unit-tests with optimized hardening
optimized-hardened-tests: deps
	$(eval cmake-kovri += $(cmake-release) $(cmake-optimize) $(cmake-hardening) $(cmake-tests))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce build with coverage. Note: leaving out hardening because of need for optimizations
coverage: deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-coverage) $(cmake-kovri-util) $(cmake-tests))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce vanilla unit-tests
tests: deps
	$(eval cmake-kovri += $(cmake-debug) $(cmake-tests))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce vanilla fuzzer-tests
fuzz-tests: deps
	$(call CMAKE_FUZZER) && $(MAKE)
	$(eval cmake-kovri += $(cmake-debug) $(cmake-fuzz-tests))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) $(cmake_target)

# Produce Doxygen documentation
doxygen:
	$(eval cmake-kovri += $(cmake) $(cmake-doxygen))
	$(call CMAKE,$(build),$(cmake-kovri)) && $(MAKE) -C $(build) doc

# Produce available CMake build options
help:
	# TODO(unassigned): fix (we must actually change directory)
	$(call CMAKE,$(build) -LH)

# Clean all build directories and Doxygen output
clean:
	$(eval remove-build = rm -fR $(build) $(build-cpp-netlib) $(build-doxygen) $(build-fuzzer) && cd $(build-cryptopp) && $(MAKE) clean)
	@if [ "$$FORCE_CLEAN" = "yes" ]; then $(remove-build); \
	else echo "CAUTION: This will remove the build directories for Kovri and all submodule dependencies, and remove all Doxygen output"; \
	read -r -p "Is this what you wish to do? (y/N)?: " CONFIRM; \
	  if [ $$CONFIRM = "y" ] || [ $$CONFIRM = "Y" ]; then $(remove-build); \
          else echo "Exiting."; exit 1; \
          fi; \
        fi

# Install binaries and package
install:
	@_install="./pkg/installers/kovri-install.sh"; \
	if [ -e $$_install ]; then $$_install; else echo "Unable to find $$_install"; exit 1; fi

# Un-install binaries and package
uninstall:
	@_install="./pkg/installers/kovri-install.sh"; \
	if [ -e $$_install ]; then $$_install -u; else echo "Unable to find $$_install"; exit 1; fi

.PHONY: all deps release-deps release-static-deps dynamic static debug release release-static release-static-android all-options optimized-hardened optimized-hardened-tests coverage coverage-tests tests doxygen help clean install uninstall
