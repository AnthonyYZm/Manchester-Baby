/**
 * @file Simulator.h
 * @brief Header file for the Manchester Baby Computer Simulator
 *
 * This file contains the class definition for the Simulator that emulates
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <iomanip>

 // Type definitions for consistent 32-bit integer usage
using u32 = uint32_t; ///< 32-bit unsigned integer (for Store, CI, PI)
using i32 = int32_t;  ///< 32-bit signed integer (for Accumulator, can be negative)

/**
 * @class Simulator
 * @brief Manchester Baby Computer Simulator
 *
 * This class emulates the Manchester Small-Scale Experimental Machine (The Baby),
 * the world's first stored-program computer. It supports both standard 32-line
 * memory mode and extended 64-line memory mode with additional instructions.
 */
class Simulator {
private:
    // --- Hardware Components ---
    std::vector<u32> store;     ///< Main memory: 32 or 64 lines of 32-bit words
    i32 accumulator;            ///< Accumulator register (A)
    u32 CI;                     ///< Control Instruction register (Program Counter)
    u32 PI;                     ///< Present Instruction register
    u32 X;                      ///< Index Register (X) - NEW for Indexed Addressing
    bool stopped;               ///< Flag indicating if machine is halted
    bool extended;              ///< Flag for extended mode (64 lines vs 32)

    u32 addressMask;
    // --- Helper Functions ---

    /**
     * @brief Converts a binary string to integer (Little-endian format)
     * @param binary String of '0's and '1's representing binary data
     * @return 32-bit unsigned integer value
     *
     * The Manchester Baby uses Little-endian format where the least significant
     * bit is stored at the leftmost position (bit 0).
     */
    u32 binaryStringToInt(const std::string& binary);
    /**7
     * @brief Converts an integer to Little-endian binary string
     * @param value 32-bit unsigned integer to convert
     * @return Binary string representation in Little-endian format
     */
    std::string intToBinaryString(u32 value);

    /**
     * @brief Fetches the next instruction from memory
     *
     * Loads the instruction at the current CI address into the PI register.
     * Handles memory addressing based on the current mode (standard or extended).
     */
    void fetch();

    /**
     * @brief Decodes and executes the current instruction
     *
     * Interprets the opcode and operand from the PI register, then performs
     * the corresponding operation. Supports both standard and extended instruction sets.
     */
    void decodeAndExecute();

public:
    /**
     * @brief Constructs a new Simulator instance
     * @param ext Extended mode flag (true for 64-line memory, false for 32-line)
     *
     * Initializes all registers to zero and configures memory size based on mode.
     * Extended mode provides 64 lines of memory instead of the standard 32.
     */
    Simulator(bool ext = false);

    /**
     * @brief Loads a machine code program from file into memory
     * @param filename Path to the machine code file
     * @return true if successful, false on error
     *
     * Reads a text file containing 32-bit binary strings (one per line)
     * and loads them into the machine's memory store.
     */
    bool loadProgram(const std::string& filename);

    /**
     * @brief Executes a single instruction cycle
     *
     * Performs one complete machine cycle: increment CI, fetch instruction,
     * decode and execute, then display the updated machine state.
     */
    void step();

    /**
     * @brief Runs the simulator in automatic mode until halt
     *
     * Continuously executes instruction cycles until the machine halts
     * (via STP instruction) or the user interrupts with 'q' key.
     * Provides interactive stepping with Enter key.
     */
    void run();

    /**
     * @brief Displays the current state of the machine
     *
     * Shows register values, memory contents, and decoded instruction information
     * in a format resembling the original CRT display.
     */
    void displayState();

    /**
     * @brief Checks if the machine is halted
     * @return true if machine is stopped, false otherwise
     */
    bool isHalted() const { return stopped; }
};

#endif // SIMULATOR_H