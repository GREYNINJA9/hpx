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
#include <cstddef>
#include <memory>
#include <vector>

// tidy() runs from pre-shutdown and notifies every activity observer of each
// target it removes. An observer registered on another locality has its agent
// here (create_activity_agent() uses local_new). Without a tracked target,
// tidy() does not notify anyone, so publish a target on that locality and
// leave the observer registered. Returning from hpx_main() is the check: a
// hang in the remote wait becomes a test timeout. The count is shared because
// pre-shutdown can invoke the callback after this function has returned.
void test_shutdown_with_remote_activity_observer()
{
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();
    HPX_TEST(localities.size() >= 2);
    if (localities.size() < 2)
    {
        return;
    }

    hpx::id_type const here = hpx::find_here();
    hpx::id_type other = hpx::invalid_id;
    for (auto const& locality : localities)
    {
        if (locality != here)
        {
            other = locality;
            break;
        }
    }
    HPX_TEST_NEQ(other, hpx::invalid_id);
    if (other == hpx::invalid_id)
    {
        return;
    }

    // true keeps the observer registered. Returning false would drop it
    // during the publish below, and shutdown would no longer notify it.
    auto received = std::make_shared<std::atomic<std::size_t>>(0);
    auto const handle =
        hpx::supervision::register_activity_observer(hpx::launch::sync, other,
            [received](hpx::supervision::activity_notification const&) {
                received->fetch_add(1, std::memory_order_acq_rel);
                return true;
            });
    HPX_TEST_NEQ(handle, hpx::invalid_id);

    hpx::id_type const target = make_test_target();
    hpx::supervision::publish_result const published =
        hpx::supervision::publish_event(
            hpx::launch::sync, other, target, hpx::supervision::event::started);
    HPX_TEST(published == hpx::supervision::publish_result::applied);

    // The first publication waits for this observer, so a zero here means
    // the remote registration never ran.
    HPX_TEST_NEQ(received->load(std::memory_order_acquire), std::size_t(0));
}

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(test_shutdown_with_remote_activity_observer);

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
