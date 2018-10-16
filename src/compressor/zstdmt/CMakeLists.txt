# zstdmt

FIND_LIBRARY(ZSTD_LIBRARY libzstd.a /usr/lib /usr/local/lib
        ${CMAKE_BINARY_DIR}/src/compressor/zstd/libzstd/lib)

IF(NOT ZSTD_LIBRARY-NOTFOUND)
add_library(zstd STATIC IMPORTED)
set_target_properties(zstd PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src/zstd/lib"
  IMPORTED_LOCATION "${ZSTD_LIBRARY}")
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib ${CMAKE_CURRENT_LIST_DIR}/libzstdmt)
set(zstdmt_sources
  CompressionPluginZstdMt.cc
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_common.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_compress.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/zstd-mt_decompress.c
  zstd-mt.c
  ${CMAKE_CURRENT_LIST_DIR}/lib/threading.c
)


add_library(ceph_zstdmt SHARED ${zstdmt_sources})
target_link_libraries(ceph_zstdmt PRIVATE zstd)
set_target_properties(ceph_zstdmt PROPERTIES
        VERSION 2.0.0
        SOVERSION 2
        INSTALL_RPATH "")
install(TARGETS ceph_zstdmt DESTINATION ${compressor_plugin_dir})
MESSAGE("AAAAAAAAAAAAAAAAAAA")

ELSE()
MESSAGE("ERROR")
ENDIF()

