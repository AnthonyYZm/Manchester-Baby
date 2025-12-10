#include "Simulator.h"
#include <iostream>
#include <limits>

/**
 * @file Simulator.cpp
 * @brief Implementation of the Manchester Baby Machine Simulator
 *
 * This file provides the complete implementation of the historical Manchester Baby
 * computer simulator, including its extended 64-line memory version with additional
 * arithmetic instructions (ADO, MPY). The simulator faithfully reproduces the
 * original machine's behavior with 32-bit word length and binary-coded instructions.
 */

 // Constants for instruction decoding (consistent with assembler)
constexpr u32 OPCODE_SHIFT = 13;           ///< Bit position for opcode extraction (bits 13-15/13-16)
constexpr u32 OPCODE_MASK_STANDARD = 0x7;  ///< Mask for 3-bit opcode (32-line mode, bits 13-15)
constexpr u32 OPERAND_MASK_STANDARD = 0x1F;///< Mask for 5-bit operand (32-line mode, bits 0-4)
constexpr u32 OPCODE_MASK_EXTENDED = 0xF;  ///< Mask for 4-bit opcode (64-line mode, bits 13-16)
constexpr u32 OPERAND_MASK_EXTENDED = 0x3F;///< Mask for 6-bit operand (64-line mode, bits 0-5)

/**
 * @brief Constructor for the Simulator class
 * @param ext Boolean flag indicating whether to use extended mode
 *
 * Initializes the Manchester Baby simulator with either standard 32-line memory
 * or extended 64-line memory configuration. The memory is implemented as a vector
 * of 32-bit unsigned integers. All registers are initialized to zero, and the
 * machine starts in a non-halted state.
 *
 * @note The Manchester Baby used a Williams-Kilburn CRT tube for memory storage
 *       with 32 words of 32 bits each. Extended mode doubles the memory capacity.
 */
Simulator::Simulator(bool ext) {
    extended = ext;
    int memorySize = extended ? 64 : 32;   // 32 or 64 memory locations
    store.resize(memorySize, 0);           // Allocate and zero-initialize memory
    accumulator = 0;                       // Accumulator register (signed 32-bit)
    CI = 0;                                // Current Instruction register (32-bit)
    PI = 0;                                // Present Instruction register (32-bit)
    X = 0;                                 // Index Register (X) - Initialized to zero
    stopped = false;                       // Machine execution state

    if
        (extended) {
        std::cout << ">>> Extended Mode Enabled: 64-line Memory & Extra Instructions (ADO, MPY, LDI, IND, LDIX, SUBX) <<<" << std::endl
            ;
    }
}

/**
 * @brief Converts a binary string to an unsigned 32-bit integer
 * @param binary Input binary string (LSB-first format)
 * @return Unsigned 32-bit integer representation
 *
 * The function processes the binary string in Little-Endian format where
 * the first character represents the least significant bit (LSB). This matches
 * the original Manchester Baby's bit ordering convention.
 *
 * Algorithm:
 * 1. Initialize result to 0
 * 2. For each character in the string (up to 32 characters):
 *    - If character is '1', set corresponding bit in result
 *    - Shift position increases with string index
 * 3. Return the accumulated value
 *
 * Example: "1001" becomes 0b1001 (9) in LSB-first interpretation
 *
 * @complexity O(n) where n is the length of binary string
 */
u32 Simulator::binaryStringToInt(const std::string& binary) {
    u32 value = 0;
    for (size_t i = 0; i < binary.length() && i < 32; ++i) {
        if (binary[i] == '1') {
            value += (1u << i);  // Set bit at position i (0-indexed from LSB)
        }
    }
    return value;
}

/**
 * @brief Converts an unsigned 32-bit integer to a binary string
 * @param value Input 32-bit unsigned integer
 * @return Binary string representation (LSB-first format)
 *
 * Produces a 32-character binary string where the first character
 * represents the least significant bit. This format is used for display
 * and matches the original Manchester Baby's CRT display.
 *
 * Algorithm:
 * 1. Initialize empty result string
 * 2. For bits 0 to 31 (inclusive):
 *    - Extract bit i using (value >> i) & 1
 *    - Append '1' if bit is set, '0' otherwise
 * 3. Return the 32-character string
 *
 * @complexity O(1) - always processes 32 bits
 */
