// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

// Memory management placeholder
#include <cstdlib>

namespace ibex::runtime {

void* ibex_alloc(size_t size) {
    return ::malloc(size);
}

void ibex_free(void* ptr) {
    ::free(ptr);
}

} // namespace ibex::runtime
