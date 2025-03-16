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
#ifndef CPU
#define CPU
#define USHORT unsigned short
#include <string>
#include "Video.h"

using std::string;

/**
 * This is the most important class, it emulates the SX52.
 * @author Rainer Blessing <rainer@theblessing.net>
 */
class Cpu
{
public:
	Cpu(Video* video);
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
	int cycle;
	int cycle2;
	int prescaler; // prescaler for the timer
	Video* video;
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

