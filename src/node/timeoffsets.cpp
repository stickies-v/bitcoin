// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/interface_ui.h>
#include <node/timeoffsets.h>
#include <sync.h>
#include <util/translation.h>
#include <warnings.h>

#include <algorithm>
#include <chrono>
#include <deque>

using namespace std::chrono_literals;

void TimeOffsets::Add(std::chrono::seconds offset)
{
    LOCK(m_mutex);

    if (m_offsets.size() >= N) {
        m_offsets.pop_front();
    }
    m_offsets.push_back(offset);
}

std::chrono::seconds TimeOffsets::Median() const
{
    LOCK(m_mutex);

    // Only calculate the median if we have 5 or more offsets
    if (m_offsets.size() < 5) return 0s;

    auto sorted_copy = m_offsets;
    std::sort(sorted_copy.begin(), sorted_copy.end());
    return sorted_copy[m_offsets.size() / 2];  // approximate median is good enough, keep it simple
}

bool TimeOffsets::WarnIfOutOfSync() const
{
    // when median == std::numeric_limits<int64_t>::min(), calling std::chrono::abs is UB
    auto median{std::max(Median(), std::chrono::seconds(std::numeric_limits<int64_t>::min() + 1))};
    if (std::chrono::abs(median) <= m_warn_threshold) return false;

    bilingual_str msg{_("Your computer's date and time appear out of sync with the network, "
                        "this may lead to consensus failure. Please ensure it is correct.")};
    LogWarning("%s\n", msg.original);

    if (!m_warning_emitted) {
        m_warning_emitted = true;
        SetMedianTimeOffsetWarning();
        uiInterface.ThreadSafeMessageBox(msg, "", CClientUIInterface::MSG_WARNING);
    }

    return true;
}
