// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/tools/old/interface.hpp>
#include <util/log.h>

#include <chrono>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace util::log;

BOOST_AUTO_TEST_SUITE(util_log_tests)

BOOST_AUTO_TEST_CASE(logger_enabled)
{
    Logger logger;

    // No callbacks registered - should not be enabled
    BOOST_CHECK(!logger.Enabled());

    // Register a callback - should now be enabled
    auto handle = logger.RegisterCallback([](const Entry&) {});
    BOOST_CHECK(logger.Enabled());

    // Unregister the callback - should no longer be enabled
    logger.UnregisterCallback(handle);
    BOOST_CHECK(!logger.Enabled());
}

BOOST_AUTO_TEST_CASE(logger_level_filtering)
{
    Logger logger;
    int callback_count = 0;
    auto handle = logger.RegisterCallback([&](const Entry&) { ++callback_count; });

    // Default min level is Debug, so Trace is skipped
    BOOST_CHECK_EQUAL(static_cast<int>(logger.GetMinLevel()), static_cast<int>(Level::Debug));
    logger.Log(Level::Trace, 0, std::source_location::current(), false, "skipped");
    logger.Log(Level::Debug, 0, std::source_location::current(), false, "logged");
    BOOST_CHECK_EQUAL(callback_count, 1);

    // Raise min level to Warning
    callback_count = 0;
    logger.SetMinLevel(Level::Warning);
    logger.Log(Level::Info, 0, std::source_location::current(), false, "skipped");
    logger.Log(Level::Warning, 0, std::source_location::current(), false, "logged");
    BOOST_CHECK_EQUAL(callback_count, 1);

    logger.UnregisterCallback(handle);
}

BOOST_AUTO_TEST_CASE(logger_callback_receives_entry)
{
    Logger logger;

    std::vector<Entry> received_entries;
    auto handle = logger.RegisterCallback([&](const Entry& entry) {
        received_entries.push_back(entry);
    });

    const uint64_t test_category = 42;
    auto before = std::chrono::system_clock::now();
    logger.Log(Level::Info, test_category, std::source_location::current(),
               /*should_ratelimit=*/true, "test message %d", 123);
    auto after = std::chrono::system_clock::now();

    BOOST_REQUIRE_EQUAL(received_entries.size(), 1);
    const auto& entry = received_entries[0];

    BOOST_CHECK_EQUAL(static_cast<int>(entry.level), static_cast<int>(Level::Info));
    BOOST_CHECK_EQUAL(entry.category, test_category);
    BOOST_CHECK_EQUAL(entry.message, "test message 123");
    BOOST_CHECK(entry.timestamp >= before && entry.timestamp <= after);
    BOOST_CHECK(entry.should_ratelimit);

    logger.UnregisterCallback(handle);
}

BOOST_AUTO_TEST_CASE(logger_multiple_callbacks)
{
    Logger logger;

    int callback1_count = 0;
    int callback2_count = 0;

    BOOST_CHECK(!logger.Enabled());
    auto handle1 = logger.RegisterCallback([&](const Entry&) {
        ++callback1_count;
    });
    auto handle2 = logger.RegisterCallback([&](const Entry&) {
        ++callback2_count;
    });
    BOOST_CHECK(logger.Enabled());

    logger.Log(Level::Info, 0, std::source_location::current(),
               /*should_ratelimit=*/false, "test");

    // Both callbacks should have been called
    BOOST_CHECK_EQUAL(callback1_count, 1);
    BOOST_CHECK_EQUAL(callback2_count, 1);

    // Unregister first callback
    logger.UnregisterCallback(handle1);

    logger.Log(Level::Info, 0, std::source_location::current(),
               /*should_ratelimit=*/false, "test");

    // Only second callback should have been called
    BOOST_CHECK_EQUAL(callback1_count, 1);
    BOOST_CHECK_EQUAL(callback2_count, 2);

    logger.UnregisterCallback(handle2);
    BOOST_CHECK(!logger.Enabled());
}

BOOST_AUTO_TEST_CASE(logger_skips_when_not_enabled)
{
    Logger logger;

    // No callbacks registered, Log should be a no-op (and not crash)
    logger.Log(Level::Info, 0, std::source_location::current(),
               /*should_ratelimit=*/false, "this should not crash");
}

BOOST_AUTO_TEST_CASE(global_logger_singleton)
{
    // GetLogger should return the same instance
    Logger& logger1 = GetLogger();
    Logger& logger2 = GetLogger();
    BOOST_CHECK_EQUAL(&logger1, &logger2);
}

BOOST_AUTO_TEST_SUITE_END()
