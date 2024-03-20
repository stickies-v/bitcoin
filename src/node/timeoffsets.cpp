// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/interface_ui.h>
#include <node/timeoffsets.h>
#include <sync.h>
#include <util/time.h>
#include <util/translation.h>
#include <warnings.h>

#include <algorithm>
#include <chrono>
#include <deque>

using namespace std::chrono_literals;

void TimeOffsets::AddAndMaybeWarn(std::chrono::seconds offset)
{
    LOCK(m_mutex);

    if (m_offsets.size() >= N) {
        m_offsets.pop_front();
    }
    m_offsets.push_back(offset);
    m_median = CalcMedian();  // cache it so we can access it without mutex and without sorting again

    if (IsOutOfSync()) {
        Warn();
    } else {
        SetMedianTimeOffsetWarning(false);
    }
}

std::chrono::seconds TimeOffsets::CalcMedian() const
{
    AssertLockHeld(m_mutex);

    // Only calculate the median if we have 5 or more offsets
    if (m_offsets.size() < 5) return 0s;

    auto sorted_copy = m_offsets;
    std::sort(sorted_copy.begin(), sorted_copy.end());
    return sorted_copy[m_offsets.size() / 2];  // approximate median is good enough, keep it simple
}

bool TimeOffsets::IsOutOfSync() const
{
    // when median == std::numeric_limits<int64_t>::min(), calling std::chrono::abs is UB
    auto median{std::max(Median(), std::chrono::seconds(std::numeric_limits<int64_t>::min() + 1))};
    return std::chrono::abs(median) <= m_warn_threshold;
}

void TimeOffsets::Warn() const
{
    AssertLockHeld(m_mutex);

    bilingual_str msg{_("Your computer's date and time appear out of sync with the network, "
                        "this may lead to consensus failure. Please ensure it is correct.")};
    LogWarning("%s\n", msg.original);
    SetMedianTimeOffsetWarning(false);

    const auto now{SteadyClock::now()};
    if (!m_warning_emitted || m_warning_emitted.value() - now > m_warning_wait) {
        m_warning_emitted = now;
        uiInterface.ThreadSafeMessageBox(msg, "", CClientUIInterface::MSG_WARNING);
    }
}
