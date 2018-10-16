if("v1.2.0" STREQUAL "")
  message(FATAL_ERROR "Tag for git checkout should not be empty.")
endif()

set(run 0)

if("/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitinfo.txt" IS_NEWER_THAN "/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitclone-lastrun.txt")
  set(run 1)
endif()

if(NOT run)
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: '/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E remove_directory "/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake'")
endif()

# try the clone 3 times incase there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git" clone --origin "origin" "https://github.com/facebook/zstd.git" "cmake"
    WORKING_DIRECTORY "/home/samuel/workspace/ceph/src/zstd2/zstd/build"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/facebook/zstd.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git" checkout v1.2.0
  WORKING_DIRECTORY "/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'v1.2.0'")
endif()

execute_process(
  COMMAND "/usr/bin/git" submodule init 
  WORKING_DIRECTORY "/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to init submodules in: '/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake'")
endif()

execute_process(
  COMMAND "/usr/bin/git" submodule update --recursive 
  WORKING_DIRECTORY "/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitinfo.txt"
    "/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitclone-lastrun.txt"
  WORKING_DIRECTORY "/home/samuel/workspace/ceph/src/zstd2/zstd/build/cmake"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/home/samuel/workspace/ceph/src/compressor/zstdmt/external/src/zstdmt_ext-stamp/zstdmt_ext-gitclone-lastrun.txt'")
endif()

