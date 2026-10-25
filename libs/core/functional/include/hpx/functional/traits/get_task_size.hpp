//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>

namespace hpx::threads {

    // The values are defined in hpx/coroutines/thread_enums.hpp, which this
    // module can't include. task_size{} is task_size::unknown.
    HPX_CXX_CORE_EXPORT enum class task_size : std::uint8_t;
}    // namespace hpx::threads

namespace hpx::traits {

    // By default, we don't know how long the function runs, so we report the
    // given Size
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size{}, typename Enable = void>
    struct get_task_size
    {
        static constexpr threads::task_size value = Size;
    };

    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size{}>
    inline constexpr threads::task_size get_task_size_v =
        get_task_size<F, Size>::value;
}    // namespace hpx::traits