std::string Simulator::intToBinaryString(u32 value) {
    std::string binary = "";
    for (int i = 0; i < 32; ++i) {
        if ((value >> i) & 1) {
            binary += "1";
        }
        else {
            binary += "0";
        }
    }
    return binary;
}

/**
 * @brief Loads a program from a binary file into memory
 * @param filename Path to the program file containing binary instructions
 * @return true if loading succeeded, false otherwise
 *
 * Reads 32-bit binary strings from the file, pads or truncates to exactly
 * 32 bits, and loads them into consecutive memory locations starting at
 * address 0. Each line in the file should contain exactly 32 '0' or '1'
 * characters representing one instruction word.
 *
 * Algorithm:
 * 1. Open file for reading
 * 2. For each line until EOF or memory limit:
 *    a. Handle lines longer than 32 chars: truncate
 *    b. Handle lines shorter than 32 chars: pad with '0's
 *    c. Convert binary string to integer
 *    d. Store in memory at current address
 * 3. Report number of instructions loaded
 *
 * Memory limits: 32 lines in standard mode, 64 lines in extended mode
 *
 * @throws No exceptions, but returns false on file open failure
 */
bool Simulator::loadProgram(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    std::string line;
    int lineCount = 0;
    int maxLines = extended ? 64 : 32;  // Memory capacity

    while (std::getline(file, line) && lineCount < maxLines) {
        // Handle lines longer than 32 bits: truncate to 32 characters
        if (line.length() > 32) {
            line = line.substr(0, 32);
        }
        // Handle lines shorter than 32 bits: pad with zeros at the end
        else if (line.length() < 32) {
            line.append(32 - line.length(), '0');  // Pad with trailing zeros
        }

        // Convert binary string to integer and store in memory
        store[lineCount] = binaryStringToInt(line);
        lineCount++;
    }

    file.close();
    std::cout << "Program loaded (" << lineCount << " lines)." << std::endl;
    return true;
}

/**
 * @brief Fetches the next instruction from memory
 *
 * Loads the instruction from the memory location pointed to by CI
 * (Current Instruction register) into PI (Present Instruction register).
 * The memory address is masked to prevent accessing beyond memory bounds.
 *
 * Addressing details:
 * - Standard mode: 5-bit address (0-31) uses mask 0x1F
 * - Extended mode: 6-bit address (0-63) uses mask 0x3F
 *
 * The fetch operation occurs after CI is incremented in the step() function,
 * implementing a standard fetch-execute cycle.
 */
void Simulator::fetch() {
    u32 mask = extended ? 0x3F : 0x1F;  // Address mask based on mode
    PI = store[CI & mask];              // Fetch instruction to PI register
}

/**
 * @brief Decodes and executes the current instruction
 *
 * Extracts opcode and operand from PI using bitwise operations, then
 * performs the corresponding operation. The instruction format is:
 *
 * Standard mode (32 lines):
 * - Bits 0-4:   Operand (5 bits, 0-31)
 * - Bits 5-12:  Unused (set to 0)
 * - Bits 13-15: Opcode (3 bits, 0-7)
 * - Bits 16-31: Unused (set to 0)
 *
 * Extended mode (64 lines):
 * - Bits 0-5:   Operand (6 bits, 0-63)
 * - Bits 6-12:  Unused (set to 0)
 * - Bits 13-16: Opcode (4 bits, 0-15)
 * - Bits 17-31: Unused (set to 0)
 *
 * Instruction Set Implementation:
 * 0: JMP  - Jump to address (CI = store[operand])
 * 1: JRP  - Jump relative (CI += store[operand])
 * 2: LDN  - Load negative (accumulator = -store[operand])
 * 3: STO  - Store (store[operand] = accumulator)
 * 4,5: SUB - Subtract (accumulator -= store[operand])
 * 6: CMP  - Skip if negative (if (accumulator < 0) CI++)
 * 7: STP  - Stop execution
 * 8: ADO  - Add (extended only, accumulator += store[operand])
 * 9: MPY  - Multiply (extended only, accumulator *= store[operand])
 *
 * Note: The Manchester Baby used two's complement arithmetic. The SUB
 * instruction has two opcode values (4 and 5) for historical reasons.
 */
