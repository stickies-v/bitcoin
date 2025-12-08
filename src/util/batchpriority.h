// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BATCHPRIORITY_H
#define BITCOIN_UTIL_BATCHPRIORITY_H

#include <util/expected.h>

#include <string>

/**
 * On platforms that support it, tell the kernel the calling thread is
 * CPU-intensive and non-interactive. See SCHED_BATCH in sched(7) for details.
 * Returns an error string on failure.
 *
 */
util::Expected<void, std::string> ScheduleBatchPriority();

#endif // BITCOIN_UTIL_BATCHPRIORITY_H
