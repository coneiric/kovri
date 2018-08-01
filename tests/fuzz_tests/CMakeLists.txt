cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(kovri-fuzzer CXX)

add_executable(kovri-fuzzer
  i2pcontrol.cc
  lease_set.cc
  routerinfo.cc
  su3.cc
  i2pcontrol.h
  lease_set.h
  routerinfo.h
  su3.h
  target.h
  driver.cc)

target_link_libraries(kovri-fuzzer PRIVATE
  kovri-private
  kovri-client)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-coverage=trace-pc-guard -fsanitize=fuzzer,address")

if (COMMAND cotire)
  set_target_properties(kovri-fuzzer PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/deps")
  cotire(kovri-fuzzer)

  set(tests_target kovri-fuzzer_unity)
else()
  set(tests_target kovri-fuzzer)
endif()

add_test(NAME KovriFuzzer COMMAND ${tests_target})

install(TARGETS kovri-fuzzer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
