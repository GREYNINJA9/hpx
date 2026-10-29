//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \enum task_size
    ///
    /// The expected run time of a task, a hint the scheduler may use. A task
    /// with an unknown size is scheduled as it would be without the hint.
    HPX_CXX_CORE_EXPORT enum class task_size : std::uint8_t {
        unknown = 0,
        small_ = 1,    ///< the underscore is to work around 'small' being
                       ///  defined to char on Windows
        medium = 2,
        large = 3
    };

    /// The number of small tasks the run time of a task of each size stands
    /// for. Each size stands for a hundred times the run time of the one below
    /// it: small up to about 100 ns, medium about 10 us, large about 1 ms or
    /// more.
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        small_tasks_per_small_task = 1;
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        small_tasks_per_medium_task = 100;
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        small_tasks_per_large_task = 10000;

    /// The size of a task that calls a function of the given \p size \p calls
    /// times, for instance a hundred small calls make a medium task and a
    /// hundred medium calls a large one. The size of a task calling a function
    /// of unknown size is unknown.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr task_size bulk_task_size(
        task_size const size, std::size_t const calls) noexcept
    {
        std::size_t small_tasks_per_call = 0;
        switch (size)
        {
        case task_size::small_:
            small_tasks_per_call = small_tasks_per_small_task;
            break;
        case task_size::medium:
            small_tasks_per_call = small_tasks_per_medium_task;
            break;
        case task_size::large:
            return task_size::large;
        default:
            return task_size::unknown;
        }

        // the run time of all calls in small tasks, limited to avoid overflow
        std::size_t const small_tasks =
            calls > small_tasks_per_large_task / small_tasks_per_call ?
            small_tasks_per_large_task :
            calls * small_tasks_per_call;

        if (small_tasks >= small_tasks_per_large_task)
            return task_size::large;
        if (small_tasks >= small_tasks_per_medium_task)
            return task_size::medium;
        return task_size::small_;
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::task_size;
    HPX_CXX_CORE_EXPORT using threads::bulk_task_size;
}    // namespace hpx

namespace hpx::traits {

    // By default, we don't know how long the function runs. Size stays
    // unknown for callers; it is an extension point for specializations,
    // which pass it on to the function they wrap.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown,
        typename Enable = void>
    struct get_task_size
    {
        static constexpr threads::task_size value = Size;
    };

    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown>
    inline constexpr threads::task_size get_task_size_v =
        get_task_size<F, Size>::value;
}    // namespace hpx::traits
