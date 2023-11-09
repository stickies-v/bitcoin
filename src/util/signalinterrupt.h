// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SIGNALINTERRUPT_H
#define BITCOIN_UTIL_SIGNALINTERRUPT_H

#ifdef WIN32
#include <condition_variable>
#include <mutex>
#else
#include <util/tokenpipe.h>
#endif

#include <atomic>
#include <cstdlib>
#include <functional>

namespace util {
/**
 * Helper class that manages an interrupt flag, and allows a thread or
 * signal to interrupt another thread.
 *
 * This class is safe to be used in a signal handler. If sending an interrupt
 * from a signal handler is not necessary, the more lightweight \ref
 * CThreadInterrupt class can be used instead.
 */

class SignalInterrupt
{
public:
    SignalInterrupt(
        std::function<void()> failed_token_read_cb = nullptr,
        std::function<void()> failed_token_write_cb = nullptr
    );
    explicit operator bool() const;
    bool operator()();
    bool reset();
    bool wait();

private:
    std::atomic<bool> m_flag{false};
    //! (optional) callback to be executed when interrupt TokenPipe unexpectedly cannot be read
    std::function<void()> m_failed_token_read_cb;
    //! (optional) callback to be executed when interrupt TokenPipe unexpectedly cannot be written
    std::function<void()> m_failed_token_write_cb;

#ifndef WIN32
    // On UNIX-like operating systems use the self-pipe trick.
    TokenPipeEnd m_pipe_r;
    TokenPipeEnd m_pipe_w;
#else
    // On windows use a condition variable, since we don't have any signals there
    std::mutex m_mutex;
    std::condition_variable m_cv;
#endif
};
} // namespace util

#endif // BITCOIN_UTIL_SIGNALINTERRUPT_H
