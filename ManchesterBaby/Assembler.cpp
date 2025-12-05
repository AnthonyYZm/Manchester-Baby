#include "Assembler.h"
#include <sstream>

/**
 * Constructor
 * Initializes the assembler by clearing data structures and reserving memory.
 */
Assembler::Assembler() {
    // 1. Ensure containers are empty to start fresh
    symbolTable.clear();
    sourceLines.clear();
    machineCode.clear();

    // 2. Pre-allocate memory for optimization
    // The Manchester Baby store has exactly 32 lines.
    // Reserving space prevents unnecessary memory re-allocations.
    sourceLines.reserve(32);
    machineCode.reserve(32);
}
/**
 * Helper Function: cleanLine
 * Purpose: Removes comments and whitespace from a single line of code.
 * @param line The raw string from the file.
 * @return The cleaned string.
 */
std::string Assembler::cleanLine(const std::string& line) {
    std::string clean = line;

    // 1. Remove Comments
    // In Baby assembly, comments typically start with a semicolon ';'
        size_t commentPos = clean.find(';');
    if (commentPos != std::string::npos) {
        clean = clean.substr(0, commentPos); // Keep only the text before the semicolon
    }

    // 2. Trim Trailing Whitespace (Right)
    // Removes spaces, tabs, and potential Windows carriage returns (\r)
    clean.erase(std::find_if(clean.rbegin(), clean.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), clean.end());

    // 3. Trim Leading Whitespace (Left)
    // Removes indentation spaces or tabs
    clean.erase(clean.begin(), std::find_if(clean.begin(), clean.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));

    // 4. Convert to Upper Case (Robustness)
    // [cite_start]Ensures 'jmp', 'Jmp', and 'JMP' are all treated identically[cite: 87].
    std::transform(clean.begin(), clean.end(), clean.begin(), ::toupper);

    return clean;
}

/**
 * Method: loadSourceFile
 * Purpose: Reads the assembly source file and prepares it for processing.
 * @param filename The path to the text file.
 * @return true if successful, false if the file cannot be opened.
 */
bool Assembler::load(const std::string& filename) {
    // Open the input text file [cite: 56]
    std::ifstream inputFile(filename);

    // 1. Check if file opened successfully
    if (!inputFile.is_open()) {
        // Error message handling (Informativeness) [cite: 87]
        std::cerr << "Error: Could not open source file '" << filename << "'." << std::endl;
        return false;
    }

    // Clear old data to handle multiple programs in one run [cite: 87]
    sourceLines.clear();
    symbolTable.clear();
    machineCode.clear();

    std::string line;

    // 2. Read the file line by line
    while (std::getline(inputFile, line)) {
        // Pre-process the line (remove comments and whitespace)
        std::string cleaned = cleanLine(line);

        // 3. Store valid lines
        // Only add the line if it is not empty after cleaning
        if (!cleaned.empty()) {
            sourceLines.push_back(cleaned);
        }
    }

    inputFile.close();

    // 4. Verification (Optional but recommended)
    std::cout << "Successfully loaded " << sourceLines.size() << " instructions." << std::endl;

    // Warning if program exceeds the Baby's memory limit (32 lines) [cite: 145]
        if (sourceLines.size() > 32) {
            std::cerr << "Warning: Source code exceeds the 32-line memory limit of the Manchester Baby." << std::endl;
        }

    return true;
}

/**
 * Helper: toBinary
 * Converts an integer to a 32-bit binary string in Little-Endian format.
 * Manchester Baby reads bits from left (LSB) to right (MSB).
 */
std::string Assembler::toString(int number) {
    // 1. Convert to standard Big-Endian binary (32 bits)
    // std::bitset automatically handles 2's complement for negative numbers
    std::bitset<32> binaryBits(number);

    // 2. Convert to string
    std::string binaryString = binaryBits.to_string();

    // 3. Reverse the string to make it Little-Endian
    // Example: 1 (Standard: ...001) -> Baby: 100...
    std::reverse(binaryString.begin(), binaryString.end());

    return binaryString;
}

/**
 * Pass 1: Symbol Table Construction
 * Scans for labels (ending with ':') and maps them to line numbers.
 */
bool Assembler::pass1() {
    int count = 0;

    for (const std::string& line : sourceLines) {
        std::stringstream ss(line);
        std::string token;

        // Read the first token to check for a label
        if (ss >> token) {
            // Check if the token ends with a colon (e.g., "START:")
            if (token.back() == ':') {
                // Remove the colon to get the clean label name
                std::string labelName = token.substr(0, token.length() - 1);

                // Error Check: Duplicate Label
                if (symbolTable.count(labelName)) {
                    std::cerr << "Error (Line " << count << "): Duplicate label '"
                        << labelName << "' defined." << std::endl;
                    return false;
                }

                // Store in Symbol Table: Name -> Address (Line Number)
                symbolTable[labelName] = count;
            }
        }
        count++;
    }

    return true;
}

