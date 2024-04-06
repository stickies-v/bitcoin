// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/interface_ui.h>
#include <node/timeoffsets.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/time.h>
#include <util/translation.h>
#include <warnings.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>

using namespace std::chrono_literals;

PeerClock::PeerClock(const std::chrono::system_clock::time_point& peer_time) : 
    m_steady_connect_time{SteadyClock::now()},
    m_system_connect_time{peer_time} {}

void TimeOffsets::Add(PeerClock peer_clock)
{
    LOCK(m_mutex);

    if (m_offsets.size() >= N) {
        m_offsets.pop_front();
    }
    m_offsets.push_back(peer_clock);
    LogDebug(BCLog::NET, "Added time offset %+ds, total samples %d\n",
             Ticks<std::chrono::seconds>(peer_clock.GetOffset()), m_offsets.size());
}

std::vector<std::chrono::seconds> TimeOffsets::GetSortedAdjustedOffsets() const
{
    AssertLockHeld(m_mutex);

    std::vector<std::chrono::seconds> adj_offsets;
    adj_offsets.reserve(m_offsets.size());

    for (auto sample : m_offsets) {
        adj_offsets.emplace_back(sample.GetOffset());
    }

    std::sort(adj_offsets.begin(), adj_offsets.end());
    return adj_offsets;
}

std::chrono::seconds TimeOffsets::Median() const
{
    LOCK(m_mutex);

    // Only calculate the median if we have 5 or more offsets
    if (m_offsets.size() < 5) return 0s;

    const auto sorted_adj_offsets{GetSortedAdjustedOffsets()};
    return sorted_adj_offsets[sorted_adj_offsets.size() / 2];  // approximate median is good enough, keep it simple
}

bool TimeOffsets::WarnIfOutOfSync() const
{
    // when median == std::numeric_limits<int64_t>::min(), calling std::chrono::abs is UB
    auto median{std::max(Median(), std::chrono::seconds(std::numeric_limits<int64_t>::min() + 1))};
    if (std::chrono::abs(median) <= m_warn_threshold) {
        SetMedianTimeOffsetWarning(std::nullopt);
        return false;
    }

    bilingual_str msg{strprintf(_(
        "Your computer's date and time appears to be more than %d minutes out of sync with the network, "
        "this may lead to consensus failure. After you've corrected your settings, this message "
        "should no longer appear when you restart your node. Without a restart, it should stop showing "
        "automatically after you've connected to a sufficient number of new outbound peers, which may "
        "take some time."
    ), Ticks<std::chrono::minutes>(m_warn_threshold))};
    LogWarning("%s\n", msg.original);
    SetMedianTimeOffsetWarning(msg);

    const auto now{SteadyClock::now()};
    {
        LOCK(m_mutex);
        if (!m_warning_emitted || now - m_warning_emitted.value() > m_warning_wait) {
            m_warning_emitted = now;
            uiInterface.ThreadSafeMessageBox(msg, "", CClientUIInterface::MSG_WARNING);
    }
    }

    return true;
}
