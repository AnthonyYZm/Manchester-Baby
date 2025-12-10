/**
 * @file main.cpp
 * @brief Manchester Baby System (Assembler & Simulator)
 *
 * This file contains the main entry point for the Manchester Baby computer system,
 * which includes both an assembler and a simulator. The program can operate in
 * two modes: assembly mode (converts assembly code to machine code) and
 * simulation mode (executes machine code on the Manchester Baby simulator).
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "Simulator.h"
#include "Assembler.h"

 /**
  * @brief Run the assembler to convert assembly code to machine code
  *
  * @param inputFile Path to the assembly source file
  * @param outputFile Path to save the generated machine code
  * @return int Returns 0 on success, 1 on failure
  */
int runAssembler(const std::string& inputFile, const std::string& outputFile);

/**
 * @brief Run the simulator to execute machine code
 *
 * @param inputFile Path to the machine code file
 * @param extended If true, use extended 64-line memory mode; otherwise use standard 32-line memory
 * @return int Returns 0 on success, 1 on failure
 */
int runSimulator(const std::string& inputFile, bool extended);

/**
 * @brief Print usage instructions for the program
 *
 * @param programName The name of the executable (argv[0])
 */
void printUsage(const char* programName);

/**
 * @brief Main entry point for the Manchester Baby system
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return int Program exit code (0 for success, non-zero for error)
 *
 * The main function parses command-line arguments to determine the operating mode:
 * - Assembler mode (-a): Converts assembly code to machine code
 * - Simulator mode (-s): Executes machine code on the simulator
 * - Help mode (-h): Displays usage information
 *
 * The Manchester Baby was the world's first stored-program computer, developed
 * at the University of Manchester in 1948. This program simulates its operation.
 */
int main(int argc, char* argv[]) {
    // Check minimum argument count
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    // Assembler mode
    if (mode == "-a" || mode == "--assemble") {
        if (argc < 3) {
            std::cerr << "Error: Assembler requires an input file." << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        std::string inputFile = argv[2];
        std::string outputFile = (argc > 3) ? argv[3] : "output.txt";
        return runAssembler(inputFile, outputFile);
    }
    // Simulator mode
    else if (mode == "-s" || mode == "--simulate") {
        if (argc < 3) {
            std::cerr << "Error: Simulator requires an input file." << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        std::string inputFile = argv[2];
        bool extended = (argc > 3 && std::string(argv[3]) == "-e");
        return runSimulator(inputFile, extended);
    }
    // Help mode
    else if (mode == "-h" || mode == "--help") {
        printUsage(argv[0]);
        return 0;
    }
    // Invalid mode
    else {
        std::cerr << "Error: Unknown mode '" << mode << "'" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
}

/**
 * @brief Execute the assembler to convert assembly code to machine code
 *
 * This function handles the assembly process by:
 * 1. Loading the source assembly file
 * 2. Building a symbol table for labels
 * 3. Generating machine code from assembly instructions
 * 4. Saving the output to a file
 *
 * @param inputFile Path to the assembly source file
 * @param outputFile Path to save the generated machine code
 * @return int Returns 0 on success, 1 on failure
 */
int runAssembler(const std::string& inputFile, const std::string& outputFile) {
    std::cout << "=== Manchester Baby Assembler ===" << std::endl;
    std::cout << "Input: " << inputFile << " | Output: " << outputFile << std::endl;

    Assembler assembler;

    // Load source code
    if (!assembler.loadSource(inputFile)) {
        std::cerr << "Failed to load source file." << std::endl;
        return 1;
    }

    // Build symbol table (first pass)
    std::cout << "Building symbol table..." << std::endl;
    if (!assembler.buildSymbolTable()) {
        std::cerr << "Symbol table construction failed." << std::endl;
        return 1;
    }

    assembler.displaySymbolTable();

    // Generate machine code (second pass)
    std::cout << "Generating machine code..." << std::endl;
    if (!assembler.generateMachineCode()) {
        std::cerr << "Machine code generation failed." << std::endl;
        return 1;
    }

    // Save output
    assembler.saveOutput(outputFile);
    std::cout << "Assembly complete. " << assembler.getMachineCodeLineCount()
        << " lines of machine code generated." << std::endl;
    return 0;
}

/**
 * @brief Execute the simulator to run machine code
 *
 * This function loads a machine code program and executes it on the
 * Manchester Baby simulator. It supports two modes:
 * - Standard mode: 32-line memory (original Baby configuration)
 * - Extended mode: 64-line memory (enhanced configuration)
 *
 * @param inputFile Path to the machine code file
 * @param extended If true, use extended 64-line memory mode
 * @return int Returns 0 on success, 1 on failure
 */
int runSimulator(const std::string& inputFile, bool extended) {
    std::cout << "=== Manchester Baby Simulator ===" << std::endl;
    std::cout << "Loading machine code from: " << inputFile << std::endl;

    if (extended) {
        std::cout << "Mode: Extended (64-line memory)" << std::endl;
    }
    else {
        std::cout << "Mode: Standard (32-line memory)" << std::endl;
    }

    // Create simulator instance
    Simulator baby(extended);

    // Load and run program
    if (baby.loadProgram(inputFile)) {
        baby.run();
        return 0;
    }
    else {
        std::cerr << "Failed to load machine code program." << std::endl;
        return 1;
    }
}

/**
 * @brief Display program usage instructions
 *
 * This function prints a help message showing how to use the program,
 * including available command-line options and examples.
 *
 * @param programName The name of the executable (typically argv[0])
 */
void printUsage(const char* programName) {
    std::cout << "Manchester Baby Computer System" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  Assembler: " << programName << " -a <source.txt> [output.txt]" << std::endl;
    std::cout << "  Simulator: " << programName << " -s <machine_code.txt> [-e]" << std::endl;
    std::cout << "  Help:      " << programName << " -h" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a, --assemble   Run assembler on source file" << std::endl;
    std::cout << "  -s, --simulate   Run simulator on machine code file" << std::endl;
    std::cout << "  -e               Enable extended mode (64-line memory)" << std::endl;
    std::cout << "  -h, --help       Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " -a program.asm program.bin" << std::endl;
    std::cout << "  " << programName << " -s program.bin" << std::endl;
    std::cout << "  " << programName << " -s program.bin -e" << std::endl;
}