# The SX52 core without any front end: no SDL, wxWidgets or curses.
add_library(sxcpu STATIC cpu.cpp cpu.h cpu_ports.h disasm.cpp disasm.h)
target_include_directories(sxcpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
