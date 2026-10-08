// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Benchmarks the kernel logging hot path: a log statement inside the library up to and including
// the user's logging callback.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <bench/nanobench.h>

#include <kernel/bitcoinkernel_wrapper.h>
#include <util/log.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace btck;

namespace {

uint64_t g_sink{0};

//! Callback that ignores the entry: measures delivery cost only.
class NoopLog
{
public:
    void LogMessage(const LogEntry&) {}
};

//! Callback that reads the message only.
class MessageOnlyLog
{
public:
    void LogMessage(const LogEntry& entry) { g_sink += entry.Message().size(); }
};

//! Callback that reads every field.
class AllFieldsLog
{
public:
    void LogMessage(const LogEntry& entry)
    {
        g_sink += entry.Message().size();
        g_sink += entry.ThreadName().size();
        g_sink += static_cast<uint64_t>(entry.Timestamp().time_since_epoch().count());
        g_sink += entry.MockTime().has_value();
        g_sink += entry.FileName().size();
        g_sink += entry.FunctionName().size();
        g_sink += entry.Line();
        g_sink += static_cast<uint64_t>(entry.Level());
        g_sink += static_cast<uint64_t>(entry.Category());
    }
};

template <typename Log>
void RunWithLogger(ankerl::nanobench::Bench& bench, const std::string& name)
{
    Logger<Log> logger{std::make_unique<Log>()};
    bench.run(name + ", LogInfo fixed", [&] {
        LogInfo("benchmark log entry");
    });
    int i{0};
    bench.run(name + ", LogInfo formatted", [&] {
        LogInfo("benchmark log entry %d %s", i++, "arg");
    });
    bench.run(name + ", LogTrace fixed", [&] {
        LogTrace(BCLog::KERNEL, "benchmark log entry");
    });
}

} // namespace

int main()
{
    ankerl::nanobench::Bench bench;
    bench.unit("entry").warmup(1000).minEpochIterations(20000);

    logging_set_min_level(LogLevel::TRACE_LEVEL);

    bench.run("no connection, LogInfo fixed", [&] {
        LogInfo("benchmark log entry");
    });
    bench.run("no connection, LogTrace fixed", [&] {
        LogTrace(BCLog::KERNEL, "benchmark log entry");
    });

    RunWithLogger<NoopLog>(bench, "noop callback");
    RunWithLogger<MessageOnlyLog>(bench, "message only");
    RunWithLogger<AllFieldsLog>(bench, "all fields");

    ankerl::nanobench::doNotOptimizeAway(g_sink);
    return 0;
}
