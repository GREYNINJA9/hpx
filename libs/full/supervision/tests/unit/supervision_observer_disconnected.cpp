//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An observer whose locality went away must not fence the target it observes:
// the failed delivery is dropped together with the observer, and the target's
// own state is left alone (#7834). A remote observer's locality going away
// surfaces as a delivery failing with hpx::error::locality_was_disconnected;
// a local callback throwing that error reaches the very same code path, which
// keeps these tests on a single locality and deterministic.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <atomic>
#include <cstddef>
#include <memory>

namespace {

    // Callback behaving like an observer on a locality that has gone away.
    // Counts its invocations, so the tests can tell it was unregistered.
    hpx::supervision::lifecycle_callback make_disconnected_observer(
        std::shared_ptr<std::atomic<std::size_t>> calls)
    {
        return
            [calls = HPX_MOVE(calls)](
                hpx::supervision::lifecycle_event_notification const&) -> bool {
                calls->fetch_add(1, std::memory_order_relaxed);
                HPX_THROW_EXCEPTION(hpx::error::locality_was_disconnected,
                    "make_disconnected_observer",
                    "observer locality went away");
            };
    }
}    // namespace

// A live event that cannot be delivered because the observer's locality went
// away leaves the target admitted, and the observer is not invoked again.
void test_disconnected_observer_does_not_fail_target()
{
    hpx::id_type const target = make_test_target();
    auto const calls = std::make_shared<std::atomic<std::size_t>>(0);

    hpx::supervision::register_observer(
        target, make_disconnected_observer(calls));

    auto result = hpx::supervision::publish_event(
        target, hpx::supervision::event::started);
    HPX_TEST(result == hpx::supervision::publish_result::applied);
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    // A target recorded as failed would reject this transition.
    result = hpx::supervision::publish_event(
        target, hpx::supervision::event::running);
    HPX_TEST(result == hpx::supervision::publish_result::applied);

    // The observer was dropped after its failed delivery.
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    auto const state = hpx::supervision::query_state(target);
    HPX_TEST(state.last_event == hpx::supervision::event::running);
    HPX_TEST(!state.ec);

    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::admitted);
}

// The failed delivery does not keep the target's remaining observers from
// being notified.
void test_disconnected_observer_does_not_stop_other_observers()
{
    hpx::id_type const target = make_test_target();
    auto const calls = std::make_shared<std::atomic<std::size_t>>(0);
    std::atomic<std::size_t> healthy_calls{0};

    // registered first, so it is notified first
    hpx::supervision::register_observer(
        target, make_disconnected_observer(calls));

    auto const healthy_handle = hpx::supervision::register_observer(target,
        [&healthy_calls](
            hpx::supervision::lifecycle_event_notification const&) {
            healthy_calls.fetch_add(1, std::memory_order_relaxed);
            return true;
        });

    reach_running(hpx::find_here(), target);

    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(healthy_calls.load(), static_cast<std::size_t>(2));

    auto const state = hpx::supervision::query_state(target);
    HPX_TEST(state.last_event == hpx::supervision::event::running);
    HPX_TEST(!state.ec);

    hpx::supervision::unregister_observer(healthy_handle);
}

// The same holds for the notification replayed to an observer when it is
// registered for a target that already has state.
void test_disconnected_observer_replay_does_not_fail_target()
{
    hpx::id_type const target = make_test_target();
    auto const calls = std::make_shared<std::atomic<std::size_t>>(0);

    reach_running(hpx::find_here(), target);

    hpx::supervision::register_observer(
        target, make_disconnected_observer(calls));
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    auto const state = hpx::supervision::query_state(target);
    HPX_TEST(state.last_event == hpx::supervision::event::running);
    HPX_TEST(!state.ec);

    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::admitted);

    // the target reaches its own terminal state, not a recorded failure
    auto const result = hpx::supervision::publish_event(
        target, hpx::supervision::event::completed);
    HPX_TEST(result == hpx::supervision::publish_result::applied);
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    HPX_TEST(hpx::supervision::query_state(target).last_event ==
        hpx::supervision::event::completed);
}

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(test_disconnected_observer_does_not_fail_target);
    HPX_SUPERVISION_TEST_RUN(
        test_disconnected_observer_does_not_stop_other_observers);
    HPX_SUPERVISION_TEST_RUN(
        test_disconnected_observer_replay_does_not_fail_target);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
