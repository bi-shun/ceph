# zstd

# libzstd - build it statically
set(ZSTDMT_C_FLAGS "-fPIC -Wno-unused-variable -O3")

include(ExternalProject)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib ${CMAKE_SOURCE_DIR}/src/zstd/lib)

ExternalProject_Add(zstdmt_ext
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/zstd/build/cmake
  CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_C_FLAGS=${ZSTDMT_C_FLAGS}
             -DCMAKE_AR=${CMAKE_AR}
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libzstd
  BUILD_COMMAND $(MAKE) libzstd_static
  INSTALL_COMMAND "true")

# force zstd make to be called on each time
ExternalProject_Add_Step(zstdmt_ext forcebuild
  DEPENDEES configure
  DEPENDERS build
  COMMAND "true"
  ALWAYS 1)

add_library(zstd STATIC IMPORTED)
set_property(TARGET zstd PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/libzstd/lib/libzstd.a")
add_dependencies(zstd zstdmt_ext)
set(ZSTDMT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/zstd/lib)

#
#
set(zstdmt_sources
  CompressionPluginZstdMt.cc
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_common.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_compress.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_decompress.c
  zstd-mt.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/threading.c
)

add_library(ceph_zstdmt SHARED ${zstdmt_sources})
add_dependencies(ceph_zstdmt ${CMAKE_SOURCE_DIR}/src/ceph_ver.h)
target_link_libraries(ceph_zstdmt zstd)
set_target_properties(ceph_zstdmt PROPERTIES
  VERSION 2.0.0
  SOVERSION 2
  INSTALL_RPATH "")
install(TARGETS ceph_zstdmt DESTINATION ${compressor_plugin_dir})

if(WITH_EMBEDDED)
  add_library(cephd_compressor_zstdmt STATIC ${zstdmt_sources})
  set_target_properties(cephd_compressor_zstdmt PROPERTIES COMPILE_DEFINITIONS BUILDING_FOR_EMBEDDED)
endif()
