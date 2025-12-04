// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_KERNEL_LOGGING_BRIDGE_H
#define BITCOIN_NODE_KERNEL_LOGGING_BRIDGE_H

#include <kernel/log.h>

namespace node {

//! Bridge that forwards kernel log messages to the node's logging infrastructure.
//! Registers a callback with kernel::GetLogger() on construction and unregisters
//! on destruction.
class KernelLoggingBridge
{
public:
    KernelLoggingBridge();
    ~KernelLoggingBridge();

    // Non-copyable, non-movable
    KernelLoggingBridge(const KernelLoggingBridge&) = delete;
    KernelLoggingBridge& operator=(const KernelLoggingBridge&) = delete;

private:
    kernel::Logger::CallbackHandle m_callback_handle;
};

} // namespace node

#endif // BITCOIN_NODE_KERNEL_LOGGING_BRIDGE_H
