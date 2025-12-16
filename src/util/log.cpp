// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/log.h>

#include <threadsafety.h>

#include <iterator>
#include <utility>

namespace util::log {

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

bool Logger::Enabled() const
{
    StdLockGuard lock(m_mutex);
    return !m_callbacks.empty();
}

Logger& GetLogger()
{
    /**
     * NOTE: the logger instances is leaked on exit. This is ugly, but will be
     * cleaned up by the OS/libc. Defining a logger as a global object doesn't work
     * since the order of destruction of static/global objects is undefined.
     * Consider if the logger gets destroyed, and then some later destructor calls
     * Log, maybe indirectly, and you get a core dump at shutdown trying to
     * access the logger. When the shutdown sequence is fully audited and tested,
     * explicit destruction of these objects can be implemented by changing this
     * from a raw pointer to a std::unique_ptr.
     *
     * This method of initialization was originally introduced in
     * ee3374234c60aba2cc4c5cd5cac1c0aefc2d817c.
     */
    static Logger* g_logger{new Logger()};
    return *g_logger;
}

} // namespace util::log
