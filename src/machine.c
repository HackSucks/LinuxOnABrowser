//
// Created by hanyuan on 2024/2/8.
//

#include "machine.h"
#include "parameters.h"
#include "mmu.h"
#include "decode.h"
#include "uart8250.h"
#include "trap.h"
#include "zicsr.h"
#include "port/console.h"
#include "perf.h"
#include "port/os_yield_cpu.h"

//#define RISCV_ISA_TESTS

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/*
 * How many instructions to run per scheduling tick.
 *
 * emscripten_set_main_loop with fps=0 uses requestAnimationFrame (~60/s).
 * That gives 60 * STEPS_PER_FRAME instructions/second maximum.
 * Kernel decompression needs ~billions of instructions, so we need
 * STEPS_PER_FRAME large enough to finish in reasonable wall time.
 *
 * We use emscripten_async_call with setTimeout(0) instead of rAF so the
 * browser doesn't cap us at 60fps — setTimeout(0) fires as fast as the
 * JS event loop allows (~250-1000x/sec), giving much higher throughput
 * during the CPU-heavy boot phase while still yielding between batches.
 */
#define STEPS_PER_BATCH 500000

static int g_printreg __attribute__((unused));
static void machine_schedule_next(void);

static void machine_batch(void *arg) {
    for (int i = 0; i < STEPS_PER_BATCH; i++) {
        /* WFI fast-path: just tick until an interrupt fires */
        if (in_wfi) {
            zicnt_cycle_tick();
            if (zicnt_get_cycle() % ZICNT_TICK_INTERVAL == 0) zicnt_time_tick();
            if (zicnt_get_cycle() % 16 == 0) uart8250_tick();
            trap_take_interrupt();
            continue;
        }

        uint8_t access_error_intr = 0;
        uint32_t instruction = mmu_read_inst(program_counter, &access_error_intr);
        if (unlikely(access_error_intr)) {
            if (access_error_intr == 2)
                trap_throw_exception(EXCEPTION_INST_PAGEFAULT, program_counter);
            else if (access_error_intr == 3)
                trap_throw_exception(EXCEPTION_INST_ADDR_MISALIGNED, program_counter);
            else
                trap_throw_exception(EXCEPTION_INST_ACCESS_FAULT, program_counter);
        } else {
            decode(instruction);
        }

        zicnt_cycle_tick();
        if (zicnt_get_cycle() % ZICNT_TICK_INTERVAL == 0) zicnt_time_tick();
        if (zicnt_get_cycle() % 16 == 0) uart8250_tick();
        trap_take_interrupt();
    }

    machine_schedule_next();
}

static void machine_schedule_next(void) {
    /* setTimeout(0) — yield to browser event loop then resume immediately.
     * Much faster than rAF (not capped at 60fps) while still non-blocking. */
    emscripten_async_call(machine_batch, NULL, 0);
}

#endif /* __EMSCRIPTEN__ */

static void machine_pre_boot(uint32_t start);
#ifndef __EMSCRIPTEN__
static void machine_tick(void);
static void machine_debug(uint32_t instruction, int printreg);
#endif

void machine_start(uint32_t start, int printreg) {
    machine_pre_boot(start);

#ifdef __EMSCRIPTEN__
    g_printreg = printreg;
    machine_schedule_next();
#else
    for (;;) {
        uint8_t access_error_intr = 0;
        uint32_t instruction = mmu_read_inst(program_counter, &access_error_intr);
        if (unlikely(access_error_intr)) {
            if (access_error_intr == 2)
                trap_throw_exception(EXCEPTION_INST_PAGEFAULT, program_counter);
            else if (access_error_intr == 3)
                trap_throw_exception(EXCEPTION_INST_ADDR_MISALIGNED, program_counter);
            else
                trap_throw_exception(EXCEPTION_INST_ACCESS_FAULT, program_counter);
        } else {
            machine_debug(instruction, printreg);
            decode(instruction);
        }
        machine_tick();
    }
#endif
}

static void machine_pre_boot(uint32_t start) {
    program_counter = start;
    zicnt_init();
    port_os_console_init();
    port_os_yield_cpu_add_interrupt(zicnt_time_tick);
    port_os_yield_cpu_add_interrupt(uart8250_tick);
}

#ifndef __EMSCRIPTEN__
static void machine_tick(void) {
    zicnt_cycle_tick();
    if (zicnt_get_cycle() % ZICNT_TICK_INTERVAL == 0) zicnt_time_tick();
    if (zicnt_get_cycle() % 16 == 0) uart8250_tick();
    trap_take_interrupt();
}

static void machine_debug(uint32_t instruction, int printreg) {
#if TEMU_DEBUG_CODE
    if (printreg) {
        mmu_debug_printreg(program_counter);
    }
#ifdef RISCV_ISA_TESTS
    if (program_counter == 0x2003008) { int a = 0; }
    if (instruction == 0x00000073)    { int a = 0; }
#endif
#endif
}
#endif