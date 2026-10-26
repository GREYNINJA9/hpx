//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>

namespace {

    // Shared with the activity callbacks, which may outlive the test body:
    // tidy() abandons the callback's future once teardown has started.
    struct blocked_callback_state
    {
        std::atomic<std::size_t> entered{0};
        std::atomic<std::size_t> finished{0};
        std::atomic<bool> release{false};
    };

    // Blocks the worker running the activity callback until the test releases
    // it. The upper bound keeps a regressed unbounded wait in tidy() from
    // hanging the whole test executable; the test asserts that tidy() returns
    // long before the bound expires.
    void block_until_released(blocked_callback_state& state)
    {
        state.entered.fetch_add(1, std::memory_order_acq_rel);

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!state.release.load(std::memory_order_acquire) &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        state.finished.fetch_add(1, std::memory_order_acq_rel);
    }
}    // namespace

// Regression test: an activity notification admitted just before teardown
// starts can block in its callback while tidy() is already running. tidy()
// must not wait for that callback indefinitely; it should notice the teardown
// state and drop the agent instead. The agent is local so the test does not
// need a second locality to leave; the sliced wait is the same for local and
// remote agents.
void test_tidy_gives_up_on_blocked_activity_agent()
{
    hpx::id_type const locality = hpx::find_here();

    auto state = std::make_shared<blocked_callback_state>();

    auto const handle =
        hpx::supervision::register_activity_observer(hpx::launch::sync,
            locality, [state](hpx::supervision::activity_notification const&) {
                block_until_released(*state);
                return true;
            });
    HPX_TEST_NEQ(handle, hpx::invalid_id);

    hpx::id_type const target = make_test_target();

    // Publish on another HPX thread: the notification is admitted while the
    // runtime is still running and then blocks in the activity callback.
    auto publish = hpx::async([target]() {
        hpx::supervision::publish_event(
            target, hpx::supervision::event::started);
    });

    while (state->entered.load(std::memory_order_acquire) == 0)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto const start = std::chrono::steady_clock::now();

    hpx::error_code ec;
    hpx::supervision::get_supervision_manager().tidy(ec);

    auto const elapsed = std::chrono::steady_clock::now() - start;
    HPX_TEST(!ec);

    // Release every blocked callback and let it finish before the runtime
    // shuts down.
    std::size_t const entered_at_release =
        state->entered.load(std::memory_order_acquire);
    state->release.store(true, std::memory_order_release);
    while (state->finished.load(std::memory_order_acquire) < entered_at_release)
    {
        hpx::this_thread::yield();
    }
    publish.get();

    // The wait polls in 100ms slices, so tidy() must return long before the
    // callback's own five-second bound.
    HPX_TEST(elapsed < std::chrono::seconds(2));
}

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(test_tidy_gives_up_on_blocked_activity_agent);

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
