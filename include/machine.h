//
// Created by hanyuan on 2024/2/8.
//

#ifndef TEMU_MACHINE_H
#define TEMU_MACHINE_H

#include <stdint.h>

/* On WASM, machine_start() registers an rAF loop and returns immediately.
 * _Noreturn would be a lie and causes UB / compiler warnings. */
#ifdef __EMSCRIPTEN__
void machine_start(uint32_t start, int printreg);
#else
_Noreturn void machine_start(uint32_t start, int printreg);
#endif

#endif //TEMU_MACHINE_H