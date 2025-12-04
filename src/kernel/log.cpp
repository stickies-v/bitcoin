// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/log.h>
#include <threadsafety.h>

#include <util/threadnames.h>

#include <iterator>
#include <source_location>
#include <string>
#include <utility>

namespace kernel {

Logger& GetLogger()
{
    static Logger logger;
    return logger;
}

Logger::CallbackHandle Logger::RegisterCallback(Callback callback)
{
    StdLockGuard lock(m_mutex);
    m_callbacks.push_back(std::move(callback));
    return std::prev(m_callbacks.end());
}

void Logger::UnregisterCallback(CallbackHandle handle)
{
    StdLockGuard lock(m_mutex);
    m_callbacks.erase(handle);
}

void Logger::DoCallbacks(
    Level level,
    Category category,
    const std::string& message,
    std::source_location source_loc)
{
    LogEntry entry{
        .level = level,
        .category = category,
        .message = message,
        .source_loc = source_loc,
        .timestamp = std::chrono::system_clock::now(),
        .thread_name = util::ThreadGetInternalName(),
    };

    StdLockGuard lock(m_mutex);
    for (const auto& callback : m_callbacks) {
        callback(entry);
    }
}

bool Logger::Enabled() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    StdLockGuard lock(m_mutex);
    return !m_callbacks.empty();
}

} // namespace kernel
