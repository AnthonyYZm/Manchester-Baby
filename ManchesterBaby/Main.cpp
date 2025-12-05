#include <iostream>
#include "Assembler.h"

int main(int argc, char* argv[]) {
    // 1. Setup file names
    // Default values if no arguments provided
    std::string inputFile = "program.txt";   // The assembly source code
    std::string outputFile = "output.txt";   // The machine code output

    // Allow user to specify files via command line (Optional but good for Linux)
    if (argc > 1) {
        inputFile = argv[1];
    }
    if (argc > 2) {
        outputFile = argv[2];
    }

    std::cout << "Starting Manchester Baby Assembler..." << std::endl;
    std::cout << "Input: " << inputFile << " | Output: " << outputFile << std::endl;

    // 2. Instantiate the Assembler
    Assembler assembler;

    // 3. Load Source File
    if (!assembler.load(inputFile)) {
        return 1; // Exit with error
    }

    // 4. Run Pass 1 (Symbol Table Construction)
    std::cout << "Running Pass 1 (Scanning Labels)..." << std::endl;
    if (!assembler.pass1()) {
        std::cerr << "Pass 1 Failed." << std::endl;
        return 1;
    }

    // (Optional) Print Symbol Table for debug/extension marks
    assembler.printSymbolTable();

    // 5. Run Pass 2 (Code Generation)
    std::cout << "Running Pass 2 (Generating Code)..." << std::endl;
    if (!assembler.pass2()) {
        std::cerr << "Pass 2 Failed." << std::endl;
        return 1;
    }

    // 6. Save Result
    assembler.saveFile(outputFile);

    std::cout << "Assembly complete." << std::endl;
    return 0;
}