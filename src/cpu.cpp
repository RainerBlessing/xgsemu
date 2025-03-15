/*
* =====================================================================================
* 
*        Filename:  cpu.cpp
* 
*     Description:  
* 
*         Version:  1.0
*         Created:  13.07.2005 23:17:59 CEST
*        Revision:  none
*        Compiler:  gcc
* 
*          Author:  Rainer Blessing (), rainer@theblessing.net
*         Company:  
* 
* =====================================================================================
*/
#include <string.h>
#include <sstream>
#include <string>
#include "SDL.h"
#include "Video.h"
//#include "Windows.h"

using namespace std;
#include "cpu.h"

Cpu::Cpu (Video* video)
{
	rtcc = &RAM[1];
	status = &RAM[3];
	fsr = &RAM[4];
	ra = &RAM[5];
	rb = &RAM[6];
	rc = &RAM[7];
	rd = &RAM[8];
	re = &RAM[9];
	w = &RAM[261];
	t1 = SDL_GetTicks();
	reset();
	this->video = video;
}

void Cpu::reset()
{
	memset (RAM, 0, 262);
	memset (SRAM, 0, 131072*sizeof(unsigned char));
	cycle = 0;
	cycle2 = 0;
	prescaler = 0;
	sp = Stack;
	pc = 0xfff;
}

USHORT *
Cpu::getROM ()
{
	return ROM;
}

unsigned char *
Cpu::getRAM ()
{
	return RAM;
}

USHORT *
Cpu::getPC ()
{
	return &pc;
}

