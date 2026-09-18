# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(GNUInstallDirs)

# libbitcoinkernel is built from the source lists of the libraries in
# `bitcoin_kernel`, so the external and internal builds cannot use
# different sources. The sources are compiled again rather than linked
# from the static archives, so that this target can use its own symbol
# visibility and compile options.
add_library(bitcoinkernel
  bitcoinkernel.cpp
  $<TARGET_OBJECTS:bitcoin_clientversion>
  $<TARGET_OBJECTS:bitcoin_crypto>
  $<TARGET_OBJECTS:leveldb>
  $<TARGET_OBJECTS:crc32c>
)

function(add_sources_of_library target library)
  get_target_property(sources ${library} SOURCES)
  get_target_property(source_dir ${library} SOURCE_DIR)
  foreach(source IN LISTS sources)
    if(source MATCHES "^\\$<")
      message(FATAL_ERROR "${library} has a generator expression in its sources, which ${target} cannot compile from: ${source}")
    endif()
    cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY "${source_dir}" NORMALIZE)
    target_sources(${target} PRIVATE "${source}")
  endforeach()
endfunction()

# `bitcoin_kernel` must list plain target names, added from the directory
# that defines it. Entries added from another directory or through
# generator expressions are not target names and cannot be resolved here.
get_target_property(kernel_libraries bitcoin_kernel INTERFACE_LINK_LIBRARIES)
foreach(library IN LISTS kernel_libraries)
  if(NOT TARGET "${library}")
    message(FATAL_ERROR "bitcoin_kernel entry is not a target: ${library}")
  endif()
  # bitcoin_crypto has per-file compile options, so its objects are reused above.
  if(NOT library STREQUAL "bitcoin_crypto")
    add_sources_of_library(bitcoinkernel ${library})
  endif()
endforeach()

# Compiler warnings that apply only to the kernel and its dependencies.
# These can be more strict and/or warnings that only apply to shared
# libs.
add_library(kernel_warn_interface INTERFACE)
if(NOT MSVC)
  try_append_cxx_flags("-Wunique-object-duplication" TARGET kernel_warn_interface SKIP_LINK)
endif()

# Also manually apply the warnings to the kernel's internal dependencies
target_link_libraries(bitcoin_clientversion PRIVATE kernel_warn_interface)
target_link_libraries(bitcoin_crypto PRIVATE kernel_warn_interface)
target_link_libraries(leveldb PRIVATE kernel_warn_interface)
target_link_libraries(crc32c PRIVATE kernel_warn_interface)

target_link_libraries(bitcoinkernel
  PRIVATE
    core_interface
    kernel_warn_interface
    secp256k1_objs
    $<$<PLATFORM_ID:Windows>:bcrypt>
    $<TARGET_NAME_IF_EXISTS:USDT::headers>
    $<TARGET_NAME_IF_EXISTS:Boost::headers>
)

target_include_directories(bitcoinkernel PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb/include>)

# Add a convenience libbitcoinkernel target as a synonym for bitcoinkernel.
add_custom_target(libbitcoinkernel)
add_dependencies(libbitcoinkernel bitcoinkernel)

get_target_property(bitcoinkernel_type bitcoinkernel TYPE)
if(bitcoinkernel_type STREQUAL "STATIC_LIBRARY")
  target_compile_definitions(bitcoinkernel PUBLIC BITCOINKERNEL_STATIC)
endif()

configure_file(${PROJECT_SOURCE_DIR}/libbitcoinkernel.pc.in ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT libbitcoinkernel)

install(TARGETS bitcoinkernel
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT libbitcoinkernel
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT libbitcoinkernel
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT libbitcoinkernel
)

install(FILES bitcoinkernel.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT libbitcoinkernel)