/**
 * Pass 2: Code Generation
 * Translates assembly into binary machine code.
 */
bool Assembler::pass2() {
    int lineCounter = 0;

    for (const std::string& line : sourceLines) {
        std::stringstream ss(line);
        std::string token;
        std::string opcodeStr;
        std::string operandStr;

        // Step 1: Parse Line Components

        ss >> token; // Read first token

        // Check if the first token is a Label (end with ':')
        // If it is, skip it and read the next token as the Opcode
        if (token.back() == ':') {
            if (!(ss >> token)) {
                // Handle case: "LABEL:" on a line by itself (valid but handled as 0 instruction)
                // But normally implies an instruction follows or it's a marker. 
                // For simplicity, we treat empty label lines as NO-OP (000...) or error.
                // Here we assume well-formed assembly usually has content.
            }
        }

        opcodeStr = token; // The current token is the Opcode (e.g., "LDN", "VAR")
        ss >> operandStr;  // Try to read the Operand (e.g., "NUM", "10")

        // Step 2: Translate Components

        int finalValue = 0;

        // Case 1: Handle Variables (Pseudo-instruction VAR)
        // This is data, not an instruction. We just store the value directly.
        if (opcodeStr == "VAR") {
            try {
                // Operand is a number (e.g., VAR 10)
                finalValue = std::stoi(operandStr);
            }
            catch (...) {
                // Operand is a symbol? (Rare for VAR, but possible)
                std::cerr << "Error (Line " << lineCounter << "): VAR expects a numeric constant." << std::endl;
                return false;
            }
        }
        // Case 2: Handle Standard Instructions
        else {
            // 2.1 Find Opcode ID
            if (OPCODE_MAP.find(opcodeStr) == OPCODE_MAP.end()) {
                std::cerr << "Error (Line " << lineCounter << "): Invalid Opcode '" << opcodeStr << "'." << std::endl;
                return false;
            }
            int opcodeID = OPCODE_MAP.at(opcodeStr);

            // 2.2 Resolve Operand (Address)
            int operandAddress = 0;

            if (!operandStr.empty()) {
                // Check if operand is a number (Absolute Address)
                if (std::isdigit(operandStr[0])) {
                    operandAddress = std::stoi(operandStr);
                }
                // Check if operand is a Symbol (Label/Variable)
                else if (symbolTable.count(operandStr)) {
                    operandAddress = symbolTable.at(operandStr);
                }
                else {
                    std::cerr << "Error (Line " << lineCounter << "): Undefined symbol '" << operandStr << "'." << std::endl;
                    return false;
                }
            }

            // 2.3 Assemble the Instruction Integer
            // Format: [Opcode (bits 13-15)] ... [Operand (bits 0-4)]
            // Note: Verify these bit positions with your specific lecture notes.
            // Based on Slide 16 decoding logic:
            finalValue = (opcodeID << 13) | operandAddress;
        }

        // Step 3: Convert to Binary String

        std::string binaryLine = toString(finalValue);
        machineCode.push_back(binaryLine);

        lineCounter++;
    }

    // Fill remaining memory with zeros if program < 32 lines
    while (machineCode.size() < 32) {
        machineCode.push_back(toString(0));
    }

    return true;
}

/**
 * Save Machine Code to File
 * Writes the 32-line binary strings to a text file.
 */
void Assembler::saveFile(const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not create output file '" << filename << "'." << std::endl;
        return;
    }

    // Iterate through the generated machine code and write to file
    for (const std::string& line : machineCode) {
        outputFile << line << std::endl;
    }

    outputFile.close();
    std::cout << "Success: Machine code saved to '" << filename << "'." << std::endl;
}

/**
 * Debug: Print Symbol Table
 * Displays all detected labels and their corresponding memory addresses.
 */
void Assembler::printSymbolTable() {
    std::cout << "\n--- Symbol Table ---" << std::endl;
    std::cout << "Label\t\tAddress" << std::endl;
    std::cout << "--------------------" << std::endl;

    if (symbolTable.empty()) {
        std::cout << "(Empty)" << std::endl;
    }
    else {
        for (const auto& pair : symbolTable) {
            // pair.first is the Label Name, pair.second is the Line Number
            std::cout << pair.first << "\t\t" << pair.second << std::endl;
        }
    }
    std::cout << "--------------------\n" << std::endl;
}