#A Manchester-Baby simulator and Assembler  

\Contributors: @Hao-xianmi @ikun1028-nobody @Thomaslai1 @WilliamBosman927

## Compilation Command

To compile the complete system (Assembler + Simulator), use:

g++ -std=c++11 Main.cpp Assembler.cpp Simulator.cpp -o baby

## Basic Usage
### Assembler Mode

./baby -a program.asm program.bin

### Simulator Mode (Standard 32-line memory)

./baby -s program.bin

### Simulator Mode (Extended 64-line memory)

./baby -s program.bin -e

### Help

./baby -h

## File Structure

- `Main.cpp` - Program entry point and command-line interface

- `Assembler.cpp` - Assembly to machine code translation

- `Simulator.cpp` - Manchester Baby hardware simulation

- Corresponding header files (.h) - Class definitions



\## Requirements

\- Ubuntu Linux

\- GCC/G++ compiler with C++11 support




