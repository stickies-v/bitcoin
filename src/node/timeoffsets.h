// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TIMEOFFSETS_H
#define BITCOIN_NODE_TIMEOFFSETS_H

#include <sync.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <optional>
#include <stddef.h>

class TimeOffsets
{
private:
    mutable Mutex m_mutex;
    static constexpr size_t N{50};
    //! Minimum difference between system and network time for a warning to be raised.
    static constexpr std::chrono::minutes m_warn_threshold{10};
    //! Log the last time a warning was raised to avoid flooding user with warnings
    mutable std::optional<std::chrono::steady_clock::time_point> m_warning_emitted GUARDED_BY(m_mutex){};
    //! Minimum time to wait before raising a new GUI warning
    static constexpr std::chrono::minutes m_warning_wait{60};

    std::deque<std::chrono::seconds> m_offsets GUARDED_BY(m_mutex){};

    //! Cached value of the median of the m_offsets
    std::atomic<std::chrono::seconds> m_median;
    std::chrono::seconds CalcMedian() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    /**
     * Unconditionally warn to log and set g_timeoffset_warning to true.
     * Conditionally raise a GUI MessageBox if the last warning has been raised longer than
     * m_warning_wait ago.
     */
    void Warn() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

public:
    /**
     * Add a new time offset and raise warnings if the median offset exceeds the threshold.
     * 
     * @see Warn() for the warnings that are raised.
     */
    void AddAndMaybeWarn(std::chrono::seconds offset) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Returns true if the median offset exceeds m_warn_threshold. */
    bool IsOutOfSync() const;

    /** Return the median of the collected time offset samples. */
    std::chrono::seconds Median() const { return m_median; }
};

#endif // BITCOIN_NODE_TIMEOFFSETS_H