void Cpu::execute ()
{
	register USHORT mne = 0;
	register USHORT bit;
	register USHORT fr;
	register USHORT a;
	int i = 0;

	while (i++ < 1000000)
	{
		mne = ROM[pc];

		prescaler += (cycle - cycle2);
		if (prescaler > (2 << (option&0x7)))
		{
			RAM[1]++;
			prescaler = 0;
		}

		RAM[1] &= 0xff;
		cycle2 = cycle;
		switch (mne >> 8)
		{
				case 0xa:    			//jmp 101k kkk kkkk
				pc = ((*status & 0xe0) << 4) | (mne & 0x1ff);
				cycle += 3;
				continue ;
				case 0xb:    			//jmp 101k kkk kkkk
				pc = ((*status & 0xe0) << 4) | (mne & 0x1ff);
				cycle += 3;
				continue ;
				case 0xc:    				//mov w,lit 1100 kkkk kkkk
				*w = mne & 0xff;

				cycle++;
				pc++;
				continue ;
				case 0x5:    				//setb fr.bit 0101 bbbf ffff
				bit = (mne >> 5) & 7;
				fr = mne & 0x1f;

				if (fr > 0x4 && fr < 0xa && ~Tris[fr - 5]&bit == bit)
				{
					switch (fr - 5)
					{
							case 0:
							if (bit < 3)
							{
								*ra |= 1 << bit;
								joystick();
							}
							break;
							case 2:
							*rc |= 1 << bit;
							sram();
							break;
							default:
							break;
					}
					//RInput[fr-5]|=1<<bit;
					pc++;
					continue ;
				}
				if (fr == 0)
					fr = *fsr;
				else if (fr > 0xf)
					fr = (*fsr & 0xf0) | (fr & 0xf);

				RAM[fr] |= 1 << bit;
				cycle++;
				pc++;
				continue;
				case 0x6:    				//snb fr.bit 0110 bbbf ffff
				bit = (mne >> 5) & 7;
				fr = mne & 0x1f;
				if (fr == 0)
					fr = *fsr;
				else if (fr > 0xf)
					fr = (*fsr & 0xf0) | (fr & 0xf);

				if (!(RAM[fr] >> bit & 1))
				{
					pc++;	//skip if fr==0
					cycle++;
					while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
					{
						pc++;	//skip if page/bank
						cycle++;
					}
				}
				cycle++;
				pc++;
				continue;
				case 0xe:    				//and w,#lit 1110 kkkk kkkk
				fr = mne & 0xff;
				if (fr == 0)
				{
					fr = *fsr;
				}
				*w = *w & fr;
				if (*w == 0)
					*status |= 1 << 2;
				else
				{
					*status &= ~(1 << 2);
				}
				cycle++;
				pc++;
				continue;
				case 0xd:    			//or w,#lit 1101 kkkk kkkk
				fr = mne & 0xff;

				*w = *w | fr;
				if (*w == 0)
					*status |= 1 << 2;
				else
				{
					*status &= ~(1 << 2);
				}
				cycle++;
				pc++;
				continue ;
				case 0xf:    	//xor w,#lit 1111 kkkk kkkk
				*w ^= mne & 0xff;
				if (*w == 0)
					*status |= 1 << 2;
				else
				{
					*status &= ~( 1 << 2 );
				}
				cycle++;
				pc++;
				continue;
				case 0x7:    //sb fr.bit 0111 bbbf ffff
				fr = mne & 0x1f;
				if (fr == 0)
					fr = *fsr;
				else if (fr > 0xf)
					fr = (*fsr & 0xf0) | (fr & 0xf);

				bit = (mne >> 5) & 7;
				if (((RAM[fr] >> bit) & 1) == 1)
				{
					pc++;	//skip if fr==0
					cycle++;
					while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
					{
						pc++;	//skip if page/bank
						cycle++;
					}
				}
				cycle++;
				pc++;
				continue ;
				case 0x4:    		//clrb fr.bit 0100 bbbf ffff
				bit = (mne >> 5) & 7;
				fr = mne & 0x1f;
				if (fr > 0x4 && fr < 0xa && ~Tris[fr - 5]&bit == bit)
				{
					switch (fr - 5)
					{
							case 0:
							if (bit < 3)
							{
								*ra &= ~(1 << bit);
								joystick();
							}
							break;
							case 2:
							*rc &= ~(1 << bit);
							sram();
							break;
							default:
							break;
					}
					//RInput[fr-5]&=~(1<<bit);
					pc++;
					continue ;
				}
				if (fr == 0)
					fr = *fsr;
				else if (fr > 0xf)
					fr = (*fsr & 0xf0) | (fr & 0xf);

				RAM[fr] &= (~(1 << bit));
				cycle++;
				pc++;
				continue ;
				case 0x9:    //call addr8 1001 kkkk kkkk
				*sp = pc + 1;
				if (sp == Stack + 8)
				{
					sp = Stack;
				}
				else sp++;
				pc = (*status & 0xe0) << 4 | mne & 0xff;
				cycle += 3;
				continue ;
				case 0x8:    //retw #lit 1000 kkkk kkkk
				if (sp == Stack)
				{
					sp = Stack + 8;
				}
				else sp--;
				pc = *sp;
				*w = mne & 0xff;
				cycle += 3;
				continue;
				default:
				switch (mne >> 5)
				{
						case 0x1:    //mov fr,w 0000 001f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);
						RAM[fr] = *w;
						cycle++;
						pc++;
						switch (fr - 5)
						{
								case 0:
								joystick();
								break;
								case 4:
								video->changeVideo(cycle, (unsigned char)*w&0xff);
								break;
								default:
								break;
						}
						continue;
						case 0xf:    //add fr,w 0001 111f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr] < 0x10 && (*w) + RAM[fr] > 0xf)
						{
							*status |= 1 << 1;	//set DC
						}
						else
						{
							*status &= ~(1 << 1);
						}
						if (fr != 2)
							a = RAM[fr] + *w;
						else
							a = pc + *w;
						if (a > 0xff)
							*status |= 1;	//set carry
						else
						{
							*status &= ~1;
						}

						if (fr != 2)
							RAM[fr] = a & 0xff;
						else
							pc = a;


						if (a == 0)
							*status |= 1 << 2;	// set zero
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0xe:    //add w,fr 0001 110f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (*w < 0x10 && (*w) + RAM[fr] > 0xf)
							*status |= 1 << 1;	//set DC
						else
						{
							*status &= ~( 1 << 1);
						}
						a = *w + RAM[fr];
						if (a > 0xff)
							*status |= 1;	//set carry
						else
						{
							*status &= ~1;
						}
						*w = a & 0xff;

						if (*w == 0)
							*status |= 1 << 2;	// set zero
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0xb:    //and fr,w 0001 011f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] = RAM[fr] & *w;
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0xa:    //and w,fr 0001 010f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = *w & RAM[fr];
						if (*w == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x13:    //not fr 0010 011f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] = ~RAM[fr];
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;

						pc++;
						continue;
						case 0x9:    				//or fr,w 0001 001f fff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] |= *w;
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x8:    //or w,fr 0001 000f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w |= RAM[fr];
						if (*w == 0)
							*status = 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0xd:    			//xor fr,w 0001 101f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] ^= *w;
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0xc:    //xor w,fr 0001 100f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w ^= RAM[fr];
						if (*w == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x3:    		//clr fr 0000 011f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] = 0;
						if (fr > 0x4 && fr < 0xa && ~Tris[fr - 5]&bit == bit)
						{
							switch (fr - 5)
							{
									case 0:
									if (bit < 3)
									{
										*ra &= ~(1 << bit);
										joystick();
									}
									case 4:
									video->changeVideo(cycle, 0);
									break;
									default:
									break;
							}
						}
						*status |= 1 << 2;
						cycle++;
						pc++;
						continue;
						case 0x7:    			//dec fr 0000 111f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] -= 1;
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						switch (fr - 5)
						{
								case 0:
								joystick();
								break;
								case 4:
								video->changeVideo(cycle, (unsigned char)RAM[fr]&0xff);
								break;
								default:
								break;
						}
						cycle++;
						pc++;
						continue;
						case 0x17:    			//decsz fr 0010 111f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] -= 1;

						if (RAM[fr] == 0)
						{
							*status |= 1 << 2;
							pc++;	//skip if fr==0
							cycle++;
							while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
							{
								pc++;	//skip if page/bank
								cycle++;
							}
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue ;
						case 0x15:    				//inc fr 0010 101f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] ++;
						if (RAM[fr] == 0)
							*status |= 1 << 2;
						else
						{
							*status &= ~(1 << 2);
						}
						switch (fr - 5)
						{
								case 0:
								joystick();
								break;
								case 4:
								video->changeVideo(cycle, (unsigned char)RAM[fr]&0xff);
								break;
								default:
								break;
						}
						cycle++;
						pc++;
						continue;
						case 0x1f:    			//incsz fr 0011 111f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr]++;
						if (RAM[fr] == 0)
						{
							*status |= 1 << 2;
							pc++;	//skip if fr==0
							cycle++;
							while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
							{
								pc++;	//skip if page/bank
								cycle++;
							}
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x1b:    		//RL fr 0011 011f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr] >> 7 == 1)
						{
							RAM[fr] = (RAM[fr] << 1) | (*status & 1);
							*status |= 1; //set carry
						}
						else
						{
							RAM[fr] = (RAM[fr] << 1) | (*status & 1);
							*status &= ~1;
						}
						cycle++;
						pc++;
						continue;
						case 0x19:    //RR fr 0011 0001f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr]&0x1 == 0x1)
						{
							RAM[fr] = (RAM[fr] >> 1) | ((*status & 1) << 7);
							*status |= 1; //set carry
						}
						else
						{
							RAM[fr] = (RAM[fr] >> 1) | ((*status & 1) << 7);
							*status &= ~1;
						}

						cycle++;
						pc++;
						continue;
						case 0x5:    //sub fr,w 0000 101f ffff
						//TODO: complement of carry flag ...
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr] > 0xf && RAM[fr] - *w > 0x10)
							*status |= 1 << 1;	//set DC
						else
						{
							*status &= ~(1 << 1);
						}
						if (RAM[fr] >= *w)
							*status |= 1;	//set carry
						else
						{
							*status &= ~1;
						}
						RAM[fr] -= *w;
						if (*w == 0)
							*status |= 1 << 2;	// set zero
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x1d:    //swap fr 0011 101f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						RAM[fr] = (RAM[fr] >> 4) | ((RAM[fr] & 0xf) << 4);
						cycle++;
						pc++;
						continue;
						case 0x10:    //mov w,fr 0010 000f ffff
						fr = mne & 0x1f;

						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = RAM[fr];
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x4:    //mov w,fr-w 0000 100f ffff
						//TODO: CF, complement, DC...
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr] >= *w)
							*status |= 1;	//set carry
						else
						{
							*status &= ~1;
						}
						*w = RAM[fr] - *w;
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x12:    //mov w,/fr (complement) 0010 010f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = ~RAM[fr];
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x6:    //mov w,--fr 0010 100f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = RAM[fr] - 1;
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x14:    	//mov w,++fr 0010 100f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = RAM[fr] + 1;
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x1a:    		//mov w,<<fr 0011 010f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						a = (RAM[fr] << 1) | (*status & 1);
						if (a > 0xff == 0)
							*status |= 1; //set carry
						else
						{
							*status &= ~1;
						}

						*w = a & 0xff;
						cycle++;
						pc++;
						continue;
						case 0x18:    //mov w,>>fr 0011 000f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						a = (RAM[fr] >> 1) | (*status & 1) << 7;
						if (a > 0xff)
							*status |= 1; //set carry
						else
						{
							*status &= ~1;
						}
						*w = a & 0xff;
						cycle++;
						pc++;
						continue;
						case 0x1c: 	//mov w,<>fr 0011 100f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = (RAM[fr] >> 4) | ((RAM[fr] & 0xf) << 4);
						cycle++;
						pc++;
						continue;

						case 0x16:    //movsz w,--fr 0010 110f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = RAM[fr] - 1;
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
							pc++;	//skip if fr==0
							cycle++;
							while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
							{
								pc++;	//skip if page/bank
								cycle++;
							}
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x1e:    	//movsz w,++fr 0011 110f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						*w = RAM[fr] + 1;
						if (*w == 0)
						{
							*status |= 1 << 2;	// set zero
							pc++;	//skip if fr==0
							cycle++;
							while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
							{
								pc++;	//skip if page/bank
								cycle++;
							}
							cycle++;
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						case 0x11:    	//test fr 0010 001f ffff
						fr = mne & 0x1f;
						if (fr == 0)
							fr = *fsr;
						else if (fr > 0xf)
							fr = (*fsr & 0xf0) | (fr & 0xf);

						if (RAM[fr] == 0)
						{
							*status |= 1 << 2;	// set zero
						}
						else
						{
							*status &= ~(1 << 2);
						}
						cycle++;
						pc++;
						continue;
						default:
						if (mne == 0x40)
						{				//clr w 0000 0100 0000
							*w = 0;
							*status |= 1 << 2;
							cycle++;
						}
						else if (mne == 4)
						{				//clr !WDT 0000 0000 0100
							//TODO
							//TO=1
							//PD=1
							cycle++;

						}
						else if (mne == 0x42)
						{ //mov w,m 0000 0100 0010
							*w = mode;
							cycle++;
						}

						else if (mne == 0x43)
						{ //mov m,w 0000 0100 011
							mode = *w;
							cycle++;
						}
						else if (mne >> 4 == 0x5)
						{ //mov m,#lit 0000 0101 kkkk
							mode = mne & 0xf;
							cycle++;
						}
						else if (mne == 0x2)
						{ //mov !option,w 0000 0000 0010
							option = *w;
							cycle++;
						}
						else if (mne == 0)
						{				//nop
							cycle++;
						}
						else if (mne == 0xc)
						{ //ret 0000 0000 1100
							if (sp == Stack)
							{
								sp = Stack + 8;
							}
							else sp--;
							pc = ((*status & 0xe0) << 4) | *sp;
							cycle += 3;
							continue ;
						}
						else if (mne == 0xd)
						{ //retp
							if (sp == Stack)
							{
								sp = Stack + 8;
							}
							else sp--;
							USHORT page = *sp >> 4;
							RAM[3] = page | (RAM[3] & 0x1f);
							pc = *sp;
							cycle += 3;
							continue;
						}
						else if (mne >> 3 == 3)
						{ //bank addr12 0000 0001 1nnn
							*fsr = ((mne & 7) << 4) | (*fsr & 0xf);
							cycle++;
						}
						else if (mne >> 3 == 2)
						{ //page addr12 0000 0001 0nnn
							*status = ((mne & 7) << 5) | (*status & 0x1f);
							cycle++;
						}
						else if (mne == 0x41)
						{ //IREAD 0000 0100 0001
							a = ((mode & 0xf) << 8) | *w;
							a = ROM[a] & 0xfff;
							mode = (mode & 0xf0) | (a >> 8);
							*w = a & 0xff;
							cycle += 3;
						}
						else if (mne >= 0x5 && mne <= 0x9)
						{ //mov !rx,w
							if (mode == 0xf)
							{
								Tris[mne - 5] = *w;
							}
							cycle++;
						}
						//TODO: reti
						//TODO: retiw
						//TODO: SLEEP


						else
						{
							cycle++;
						}
						pc++;

						continue;
				}
		}
	}
}

void Cpu::joystick()
{
	static bool shift = false;
	static bool setb = false;
	static char jptr;
	shift = false;
	if ((*ra&2) == 0)
	{
		jptr = 15;
	}
	if ((*ra&1) == 1)
	{
		setb = true;
	}
	if ((*ra&1) == 0 && setb == true)
	{
		shift = true;
		setb = false;
	}
	if (shift == true && jptr > -1)
	{
		if ((joystickr >> jptr) &1 == 1)
		{
			*ra = (*ra & 0xf8) | (*ra & 0x3);
		}
		else
			*ra = (*ra & 0xf8) | 4 | (*ra & 0x3);
		jptr--;
	}
}
void Cpu::sram()
{
	static USHORT page = 0;
	static USHORT address;
	static bool active = false;
	if ((*rc >> 4)&1 == 1)
	{
		page <<= 1; //12 bit
		page |= (*rc >> 5) & 1;
	}
	if (((*rc >> 6)&1) == 0)
	{ // chipenable=0
		address = ((*ra >> 7) & 1) << 16 | page << 4 | *rc & 0xf;

		if (((RAM[7] >> 7)&1) == 0)
		{ //write
			SRAM[address] = *rd;
		}
		else
		{ //read
			*rd = SRAM[address];
		}
		active = true;
	}
	if (((RAM[7] >> 6)&1) == 1 && active == true)
	{ // chipenable=1
		page = 0;
		active = false;
	}
}

