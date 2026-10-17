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
#include "Bu8763.h"

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
	 * The joystick input is handled in XGSEmu::handleEvents().
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
	 * Wait loops (djnz on a register, jumping to itself) are done in one step.
	 * Nothing changes but the number of instructions that have to be executed,
	 * switch it off to execute every round on its own (used by the tests).
	 */
	void setFastLoops(bool enabled)
	{
		fastLoops = enabled;
	};
	/**
	 * Number of clock cycles executed so far.
	 */
	int getCycle() const
	{
		return (int)cycle;
	};
	/**
	 * Sets the FUSEX word of the program. Bit 6 (CF) decides whether the carry flag is an
	 * input of ADD and SUB (0) or not (1). The fuses are part of the program file and are not
	 * touched by reset().
	 */
	void setFusex(USHORT fusex);
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
	unsigned char mode; // five bits, 0x1F after a reset
	unsigned char option; //8 bit
	unsigned char Tris[5]; // data direction of the ports RA..RE: 1 is an input
	unsigned char portConfig[3][5]; // pull-ups (MODE 0xE), input levels (0xD), Schmitt triggers (0xC)
	unsigned char RInput[5];
	Uint32 t1;
	// Unsigned, because the counter runs over after about 27 seconds at 80 MHz.
	unsigned int cycle;
	unsigned int cycle2;
	unsigned int overshoot; // cycles the last runCycles() went beyond its target
	int prescaler; // prescaler for the timer
	bool fastLoops;
	Video* video;
	Bu8763* sound;
	unsigned char lastRa; // RA as the sound chip has seen it
	Uint16 joystickr;
	// state of the serial joystick interface and of the SRAM address logic
	bool carryInput; // CF of the FUSEX word is 0: ADD and SUB take the carry flag as input
	bool joystickClockHigh;
	char joystickBit;
	USHORT sramPage;
	bool sramActive;
	typedef void (Cpu::*Instruction)(USHORT mne);
	static Instruction lookupInstruction(USHORT mne);
	void updateSound();
	void updateTimer();
	void registerWritten(USHORT fr);

	// operands, flags and the like
	USHORT registerAddress(USHORT mne) const;
	void setZero(bool zero);
	void setCarry(bool carry);
	void setDigitCarry(bool digitCarry);
	int carryIn() const;
	int subtract(unsigned char minuend, unsigned char subtrahend);
	void next(int cycles = 1);
	void skipFollowing();
	void skipWaitLoopRounds(USHORT fr);
	void pushReturnAddress(USHORT address);
	USHORT popReturnAddress();
	void portWritten(USHORT fr, unsigned char value);
	bool portBitWritten(USHORT fr, USHORT bit, bool set);

	// the instructions
	void jmp(USHORT mne);
	void call(USHORT mne);
	void retw(USHORT mne);
	void movWLiteral(USHORT mne);
	void andWLiteral(USHORT mne);
	void orWLiteral(USHORT mne);
	void xorWLiteral(USHORT mne);
	void setb(USHORT mne);
	void clrb(USHORT mne);
	void snb(USHORT mne);
	void sb(USHORT mne);
	void registerInstruction(USHORT mne);
	void controlInstruction(USHORT mne);
	void movPortControl(USHORT mne);
	unsigned char* controlRegister(int port);
	void setW(unsigned char value);
	void setRegister(USHORT fr, unsigned char value);
	void skipIfZero(unsigned char value);
	void movFrW(USHORT mne);
	void clrFr(USHORT mne);
	void decFr(USHORT mne);
	void incFr(USHORT mne);
	void decsz(USHORT mne);
	void incsz(USHORT mne);
	void movszDec(USHORT mne);
	void movszInc(USHORT mne);
	void andFrW(USHORT mne);
	void orFrW(USHORT mne);
	void xorFrW(USHORT mne);
	void notFr(USHORT mne);
	void andWFr(USHORT mne);
	void orWFr(USHORT mne);
	void xorWFr(USHORT mne);
	void movWFr(USHORT mne);
	void movWNotFr(USHORT mne);
	void movWDecFr(USHORT mne);
	void movWIncFr(USHORT mne);
	void movWSwapFr(USHORT mne);
	void swapFr(USHORT mne);
	void testFr(USHORT mne);
	void addFrW(USHORT mne);
	void addWFr(USHORT mne);
	void subFrW(USHORT mne);
	void movWFrMinusW(USHORT mne);
	void rlFr(USHORT mne);
	void rrFr(USHORT mne);
	void movWRlFr(USHORT mne);
	void movWRrFr(USHORT mne);
};
#endif

