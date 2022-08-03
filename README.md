# Check 2600
By Justin Green

## About
Check 2600 is an Atari 2600 emulator written in C++.
Atari is a Go term that very loosely translates to "check" in Chess, hence the name of this emulator.

## Compatibility
This is the current list of titles that are known work well. Note that this list is not exhaustive, these are just the games I've tested so far.

- Centipede
- Donkey Kong
- Mario Bros
- Pacman
- Pitfall

## Known Issues
- Sound may be 'crunchy' or non-existent on some games. This is due to limitations in QT5's multimedia libraries.
- Only NTSC is currently supported.
- Only paddle controls are currently supported.
- No multiplayer yet.
- Limited bankswitching support.

## Build Instructions
Check 2600 was written to be cross platform, but has only been tested on Linux. Your mileage may vary.

### Dependencies
- QT5 and QT5 headers
- Clang with C++14 support
- [Optional, for tests] acme

### Configuring Your Build
You will need to edit the Makefile to point `INCLUDE` to your QT5 headers. I've included 2 common locations at the top of the file, so hopefully you will just need to uncomment one of them.

### Release Build
Release builds can be build by just running `make` from the source directory.

### Tests
A few simple TIA and 6502 tests are included in the `tests` directory. These can be built using `make test`.

### Debug Build
To include debug symbols in your build, run `make debug`. Note: this will automatically build the tests as well.

## Running a ROM
Once you have built Check, you should see a binary in the source directory called `check2600`. This is the emulator. It takes the following flags:
- "-f <filename>", which specifies the file
- "-s <integer>", which specifies the UI scaling. The original Atari 2600 was 160x192, which will look tiny on a modern display, so we scale it up with this flag. The default value is 4.
- "-b <bank switch type>", which specifies the bank switching type. Currently the options are "none", "Atari8K", "Atari16K", and "Atari32K". The default is none. Note that only Atari8K has been thoroughly tested so far.
- "-d", which activates debug mode. More on this mode in the next section.

### Bank Switching
You can usually tell what the correct size of a ROM is by looking at its filesize. 2KB or 4KB usually indicate a normal ROM, 8KB usually indicates Atari8K bank switching, etc.

Certain games may not work yet because they rely on a bankswitching scheme that isn't yet supported. Dig-Dug, for example, includes extra RAM in the cartridge.

## Debug Mode
Check 2600 includes a basic built-in debugger. The interface for the debugger is command line, but its features are heavily inspired by Stella's graphical debugger.

### Disassembly
Before every command, the disassembly of the instruction at the current program counter is printed out.

### Stepping Code
`step` will step forward one instruction.

`scan` will step forward until it hits the beginning of the next scanline.

`frame` will step forward until it hits the end of a vertical sync cycle.

### Breakpoints
`break XYZW` will set a breakpoint at the hex address 0xXYZW.

`cont` will continue execution until a breakpoint is hit.

`delete XYZW` will delete the breakpoint at address 0xXYZW.

### Info Dumping
`dump reg` will print all available information about the processor's internal state, including its registers.

`dump mem` will print all 128 bytes of RAM (and stack, since these are identical on the 2600).

`dump tia` will print information about the internal state of the Television Interface Adapter.

`dump pia` will print information about the timer built into the Peripheral Interface Adapter.

`dump` or `dump all` will print all of the above.

### Input faking

The debugger can fake input using the `set` and `unset` commands. These commands can be used to toggle a digital input on or off. The inputs are intuitively named "up", "down", "left", "right", and "fire".

## Developer Notes
Patches are welcome!

### Outstanding TODOs
[ ] Test more games and fix bugs as they arise.
[ ] Add more bankswitching formats.
[ ] Add support for more control schemes.
[ ] Improve debugging interface. Possibly add a way to save and rewind state.
[ ] Add PAL and SECAM support.
[ ] Fix the sound subsystem.
[ ] Investigate improving performance with proper JIT compilation.
[ ] Write a unit test that thoroughly exercises CPU instructions and addressing modes.
[ ] Add GUIs other than QT5 software rendering. Maybe ANSI character based, or OpenGL.

### Source code overview
I've tried to comment the code as much as I could to make it easy to understand. One of the goals of this projects was to create a modular 6502 core that could be re-used in future emulators.

#### Atari 2600 Specific
- main.cc
- atari.h/atari.cc: Atari specific setup code and the main emulator loop. Also contains the debugger.
- bank_switchers.h/bank_switchers.cc: Implementation of various bank switching schemes.
- display.h/display.cc: Generic interface for host rendering, sound, and input code.
- input.h/input.cc: Current state of user input.
- ntsc.h/ntsc.cc: Helper class to simulate the sweeping of the electron beam and provide useful constants such as screen width and number of scanlines.
- pia.h/pia.cc: Simulates some of the PIA registers, especially those related to timers.
- qt_display/h/qt_display.cc: QT5 implementation of the Display class.
- sound.h/sound.cc: Current state of sound generator.
- sounds: This directory contains a python script for generating all 512 possible sounds the Atari 2600 can make. During build, this script is run and the wav files are also deposited in the sounds directory.
- tia.h/tia.cc: All TIA related code.

#### 6502 Core
- cpu.h/cpu.cc: High level code for fetch/decode/execute. This class caches instructions to avoid reparsing. It's not a JIT, but it's a similar concept.
- disasm.h/disasm.cc: The debugger's disassembler.
- instructions.h/instructions.cc: Implementation of every 6502 instruction by opcode. Note that these implementations are generally addressing mode agnostic, as that code is handled in the Operands class.
- memory.h/memory.cc: Definition of the various types of memory regions in a 6502 system, and helper functions for directing reads and writes to the appropriate region.
- operand.h/operand.cc: Encapsulates most of the addressing mode logic to simplify instructions.cc.
- registers.h/registers.cc: The current state of the processor, including the number of cycles since power on.

### Making Your Own ROMS
#### Examples
There are some very simple test ROMs located in the "tests" directory demonstrating how to get simple sprites and missiles on the screen.

#### Common Gotchas
One common "gotcha" when writing an Atari 2600 ROM is that the assembler will put all of your code at 0x0000 by default. 0x0000-0x0200 are reserved for the TIA, PIA, and RAM/stack. Sometimes your ROM will work anyway if you assemble it this way, but any absolute addresses will break. Traditionally, ROMs are assembled to occupy the 0xF000-0xFFFF address space, but the address line is only 13-bit, so anything about 0x1000 will technically do.

Another common "gotcha" is forgetting to set the reset vector. Offsets 0xFFC and 0xFFE of your ROM should be a 2-byte little-endian pointer to your entry point.

#### Intro to 2600 Programming
The following is an excellent, but incomplete introduction to programming the 2600: https://cdn.hackaday.io/files/1646277043401568/Atari_2600_Programming_for_Newbies_Revised_Edition.pdf

#### TIA and PIA Registers
I've tried to document all of the TIA and PIA registers in source comments as best as I could, but the following is an excellent reference for these registers: https://problemkaputt.de/2k6specs.htm

#### 6502 Instruction Reference
This is the most thorough reference for the 6502 instruction set that I've ever seen: https://www.masswerk.at/6502/6502_instruction_set.html. The cycle counts in particular are super helpful.
