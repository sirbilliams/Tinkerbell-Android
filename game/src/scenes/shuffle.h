// MSVC 6's random_shuffle (0x456430 / 0x457be0 for ints, also inlined): each element from the second on swapped
// with one before it (never itself), drawing from the C runtime's rand(). (cloudburst.cpp keeps its own copy.)
#pragma once
#include "../vision/engine.h"
#include <utility>
#include <vector>

namespace Game {

template <class T> void ms_shuffle(std::vector<T> &v) {
    if (v.empty()) return;
    unsigned dd = 1;
    for (size_t i = 1; i < v.size(); i++, dd++) {
        unsigned rm = 0x7fff, rn = (unsigned)Vision::crt_rand() & 0x7fff;   // 0x4f54da
        while (rm < dd && rm != ~0u) {
            rm = rm << 15 | 0x7fff;
            rn = rn << 15 | 0x7fff;
        }
        std::swap(v[i], v[rn % dd]);
    }
}

}  // namespace Game
