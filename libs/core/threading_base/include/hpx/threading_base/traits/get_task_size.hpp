//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>

namespace hpx::traits {

    // By default, we don't know how long the function runs, so we report the
    // given Size
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

    ///////////////////////////////////////////////////////////////////////////
    // hpx::async binds the arguments with deferred_call, which runs as long
    // as the function it calls.
    template <threads::task_size Size, typename F, typename... Ts>
    struct get_task_size<util::detail::deferred<F, Ts...>, Size>
      : get_task_size<F, Size>
    {
    };
}    // namespace hpx::traits