void Simulator::decodeAndExecute() {
    u32 operand, opcode;

    // Decode instruction based on mode
    if (extended) {
        // Extended mode: 6-bit operand, 4-bit opcode
        operand = PI & OPERAND_MASK_EXTENDED;        // Bits 0-5
        opcode = (PI >> OPCODE_SHIFT) & OPCODE_MASK_EXTENDED;  // Bits 13-16
    }
    else {
        // Standard mode: 5-bit operand, 3-bit opcode
        operand = PI & OPERAND_MASK_STANDARD;        // Bits 0-4
        opcode = (PI >> OPCODE_SHIFT) & OPCODE_MASK_STANDARD;  // Bits 13-15
    }

    // Execute instruction
    switch (opcode) {
    case 0:
        // JMP: Jump to address in memory
        CI = store[operand];
        break;
    case 1:
        // JRP: Jump relative (add signed value from memory)
        CI += store[operand];
        break;
    case 2:
        // LDN: Load negative (two's complement)
        accumulator = -static_cast<i32>(store[operand]);
        break;
    case 3:
        // STO: Store accumulator to memory
        store[operand] = static_cast<u32>(accumulator);
        break;
    case 4:
    case 5:
        // SUB: Subtract (both opcodes 4 and 5 perform subtraction)
        accumulator -= static_cast<i32>(store[operand]);
        break;
    case 6:
        // CMP: Skip next instruction if accumulator is negative
        if (accumulator < 0) CI++;
        break;
    case 7:
        // STP: Stop execution
        stopped = true;
        break;
    case 8:
        // ADO: Add (extended instruction only)
        if (extended) accumulator += static_cast<i32>(store[operand]);
        break;
    case 9:
        // MPY: Multiply (extended instruction only)
        if (extended) accumulator *= static_cast<i32>(store[operand]);
        break;

    case 10:
        // LDI: Load Immediate (Extended only)
        if (extended) {
            accumulator = static_cast<i32>(operand);
        }
        break;
    case 11:
        // IND: Load Negative Indirect (Extended only) - A = -Store[Store[M]]
        if (extended) {
            // Get the address from the address M
            u32 indirectAddress = store[operand & addressMask] & addressMask;
            // Load content from the indirect address
            accumulator = -static_cast<i32>(store[indirectAddress]);
        }
        break;
    case 12:
        // LDIX: Load Immediate Index (Extended only) - X = Operand
        if (extended) {
            // The X register only uses the lower address bits for indexing
            X = operand & addressMask;
        }
        break;
    case 13:
        // SUBX: Subtract Indexed (Extended only) - A = A - Store[M + X]
        if (extended) {
            // Calculate effective address: (Operand + X) mod MemorySize
            u32 effectiveAddress = (operand + X) & addressMask;
            accumulator -= static_cast<i32>(store[effectiveAddress]);
        }
        break;

    default:
        // Invalid opcode - no operation
        break;
    }
}

/**
 * @brief Executes a single instruction cycle
 *
 * Performs the complete fetch-decode-execute cycle for one instruction.
 * The cycle follows the original Manchester Baby's execution sequence:
 * 1. Increment CI (program counter)
 * 2. Fetch instruction from memory location CI
 * 3. Decode opcode and operand
 * 4. Execute the instruction
 * 5. Display updated machine state
 *
 * The CI increment occurs before fetching, which is the standard
 * von Neumann architecture pattern. The function checks the stopped
 * flag to prevent execution after a STP instruction.
 */
void Simulator::step() {
    if (stopped) return;  // Don't execute if machine is halted

    CI++;                 // Increment program counter
    fetch();              // Fetch instruction from memory
    decodeAndExecute();   // Decode and execute instruction
    displayState();       // Show updated machine state
}

/**
 * @brief Runs the simulator interactively
 *
 * Executes the program in step-by-step mode, allowing the user to
 * control execution with Enter key presses. The main loop:
 * 1. Displays initial state
 * 2. Waits for user input (Enter to step, 'q' to quit)
 * 3. Executes one instruction per step
 * 4. Checks for safety limits
 * 5. Displays final state on halt
 *
 * Safety features:
 * - Maximum 10,000 cycles to prevent infinite loops
 * - User can quit at any time with 'q' or 'quit'
 * - Clear state display after each step
 *
 * The interactive mode is useful for debugging and educational purposes,
 * allowing observation of the machine's internal state at each step.
 */
