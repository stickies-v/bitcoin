// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <util/task_runner.h>
#include <validationinterface.h>

#include <memory>

struct TestSubscriber final : public CValidationInterface {
    void BlockChecked(const CBlock& block, const BlockValidationState&) override
    {
        assert(!block.GetHash().IsNull());
    }
};

std::shared_ptr<const CBlock> CreateBlock()
{
    DataStream stream{benchmark::data::block413567};
    auto block{std::make_shared<CBlock>()};
    stream >> TX_WITH_WITNESS(*block);
    return block;
}

template <size_t Subs>
static void BlockChecked(benchmark::Bench& bench)
{
    ValidationSignals signals{std::make_unique<util::ImmediateTaskRunner>()};
    for (size_t i{0}; i < Subs; ++i) {
        signals.RegisterSharedValidationInterface(std::make_shared<TestSubscriber>());
    }
    auto block{CreateBlock()};
    bench.run([&] {
        signals.BlockChecked(*block, {});
    });
}

static void BlockCheckedOne(benchmark::Bench& bench) { BlockChecked<1>(bench); };
static void BlockCheckedTwo(benchmark::Bench& bench) { BlockChecked<2>(bench); };
static void BlockCheckedTen(benchmark::Bench& bench) { BlockChecked<10>(bench); };


BENCHMARK(BlockCheckedOne, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockCheckedTwo, benchmark::PriorityLevel::HIGH);
BENCHMARK(BlockCheckedTen, benchmark::PriorityLevel::HIGH);
