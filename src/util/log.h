// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <logging/categories.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/string.h>
#include <util/threadnames.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <source_location>
#include <string>

namespace util::log {

enum class Level {
    Trace = 0, // High-volume or detailed logging for development/debugging
    Debug,     // Reasonably noisy logging, but still usable in production
    Info,      // Default
    Warning,
    Error,
};

//! Structured log entry passed to registered callbacks.
struct Entry {
    Level level;
    uint64_t category; // Opaque to util::log; interpreted by consumers (e.g., BCLog::LogFlags)
    std::string message;
    std::source_location source_loc{std::source_location::current()};
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::string thread_name{util::ThreadGetInternalName()};
    bool should_ratelimit{true}; // Hint for consumers; typically true for level >= Info
};

//! Minimal logger that dispatches structured log entries to registered callbacks.
//! Thread-safe. Does not perform formatting, rate limiting, or category filtering.
class Logger
{
public:
    using Callback = std::function<void(const Entry&)>;
    using CallbackHandle = std::list<Callback>::iterator;

    //! Register a callback to receive log entries. Returns a handle for unregistration.
    [[nodiscard]] CallbackHandle RegisterCallback(Callback callback) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    //! Unregister a previously registered callback.
    void UnregisterCallback(CallbackHandle handle) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    //! Set the minimum log level. Messages below this level are discarded.
    void SetMinLevel(Level level) { m_min_level.store(level, std::memory_order_relaxed); }

    //! Get the current minimum log level.
    Level GetMinLevel() const { return m_min_level.load(std::memory_order_relaxed); }

    //! Returns true if a message at the given level would be logged.
    bool WillLog(Level level) const { return level >= GetMinLevel(); }

    //! Returns true if any callbacks are registered.
    bool Enabled() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    //! Format message and dispatch to all registered callbacks. No-op if minimum log level not met.
    template <typename... Args>
    void Log(Level level, uint64_t category, std::source_location loc, bool should_ratelimit,
             util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (!Enabled() || !WillLog(level)) return;

        std::string message;
        try {
            message = tfm::format(fmt, args...);
        } catch (const tinyformat::format_error& e) {
            message = std::string{"Format error: "} + e.what() + " in: " + fmt.fmt;
        }

        Entry entry{
            .level = level,
            .category = category,
            .message = std::move(message),
            .source_loc = loc,
            .timestamp = std::chrono::system_clock::now(),
            .thread_name = util::ThreadGetInternalName(),
            .should_ratelimit = should_ratelimit,
        };

        StdLockGuard lock(m_mutex);
        for (const auto& callback : m_callbacks) {
            callback(entry);
        }
    }

private:
    mutable StdMutex m_mutex;
    std::list<Callback> m_callbacks GUARDED_BY(m_mutex);
    std::atomic<Level> m_min_level{Level::Debug};
};

//! Global logger instance.
Logger& GetLogger();

} // namespace util::log

#define LogPrintLevel_(category, level, should_ratelimit, ...) \
    util::log::GetLogger().Log(level, static_cast<uint64_t>(category), std::source_location::current(), should_ratelimit, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)

// Deprecated unconditional logging.
#define LogPrintf(...) LogInfo(__VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log with the specified category and severity level.
// Level filtering is done here; category filtering and output formatting happen
// in BCLog::Sink. If level >= Info, logging to disk is rate-limited. This is
// important so that callers don't need to worry about accidentally introducing
// a disk-fill vulnerability. Additionally, users specifying -debug are assumed
// to be developers or power users who are aware that -debug may cause excessive
// disk usage due to logging.
#define LogPrintLevel(category, level, ...)                           \
    do {                                                              \
        if (util::log::GetLogger().WillLog(level)) {                  \
            bool rate_limit{level >= util::log::Level::Info};         \
            LogPrintLevel_(category, level, rate_limit, __VA_ARGS__); \
        }                                                             \
    } while (0)

// Log conditionally at Debug/Trace level with the specified category.
#define LogDebug(category, ...) LogPrintLevel(category, util::log::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) LogPrintLevel(category, util::log::Level::Trace, __VA_ARGS__)

#endif // BITCOIN_UTIL_LOG_H
