//
// Created by hanyuan on 2024/4/23.
//
#include "port/console.h"
#include "uart8250.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <stdio.h>

/* Input ring buffer fed by JS via uart8250_wasm_push_char() */
#define WASM_INPUT_BUF_SIZE 256
static uint8_t  wasm_input_buf[WASM_INPUT_BUF_SIZE];
static uint16_t wasm_input_head = 0;
static uint16_t wasm_input_tail = 0;

/* Called from JS: Module.ccall('uart8250_wasm_push_char', ...) */
EMSCRIPTEN_KEEPALIVE
void uart8250_wasm_push_char(uint8_t c) {
    uint16_t next = (wasm_input_tail + 1) % WASM_INPUT_BUF_SIZE;
    if (next != wasm_input_head) {          /* not full */
        wasm_input_buf[wasm_input_tail] = c;
        wasm_input_tail = next;
    }
}

void port_os_console_init(void) {
    uart8250_init();
}

void port_console_write(uint8_t c) {
    /* Write one char at a time through Emscripten's stdout so it
     * reaches Module.print line-by-line.  putchar buffers until \n. */
    putchar(c);
    if (c == '\n') fflush(stdout);
}

void port_console_flush(void) {
    fflush(stdout);
}

/* Non-blocking: return next char from ring buffer, or -1 if empty. */
int port_console_read(void) {
    if (wasm_input_head == wasm_input_tail) return -1;
    uint8_t c = wasm_input_buf[wasm_input_head];
    wasm_input_head = (wasm_input_head + 1) % WASM_INPUT_BUF_SIZE;
    return (int)c;
}

#elif !defined(WIN32) && !defined(WIN64) && !defined(BARE_METAL_PLATFORM)

#include <termios.h>
#include <unistd.h>
#include <stdio.h>

void port_os_console_init(void) {
    static struct termios tm;
    tcgetattr(STDIN_FILENO, &tm);
    cfmakeraw(&tm);
    tm.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &tm);
    uart8250_init();
}

void port_console_write(uint8_t c) { printf("%c", c); }
void port_console_flush(void)      { fflush(stdout); }
int  port_console_read(void)       { return getchar(); }

#elif (defined(WIN32) || defined(WIN64)) && !defined(BARE_METAL_PLATFORM)

#include <windows.h>
#include <stdio.h>

void port_os_console_init(void) {
    HANDLE hStdin;
    DWORD fdwSaveOldMode, fdwMode;
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwSaveOldMode);
    fdwMode = ENABLE_PROCESSED_INPUT | ENABLE_INSERT_MODE |
              ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdin, fdwMode);
    uart8250_init();
}

void port_console_write(uint8_t c) { printf("%c", c); }
void port_console_flush(void)      { fflush(stdout); }
int  port_console_read(void) {
#error port_console_read not implemented for Windows
}

#else
#warning Define function port_console_write outside of TEMU!
#warning Define function port_console_read outside of TEMU!
#endif