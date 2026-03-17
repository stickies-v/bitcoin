// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <sync.h>
#include <test/util/common.h>
#include <util/pointers.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <set>
#include <type_traits>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(util_pointers_tests)

static_assert(std::is_same_v<util::NotNullUniquePtr<int>, util::NotNull<std::unique_ptr<int>>>);
static_assert(std::is_same_v<util::NotNullSharedPtr<int>, util::NotNull<std::shared_ptr<int>>>);

template <class P>
concept NotNullConstructible = requires { typename std::bool_constant<(util::NotNull{P{}}, true)>; };

BOOST_AUTO_TEST_CASE(check_nullptr)
{
    struct SomePtr {
        constexpr bool operator==(std::nullptr_t) const { return false; }
    };
    struct NullPtr {
        constexpr bool operator==(std::nullptr_t) const { return true; }
    };

    static_assert(NotNullConstructible<SomePtr>);
    static_assert(!NotNullConstructible<NullPtr>);
}

BOOST_AUTO_TEST_CASE(check_derived)
{
    struct MyBase {
    };
    struct MyDerived : public MyBase {
    };

    MyBase base;
    MyDerived derived;
    util::NotNull<MyDerived*> p{&derived};
    util::NotNull<MyBase*> q(&base);
    q = p;
    BOOST_CHECK_EQUAL(q, p);
}

BOOST_AUTO_TEST_CASE(check_swap)
{
    util::NotNull box_a{std::make_unique<int>(1)};
    util::NotNull box_b{std::make_unique<int>(2)};
    static_assert(std::is_nothrow_swappable_v<decltype(box_a)>);
    BOOST_CHECK_EQUAL(*box_a - *box_b, -1);
    swap(box_a, box_b);
    BOOST_CHECK_EQUAL(*box_a - *box_b, 1);
}

BOOST_AUTO_TEST_CASE(check_deref)
{
    int v{2};
    util::NotNull p(&v);
    *p = 3;
    BOOST_CHECK_EQUAL(v, 3);
    *p.get() = 4;
    BOOST_CHECK_EQUAL(v, 4);
    util::NotNull<const int*> c{&v};
    v = 5;
    BOOST_CHECK_EQUAL(*c, 5);
}

BOOST_AUTO_TEST_CASE(check_compare_set)
{
    int a;
    int b;
    std::set<util::NotNull<int*>> uniq{};
    uniq.emplace(&a);
    uniq.emplace(&a);
    uniq.emplace(&b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_CASE(check_hash_set)
{
    int a;
    int b;
    std::unordered_set<util::NotNull<int*>> uniq{};
    uniq.emplace(&a);
    uniq.emplace(&a);
    uniq.emplace(&b);
    BOOST_CHECK_EQUAL(uniq.size(), 2);
}

BOOST_AUTO_TEST_CASE(check_tsa)
{
    struct Thing {
        Mutex mutex{};
        int a GUARDED_BY(mutex){2};
    };
    const util::NotNull p{std::make_shared<Thing>()};
    // Clang thread safety annotation (TSA) may not be able to derive that
    // operator-> and operator* refer to the same object. So use a named
    // reference before taking the lock.
    Thing& ref{*p};
    LOCK(ref.mutex);
    BOOST_CHECK_EQUAL(ref.a, 2);
}

BOOST_AUTO_TEST_SUITE_END()
