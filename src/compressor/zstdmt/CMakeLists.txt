# zstd

# libzstd - build it statically
set(ZSTDMT_C_FLAGS "-fPIC -Wno-unused-variable -O0 -g3")

include(ExternalProject)

ExternalProject_Add(zstdmt_ext
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/external/zstd/build/cmake
  CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_C_FLAGS=${ZSTDMT_C_FLAGS}
             -DCMAKE_AR=${CMAKE_AR}
             -DZSTD_BUILD_STATIC=ON
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libzstdmt
  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND "true")

# force zstd make to be called on each time
ExternalProject_Add_Step(zstdmt_ext forcebuild
  DEPENDEES configure
  DEPENDERS build
  COMMAND "true"
  ALWAYS 1)

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib ${CMAKE_CURRENT_LIST_DIR}/libzstdmt ${CMAKE_SOURCE_DIR}/src/external/zstd/lib)

add_library(zstd2 STATIC IMPORTED)
set_property(TARGET zstd2 PROPERTY
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/libzstdmt/lib/libzstd.a")
add_dependencies(zstd2 zstdmt_ext)
set(ZSTDMT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/external/zstd/lib ${CMAKE_CURRENT_LIST_DIR}/libzstdmt)

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
target_link_libraries(ceph_zstdmt zstd2)
install(TARGETS ceph_zstdmt DESTINATION ${compressor_plugin_dir})

if(WITH_EMBEDDED)
  add_library(cephd_compressor_zstdmt STATIC ${zstdmt_sources})
  set_target_properties(cephd_compressor_zstdmt PROPERTIES COMPILE_DEFINITIONS BUILDING_FOR_EMBEDDED)
endif()
