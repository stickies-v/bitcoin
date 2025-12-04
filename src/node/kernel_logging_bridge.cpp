// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/kernel_logging_bridge.h>

#include <kernel/log.h>
#include <logging.h>

namespace node {

namespace {

//! Map kernel::Category to BCLog::LogFlags
BCLog::LogFlags MapCategory(kernel::Category cat)
{
    switch (cat) {
    case kernel::Category::ALL: return BCLog::ALL;
    case kernel::Category::BENCH: return BCLog::BENCH;
    case kernel::Category::BLOCKSTORAGE: return BCLog::BLOCKSTORAGE;
    case kernel::Category::COINDB: return BCLog::COINDB;
    case kernel::Category::ESTIMATEFEE: return BCLog::ESTIMATEFEE;
    case kernel::Category::KERNEL: return BCLog::KERNEL;
    case kernel::Category::LEVELDB: return BCLog::LEVELDB;
    case kernel::Category::MEMPOOL: return BCLog::MEMPOOL;
    case kernel::Category::PRUNE: return BCLog::PRUNE;
    case kernel::Category::RAND: return BCLog::RAND;
    case kernel::Category::REINDEX: return BCLog::REINDEX;
    case kernel::Category::TXPACKAGES: return BCLog::TXPACKAGES;
    case kernel::Category::VALIDATION: return BCLog::VALIDATION;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

//! Map kernel::Level to BCLog::Level
BCLog::Level MapLevel(kernel::Level level)
{
    switch (level) {
    case kernel::Level::Trace: return BCLog::Level::Trace;
    case kernel::Level::Debug: return BCLog::Level::Debug;
    case kernel::Level::Info: return BCLog::Level::Info;
    case kernel::Level::Warning: return BCLog::Level::Warning;
    case kernel::Level::Error: return BCLog::Level::Error;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

} // namespace

KernelLoggingBridge::KernelLoggingBridge()
{
    m_callback_handle = kernel::GetLogger().RegisterCallback([](const kernel::LogEntry& entry) {
        // Forward to node's logging infrastructure, replicating LogPrintLevel's
        // category filtering and rate limiting logic while preserving the
        // kernel's original source location.
        BCLog::LogFlags category{MapCategory(entry.category)};
        BCLog::Level level{MapLevel(entry.level)};

        if (!LogAcceptCategory(category, level)) {
            return;
        }

        std::source_location loc{entry.source_loc};
        bool should_ratelimit{level >= BCLog::Level::Info};
        LogInstance().LogPrintStr(entry.message, std::move(loc), category, level, should_ratelimit);
    });
}

KernelLoggingBridge::~KernelLoggingBridge()
{
    kernel::GetLogger().UnregisterCallback(m_callback_handle);
}

} // namespace node
