// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/timeoffsets.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <chrono>


FUZZ_TARGET(timeoffsets)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TimeOffsets offsets{};
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 4'000) {
        (void)offsets.Median();
        offsets.AddAndMaybeWarn(std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()});
        offsets.IsOutOfSync();
    }
}
