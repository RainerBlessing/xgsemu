
XGSEmu - 0.06

This is an emulator for the XGS Micro Edition. Most of the
demos included on the CD are working, but it is pretty unfinished.

Implemented:
- Video emulation
- SRAM emulation
- CPU emulation (including the timer RTCC, the MODE register and the carry
  input of ADD/SUB)
- Sound emulation (BU8763 melody mode, 3 channels)
- First Joystick

Missing:
- Keyboard emulation
- Interrupts
- Timers T1/T2, comparator, watchdog, sleep
- DTMF/tone mode of the sound chip

Joystick Keys: Cursor Keys (Directions) + Ctrl (Fire Button)
Other keys: F5 (Reset), F11 (Full screen), Ctrl+L (Load a program from the
last used directory), Esc (Quit). A program can also be given on the command
line. There is no menu. See README.md for building and hardware requirements.

The example directory contains some running compiled demos.


New versions of this software will be downloadable from http://www.rainerblessing.com/wp/?page_id=87.
You can also download the source code for the Linux version from there.


Rainer Blessing

Email: rainer@theblessing.net
Web  : http://rainerblessing.com

