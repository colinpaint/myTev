// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.

#pragma once

#include <tev/Common.h>

#include <deque>
#include <mutex>
#include <condition_variable>

TEV_NAMESPACE_BEGIN

namespace colormap {
    const std::vector<float>& turbo();
    const std::vector<float>& viridis();
}

TEV_NAMESPACE_END
