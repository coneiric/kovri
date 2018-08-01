cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(kovri-fuzzer CXX)

add_executable(kovri-fuzzer
  driver.cc
  i2pcontrol.cc
  i2pcontrol.h
  identity.cc
  identity.h
  lease_set.cc
  lease_set.h
  routerinfo.cc
  routerinfo.h
  su3.cc
  su3.h
  target.h)

target_link_libraries(kovri-fuzzer PRIVATE
  kovri-private
  kovri-client)

# Set compiler flags for fuzzing.
# - `-no-pie` needed as a workaround for https://github.com/google/sanitizers/issues/856
# - another option is to build without the address sanitizer
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-pie -fsanitize-coverage=trace-pc-guard -fsanitize=fuzzer,address")

if (COMMAND cotire)
  set_target_properties(kovri-fuzzer PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/deps")
  cotire(kovri-fuzzer)

  set(tests_target "kovri-fuzzer_unity")
else()
  set(tests_target "kovri-fuzzer")
endif()

add_test(NAME KovriFuzzer COMMAND ${tests_target})

install(TARGETS kovri-fuzzer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
