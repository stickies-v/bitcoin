# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Generic helpers built on the C++ standard library.
add_library(bitcoin_util STATIC EXCLUDE_FROM_ALL
  bytevectorhash.cpp
  chaintype.cpp
  check.cpp
  exception.cpp
  expected.cpp
  feefrac.cpp
  fs.cpp
  fs_helpers.cpp
  hasher.cpp
  moneystr.cpp
  rbf.cpp
  readwritefile.cpp
  serfloat.cpp
  signalinterrupt.cpp
  strencodings.cpp
  string.cpp
  syserror.cpp
  thread.cpp
  threadinterrupt.cpp
  threadnames.cpp
  time.cpp
  tokenpipe.cpp
  ../logging.cpp
  ../random.cpp
  ../randomenv.cpp
  ../streams.cpp
  ../sync.cpp
)

target_link_libraries(bitcoin_util
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_crypto
    $<$<PLATFORM_ID:Windows>:ws2_32>
    $<$<PLATFORM_ID:Windows>:bcrypt>
)

# Networking primitives: sockets and network address handling.
add_library(bitcoin_net STATIC EXCLUDE_FROM_ALL
  asmap.cpp
  sock.cpp
)

target_link_libraries(bitcoin_net
  PRIVATE
    core_interface
    bitcoin_crypto
    bitcoin_util
    $<$<PLATFORM_ID:Windows>:ws2_32>
)

# Operating system services beyond the C++ standard library.
add_library(bitcoin_system STATIC EXCLUDE_FROM_ALL
  batchpriority.cpp
  exec.cpp
)

target_link_libraries(bitcoin_system
  PRIVATE
    core_interface
)
