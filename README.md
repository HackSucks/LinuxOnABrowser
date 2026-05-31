# LinuxOnABrowser
a port of the temu RISC-V educational emulator to WebAssembly.

# whah?
TEMU is an emulator created for educational purposes that emulates a RISC-V CPU and is purpose built for running Linux.

on the other hand, WebAssembly is a recent enough feature of modern browsers that allows for files compiled from C/C++ to WebAssembly, using a compiler like Emscripten to run in a sandboxed enviroment.

I originally planned to port QEMU to WebAssembly, but due to issues i have had to abandon it.

I chose TEMU as an initial emulator because it is very simple small and lightweight. I may switch over to another, more full emulator (like for networking support or even X.Org display from framebuffer to HTML5 <canvas> because this one only has UART text output.) in the future.

# How do I do it :3 
A ready to run example is the GitHub page linked in the project description.
