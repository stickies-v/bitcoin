// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_LOG_H
#define BITCOIN_KERNEL_LOG_H

#include <threadsafety.h>
#include <tinyformat.h>
#include <util/string.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <source_location>
#include <string>

namespace kernel {

enum class Level {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
};

enum class Category {
    ALL,
    BENCH,
    BLOCKSTORAGE,
    COINDB,
    ESTIMATEFEE,
    KERNEL,
    LEVELDB,
    MEMPOOL,
    PRUNE,
    RAND,
    REINDEX,
    TXPACKAGES,
    VALIDATION,
};

struct LogEntry {
    Level level;
    Category category;
    std::string message;
    std::source_location source_loc;
    std::chrono::system_clock::time_point timestamp;
    std::string thread_name;
};

class Logger
{
public:
    using Callback = std::function<void(const LogEntry&)>;
    using CallbackHandle = std::list<Callback>::iterator;

    //! Register a callback to receive log entries. Returns a handle that
    //! can be used to unregister the callback.
    [[nodiscard]] CallbackHandle RegisterCallback(Callback callback) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    //! Unregister a previously registered callback.
    void UnregisterCallback(CallbackHandle handle) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    //! Set the minimum log level. Messages below this level are discarded
    //! before formatting to avoid overhead.
    void SetMinLevel(Level level) { m_min_level.store(level, std::memory_order_relaxed); }

    //! Get the current minimum log level.
    Level GetMinLevel() const { return m_min_level.load(std::memory_order_relaxed); }

    //! Returns true if a message at the given level would be logged.
    bool WillLog(Level level) const { return level >= m_min_level.load(std::memory_order_relaxed); }

    //! Format and log a message. No-op if not enabled or level does not meet minimum level.
    template <typename... Args>
    inline void Log(
        Level level,
        Category category,
        std::source_location loc,
        util::ConstevalFormatString<sizeof...(Args)> fmt,
        const Args&... args) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (!Enabled() || !WillLog(level)) return;

        std::string message;
        try {
            message = tfm::format(fmt, args...);
        } catch (const tinyformat::format_error& e) {
            message = std::string{"Format error: "} + e.what() + " in: " + fmt.fmt;
        }
        DoCallbacks(level, category, message, loc);
    }

    //! Returns true if any callbacks are registered.
    bool Enabled() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    mutable StdMutex m_mutex;
    std::list<Callback> m_callbacks GUARDED_BY(m_mutex);
    std::atomic<Level> m_min_level{Level::Debug};

    //! Unconditionally execute all callbacks for the given message.
    void DoCallbacks(
        Level level,
        Category category,
        const std::string& message,
        std::source_location source_loc) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

//! Get the global kernel logger instance.
Logger& GetLogger();
} // namespace kernel

//! Log at Info/Warning/Error level. Category defaults to ALL.
#define KernelLogInfo(...) \
    kernel::GetLogger().Log(kernel::Level::Info, kernel::Category::ALL, std::source_location::current(), __VA_ARGS__)
#define KernelLogWarning(...) \
    kernel::GetLogger().Log(kernel::Level::Warning, kernel::Category::ALL, std::source_location::current(), __VA_ARGS__)
#define KernelLogError(...) \
    kernel::GetLogger().Log(kernel::Level::Error, kernel::Category::ALL, std::source_location::current(), __VA_ARGS__)

//! Log at Debug/Trace level. Category is required.
#define KernelLogDebug(category, ...) \
    kernel::GetLogger().Log(kernel::Level::Debug, category, std::source_location::current(), __VA_ARGS__)
#define KernelLogTrace(category, ...) \
    kernel::GetLogger().Log(kernel::Level::Trace, category, std::source_location::current(), __VA_ARGS__)

#endif // BITCOIN_KERNEL_LOG_H
