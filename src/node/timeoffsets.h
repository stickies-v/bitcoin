// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TIMEOFFSETS_H
#define BITCOIN_NODE_TIMEOFFSETS_H

#include <sync.h>
#include <util/time.h>

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

class PeerClock
{
private:
    std::chrono::steady_clock::time_point m_steady_connect_time;
    std::chrono::system_clock::time_point m_system_connect_time;
public:
    PeerClock(const std::chrono::system_clock::time_point& peer_time = std::chrono::system_clock::now());
    std::chrono::system_clock::time_point now() const {
        const auto time_passed{std::chrono::duration_cast<std::chrono::seconds>(SteadyClock::now() - m_steady_connect_time)};
        return m_system_connect_time + time_passed;
    }
    /** Positive adjustment means the system clock has shifted backwards, relative to the peer's
     * reported time when we handshaked. In other words, the offset has increased.
    */
    std::chrono::seconds GetOffset() const {
        std::system_clock::time_point peer_time{now()};
        NodeClock::time_point local_time{NodeClock::now()};
    
        return peer_time - local_time;
    }
    //! Add templated conversion method to `std::chrono::...`
};

class TimeOffsets
{
private:
    static constexpr size_t N{50};
    //! Minimum difference between system and network time for a warning to be raised.
    static constexpr std::chrono::minutes m_warn_threshold{10};
    //! Log the last time a GUI warning was raised to avoid flooding user with them
    mutable std::optional<std::chrono::steady_clock::time_point> m_warning_emitted GUARDED_BY(m_mutex){};
    //! Minimum time to wait before raising a new GUI warning
    static constexpr std::chrono::minutes m_warning_wait{60};

    mutable Mutex m_mutex;
    std::deque<PeerClock> m_offsets GUARDED_BY(m_mutex){};

    std::vector<std::chrono::seconds> GetSortedAdjustedOffsets() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

public:
    /** Add a new time offset sample. */
    void Add(PeerClock peer_clock) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Compute and return the median of the collected time offset samples. */
    std::chrono::seconds Median() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Raise warnings if the median time offset exceeds the warnings threshold. Returns true if
     * warnings were raised.
    */
    bool WarnIfOutOfSync() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_NODE_TIMEOFFSETS_H
