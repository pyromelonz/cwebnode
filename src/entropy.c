#include "entropy.h"

#include <time.h>
#include <stdlib.h>

void seed() {
    srandom(time(0));
}

uint32_t random_u32_b() {
    return random();
}

uint32_t random_u32_h() {
    seed();
    random_u32 = random_u32_b;
    return random_u32();
}

uint32_t (*random_u32)() = random_u32_h;