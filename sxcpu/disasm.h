#ifndef DISASM_H_
#define DISASM_H_
#include <string>

/**
 * Turns an SX52 opcode into text for a debugger, e.g. 0xC0A -> "mov w, #a".
 * 0xFFF is an empty ROM cell and gives "unused".
 */
std::string disassemble(unsigned short opcode);

#endif /*DISASM_H_*/