void Simulator::run() {
    std::cout << "=== Initial State ===" << std::endl;
    displayState();

    int cycles = 0;
    std::string input;

    while (!stopped) {
        // Wait for user input
        std::cout << "\nPress Enter to step (or type 'q' to quit)...";
        std::getline(std::cin, input);

        if (input == "q" || input == "quit") break;

        // Execute one instruction cycle
        step();
        cycles++;

        // Safety check for infinite loops
        if (cycles > 10000) {
            std::cout << "Safety limit reached (10000 cycles)." << std::endl;
            break;
        }
    }
    std::cout << "\n=== Machine Halted ===" << std::endl;
}

/**
 * @brief Displays the current state of the machine
 *
 * Shows all registers, the decoded current instruction, and the complete
 * memory contents in a format similar to the original Manchester Baby's
 * CRT display. The display includes:
 *
 * 1. Register values (CI, PI, Accumulator)
 * 2. Decoded instruction mnemonic and operand
 * 3. Complete memory contents with highlighting for current CI location
 *
 * Format details:
 * - Memory addresses are shown in decimal
 * - Memory contents are shown as 32-bit binary strings (LSB-first)
 * - The line containing the current instruction is marked with "<== CI"
 * - Opcode mnemonics match the original Manchester Baby instruction set
 *
 * The display provides a comprehensive view of the machine state for
 * debugging and educational purposes, showing both the raw binary
 * representation and its human-readable interpretation.
 */
void Simulator::displayState() {
    // Decode current instruction for display
    u32 opcode = (PI >> OPCODE_SHIFT) & (extended ? OPCODE_MASK_EXTENDED : OPCODE_MASK_STANDARD);
    u32 operand = PI & (extended ? OPERAND_MASK_EXTENDED : OPERAND_MASK_STANDARD);

    // Convert opcode to mnemonic
    std::string opName;
    switch (opcode) {
    case 0: opName = "JMP"; break;   // Jump
    case 1: opName = "JRP"; break;   // Jump relative
    case 2: opName = "LDN"; break;   // Load negative
    case 3: opName = "STO"; break;   // Store
    case 4: opName = "SUB"; break;   // Subtract
    case 5: opName = "SUB"; break;   // Subtract (alternate opcode)
    case 6: opName = "CMP"; break;   // Skip if negative
    case 7: opName = "STP"; break;   // Stop
    case 8: opName = extended ? "ADO" : "???"; break;  // Add (extended)
    case 9: opName = extended ? "MPY" : "???"; break;  // Multiply (extended)
    case 10: opName = extended ? "LDI" : "???"; break;
    case 11: opName = extended ? "IND" : "???"; break; // Indirect Load
    case 12: opName = extended ? "LDIX" : "???"; break; // Load Immediate Index
    case 13: opName = extended ? "SUBX" : "???"; break; // Subtract Indexed
    default: opName = "???"; break;   // Unknown opcode
    }

    // Display register state
    std::cout << "\n------------------------------------------------\n";
    std::cout << "Registers:\n";
    std::cout << "  CI (Counter)    : " << CI << "\n";
    std::cout << "  PI (Instruction): " << intToBinaryString(PI) << "\n";
    std::cout << "  Decoded Op      : " << opName << "\n";
    std::cout << "  Operand Addr    : " << operand << "\n";
    std::cout << "  Accumulator     : " << accumulator << "\n";
    if (extended) {
        std::cout << "  X (Index)       : " << X << "\n"; // Display Index Register
    }
    std::cout << "------------------------------------------------\n";

    // Display memory (CRT tube simulation)
    std::cout << "Store Memory (CRT Display):\n";
    int lines = extended ? 64 : 32;  // Number of memory lines

    for (int i = 0; i < lines; ++i) {
        // Convert memory value to binary string
        std::string bin = intToBinaryString(store[i]);
        std::cout << std::setw(2) << i << ": " << bin;

        // Mark the line containing the current instruction
        if (i == (CI & (extended ? 0x3F : 0x1F))) {
            std::cout << " <== CI";
        }
        std::cout << "\n";
    }
    std::cout << "------------------------------------------------\n";
}