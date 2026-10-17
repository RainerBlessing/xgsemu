// Frozen copy of Cpu from before the refactoring of Cpu::execute. It is the
// reference for the differential tests and is removed when the refactoring is done.
/*
* =====================================================================================
* 
*        Filename:  cpu.h
* 
*     Description:  
* 
*         Version:  1.0
*         Created:  13.07.2005 23:17:20 CEST
*        Revision:  none
*        Compiler:  gcc
* 
*          Author:  Rainer Blessing 
*           Email:  rainer@theblessing.net
* 
* =====================================================================================
*/
#ifndef CPU_REFERENCE_H
#define CPU_REFERENCE_H
#define USHORT unsigned short
#include <string>
#include "../src/Video.h"
#include "../src/Bu8763.h"

using std::string;

/**
 * This is the most important class, it emulates the SX52.
 * @author Rainer Blessing <rainer@theblessing.net>
 */
class CpuReference
{
public:
	CpuReference(Video* video);
	/**
	 * Returns a pointer to ROM. It is used in the debugger.
	 * @return 
	 */
	USHORT* getROM();
	/**
	 * Returns a pointer to RAM. It is used in the debugger.
	 * @return 
	 */
	unsigned char* getRAM();
	/**
	 * Returns a pointer to the external SRAM (128 KB). It is used in tests.
	 * @return
	 */
	unsigned char* getSRAM();
	/**
	 * Returns a pointer to the program register. It is used in the debugger.
	 * The program counter is not in RAM because it is a 12 bit register. Or 
	 * at least used that way.
	 * @return program counter
	 */
	USHORT* getPC();
	/**
	 *  This function executes the opcode. Everytime the function is called
	               *  several commands are executed and the function returns, so that
	               *  the GUI controls are not blocked.
	 */
	void execute();

	/**
	 * Clears RAM, ROM and sets pc to 0xfff
	 */
	void reset();

	/**
	 * The joystick input is handled in the sdlpanel class.
	               * The values are written to that "register"
	 * @return 
	 */
	Uint16* getJoystrickr()
	{
		return &joystickr;
	};
	/**
	 * Executes instructions until the given number of clock cycles has passed.
	 * The last instruction may run over the requested number, the surplus is
	 * deducted from the next call so that the program runs at the right speed
	 * on average. Use it to run one video frame (80 MHz / 60 = 1333333 cycles).
	 */
	void runCycles(unsigned int count);
	/**
	 * Number of clock cycles executed so far.
	 */
	int getCycle() const
	{
		return (int)cycle;
	};
	/**
	 * Connects the sound chip. Changes of port RA are forwarded to it.
	 * @param sound The chip or NULL to disable the sound
	 */
	void setSound(Bu8763* sound);
private:
	/**
	 *  The SRAM emulation.
	 */
	void sram();

	/**
	 * The joystick emulation
	 */
	void joystick();
	unsigned char RAM[262];
	USHORT ROM[4096];
	unsigned char SRAM[131072];
	unsigned char* indf; // indirection register (0)
	unsigned char* rtcc; // timer
	USHORT pc;
	unsigned char* status;
	unsigned char* fsr;
	unsigned char* ra;
	unsigned char* rb;
	unsigned char* rc;
	unsigned char* rd;
	unsigned char* re;
	unsigned char* w;
	USHORT Stack[8];
	USHORT* sp; //stackpointer
	unsigned char mode;
	unsigned char option; //8 bit
	unsigned char Tris[5];
	unsigned char RInput[5];
	Uint32 t1;
	// Unsigned, because the counter runs over after about 27 seconds at 80 MHz.
	unsigned int cycle;
	unsigned int cycle2;
	unsigned int overshoot; // cycles the last runCycles() went beyond its target
	int prescaler; // prescaler for the timer
	Video* video;
	Bu8763* sound;
	unsigned char lastRa; // RA as the sound chip has seen it
	Uint16 joystickr;
	inline USHORT getRegisterIndex(USHORT fr);
	inline void updateZeroFlag(USHORT value);
	void setBit(USHORT fr, USHORT bit, bool setValue);
	void skipInstructions();
	void movFrW(USHORT fr);
	void movWFr(USHORT fr) ;
	void incDecFr(USHORT fr, bool increment);
	void logicOperation(USHORT fr, USHORT operation, bool wToFr);
};
#endif

