#include "Assembler.h"
#include <sstream>
#include <cctype>
#include <cstdint>
#include <algorithm> // Added for std::transform and std::reverse
#include <bitset>    // Added for std::bitset
#include <iostream>  // Added for std::cerr and std::cout
#include <fstream>   // Added for std::ifstream and std::ofstream
#include <stdexcept> // Added for std::stoi exceptions

// Constants for instruction encoding based on a 16-bit or 32-bit machine word.
// Note: This instruction format suggests a custom architecture (e.g., a simplified Baby Machine).
constexpr uint32_t OPCODE_SHIFT = 13;           ///< Bit position shift for opcode in instruction word (Bits 13-15)
constexpr uint32_t OPERAND_MASK_STANDARD = 0x1F; ///< 5-bit mask for operand field (Bits 0-4)
constexpr uint32_t OPCODE_MASK_STANDARD = 0x7;   ///< 3-bit mask for opcode field

/**
 * @brief Default constructor for the Assembler class.
 *
 * Initializes all internal member containers (symbol table, source lines,
 * machine code) to an empty state and reserves memory for typical program
 * sizes (32 lines, matching the architecture's memory limit).
 */
Assembler::Assembler() {
    symbolTable.clear();
    sourceLines.clear();
    machineCode.clear();

    // Reserve memory based on the target architecture's 32-word limit.
    sourceLines.reserve(32);
    machineCode.reserve(32);
}

/**
 * @brief Cleans a single line of assembly source code.
 *
 * This function performs three main tasks:
 * 1. Removes comments (everything after a semicolon ';').
 * 2. Trims leading and trailing whitespace.
 * 3. Converts the entire line to uppercase for case-insensitive processing.
 *
 * @param line The raw input line read from the source file.
 * @return std::string The cleaned line without comments or extra whitespace, in uppercase.
 */
std::string Assembler::cleanLine(const std::string& line) {
    std::string clean = line;

    // Remove comments (everything after semicolon)
    size_t commentPos = clean.find(';');
    if (commentPos != std::string::npos) {
        clean = clean.substr(0, commentPos);
    }

    // Trim trailing whitespace
    // Uses C++11 reverse iterators to find the first non-space character from the end.
    clean.erase(std::find_if(clean.rbegin(), clean.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), clean.end());

    // Trim leading whitespace
    // Finds the first non-space character from the beginning and erases up to that point.
    clean.erase(clean.begin(), std::find_if(clean.begin(), clean.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));

    // Convert to uppercase for case-insensitive assembly
    std::transform(clean.begin(), clean.end(), clean.begin(), ::toupper);
    return clean;
}

/**
 * @brief Converts a 32-bit integer to a reversed binary string representation.
 *
 * The output binary string is **reversed** (Least Significant Bit (LSB) at index 0)
 * to match the specific format required by the machine code output file/simulator.
 *
 * @param number The integer value (representing the 32-bit instruction or data word) to convert.
 * @return std::string A 32-character binary string with the LSB first (reversed).
 */
std::string Assembler::intToBinaryString(int number) {
    // Treat the input integer as an unsigned 32-bit value for conversion
    std::bitset<32> binaryBits(static_cast<uint32_t>(number));

    // Convert to string (MSB at index 0)
    std::string binaryString = binaryBits.to_string();

    // Reverse the string (LSB at index 0) to match the required output format.
    std::reverse(binaryString.begin(), binaryString.end());
    return binaryString;
}

/**
 * @brief Parses a cleaned assembly line into its three potential components.
 *
 * Splits a line into optional label, opcode, and optional operand components.
 * Labels are identified by a trailing colon. It enforces a maximum of three
 * tokens (label: opcode operand).
 *
 * @param line The cleaned assembly line (no comments, trimmed, uppercase).
 * @param label Output parameter for the parsed label (empty string if none).
 * @param opcode Output parameter for the parsed opcode.
 * @param operand Output parameter for the parsed operand (empty string if none).
 * @return bool True if parsing was syntactically successful (0 to 3 tokens), false on error (e.g., > 3 tokens).
 */
bool Assembler::parseLine(const std::string& line, std::string& label,
    std::string& opcode, std::string& operand) {

    std::stringstream ss(line);
    std::string token;

    // Clear output parameters before use
    label.clear();
    opcode.clear();
    operand.clear();

    // Get first token (potential label or opcode)
    if (!(ss >> token)) {
        return false; // Empty line after cleaning/trimming
    }

    // Check if first token is a label (ends with colon)
    if (token.back() == ':') {
        label = token.substr(0, token.length() - 1);
        if (!(ss >> token)) {
            // Line contained only a label (e.g., "START:"). This is valid.
            return true;
        }
    }

    opcode = token;

    // Get operand if present
    if (ss >> token) {
        operand = token;
        // Reject lines with more than 3 tokens (e.g., LABEL: OPCODE OPERAND EXTRA)
        if (ss >> token) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Loads assembly source code from a file.
 *
 * Reads the specified file line by line, cleans each line using `cleanLine()`,
 * and stores valid non-empty lines in the `sourceLines` vector.
 * Also handles error checking for file access and warns if the program exceeds
 * the 32-line memory limit.
 *
 * @param filename Path to the source assembly file.
 * @return bool True if the file was opened and loaded successfully, false on error.
 */
bool Assembler::loadSource(const std::string& filename) {
    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open source file '" << filename << "'." << std::endl;
        return false;
    }

    // Clear previous assembly state before loading a new file
    sourceLines.clear();
    symbolTable.clear();
    machineCode.clear();

    std::string line;
    // int lineNumber = 0; // Not used; kept for context/debugging, but removed as it's not used.

    // Read and clean each line
    while (std::getline(inputFile, line)) {
        // lineNumber++; // line number is only used for error reporting, which happens on the next line.
        std::string cleaned = cleanLine(line);

        if (!cleaned.empty()) {
            sourceLines.push_back(cleaned);
        }
    }

    inputFile.close();

    std::cout << "Successfully loaded " << sourceLines.size() << " lines from '"
        << filename << "'." << std::endl;

    // Warn if exceeding memory capacity
    if (sourceLines.size() > 32) {
        std::cerr << "Warning: Source code exceeds 32-line memory limit. "
            << "Only first 32 lines will be processed." << std::endl;
    }

    return true;
}

/**
 * @brief Performs the first pass of assembly to build the symbol table.
 *
 * This function iterates through the cleaned source code lines to identify
 * all labels (symbols) and assign them their correct memory addresses.
 * It manages the Program Counter (`currentAddress`) to track memory words,
 * independently of line numbers. It also validates basic instruction/label syntax.
 *
 * The logic is updated to allow the `VAR` pseudo-instruction to exist without a label
 * (e.g., for memory padding), but it must always have an operand.
 *
 * @note Assumes the `OPCODE_MAP` (not visible in this file) is available and contains `VAR` mapped to `-1`.
 *
 * @return bool Returns true if the symbol table is successfully built, false on error
 * (e.g., duplicate label, syntax error, missing mandatory operand).
 */
bool Assembler::buildSymbolTable() {
    symbolTable.clear();

    // Initialize the Program Counter (PC), which tracks the current memory address (0-31).
    uint32_t currentAddress = 0;

    // Iterate through all cleaned source lines to perform the first pass.
    for (size_t lineNum = 0; lineNum < sourceLines.size(); lineNum++) {
        std::string label, opcode, operand;
        const std::string& rawLine = sourceLines[lineNum];

        // Skip processing if the current address has exceeded the 32-word limit.
        // This stops processing instructions but still checks for label errors in the remaining lines.
        if (currentAddress >= 32) {
            // Note: Labels in lines beyond the 32-word limit will still be processed 
            // but will be assigned an address >= 32. This might be a bug 
            // if the simulator strictly enforces 0-31 addresses.
            // For this design, we stop PC incrementing beyond 32 but continue to parse for labels.
            // A cleaner approach would be to return true and let generateMachineCode handle the limit.
        }

        // Attempt to parse the line into its components.
        if (!parseLine(rawLine, label, opcode, operand)) {
            // Note: cleanLine ensures rawLine is not empty or comment-only here, 
            // so a parse failure means >3 tokens.
            std::cerr << "Syntax error at line " << (lineNum + 1) << ": Unparsable instruction or format (too many tokens)." << std::endl;
            return false;
        }

        // 1. Label Registration: If a label is present, add it to the symbol table.
        if (!label.empty()) {
            if (symbolTable.count(label)) {
                std::cerr << "Error at line " << (lineNum + 1)
                    << ": Duplicate label '" << label << "'" << std::endl;
                return false;
            }
            // Use currentAddress (Program Counter) as the address.
            symbolTable[label] = static_cast<int>(currentAddress);
        }

        // 2. Opcode Validation and Instruction/VAR Handling.
        auto opcodeIt = OPCODE_MAP.find(opcode);

        if (opcodeIt != OPCODE_MAP.end()) {
            int opcodeValue = opcodeIt->second;

            // Handle VAR pseudo-instruction (Opcode value -1).
            if (opcodeValue == -1) {
                // VAR must always have a data value (operand).
                if (operand.empty()) {
                    std::cerr << "Error at line " << (lineNum + 1) << ": VAR pseudo-instruction requires a data value (operand)." << std::endl;
                    return false;
                }
            }
            // Check for standard instructions that require an operand (JMP, JRP, LDN, STO, SUB, etc.)
            // Exclude CMP (6) and STP (7), which are zero-operand instructions.
            else if (opcodeValue != 6 && opcodeValue != 7) {
                if (operand.empty()) {
                    std::cerr << "Error at line " << (lineNum + 1) << ": Instruction '" << opcode << "' requires an operand." << std::endl;
                    return false;
                }
            }

            // 3. Address Increment: A valid instruction or VAR declaration occupies one memory word.
            currentAddress++;

        }
        else if (!opcode.empty()) {
            // Unrecognized opcode encountered.
            std::cerr << "Error at line " << (lineNum + 1) << ": Unknown opcode '" << opcode << "'" << std::endl;
            return false;
        }
    }

    return true;
}

/**
 * @brief Generates machine code from parsed assembly.
 *
 * Performs the second pass over the source code. It translates each instruction
 * or `VAR` directive into its 32-bit binary representation.
 * It resolves symbols (labels) using the pre-built `symbolTable` and encodes
 * instructions based on the architectural specification:
 * `[3-bit opcode << 13] | [5-bit operand]`.
 *
 * @return bool True if machine code was generated successfully for all lines, false on error (e.g., undefined symbol).
 */
bool Assembler::generateMachineCode() {
    machineCode.clear();

    // Iterate up to the maximum memory capacity (32 words)
    for (size_t lineNum = 0; lineNum < sourceLines.size() && lineNum < 32; lineNum++) {
        std::string label, opcode, operand;
        uint32_t instructionValue = 0; // The 32-bit word to be encoded.

        // Re-parse the line for components
        if (!parseLine(sourceLines[lineNum], label, opcode, operand)) {
            // Should theoretically not happen if first pass succeeded, but kept for robustness.
            std::cerr << "Internal Error: Parse failure during second pass at line " << (lineNum + 1) << std::endl;
            return false;
        }

        // Skip lines that had only a label and no instruction/VAR (which were valid in parseLine, but don't generate code)
        if (opcode.empty()) {
            continue;
        }

        // --- Handle VAR directive (data storage) ---
        if (opcode == "VAR") {
            if (operand.empty()) {
                // Should have been caught in first pass, but check again.
                std::cerr << "Error at line " << (lineNum + 1)
                    << ": VAR requires an operand" << std::endl;
                return false;
            }

            // Parse operand as integer data or symbol address
            try {
                // Attempt to parse as a direct integer value
                instructionValue = std::stoi(operand);
            }
            catch (const std::exception& e) {
                // If not an integer, check if it's a known symbol
                if (symbolTable.find(operand) != symbolTable.end()) {
                    // Store the symbol's address as the data value
                    instructionValue = symbolTable[operand];
                }
                else {
                    std::cerr << "Error at line " << (lineNum + 1)
                        << ": Invalid operand for VAR '" << operand << "'. Not an integer or defined symbol." << std::endl;
                    return false;
                }
            }
        }
        // --- Handle regular instructions ---
        else {
            // Look up opcode in instruction set
            auto opcodeIt = OPCODE_MAP.find(opcode);
            // First pass should have caught an unknown opcode, but check again.
            if (opcodeIt == OPCODE_MAP.end()) {
                std::cerr << "Error at line " << (lineNum + 1)
                    << ": Unknown opcode '" << opcode << "'" << std::endl;
                return false;
            }

            int opcodeValue = opcodeIt->second;
            uint32_t operandValue = 0;
            uint32_t resolvedAddress = 0; // Stores the target address (absolute) or immediate value.

            // Resolve operand (immediate or symbolic) if one is expected/present
            if (!operand.empty()) {
                // If operand starts with a digit, treat as an immediate (absolute) address.
                if (std::isdigit(static_cast<unsigned char>(operand[0]))) {
                    try {
                        resolvedAddress = std::stoul(operand);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error at line " << (lineNum + 1) << ": Invalid numeric operand '" << operand << "'" << std::endl;
                        return false;
                    }
                }
                // Otherwise, treat as a symbolic address (label).
                else {
                    auto symbolIt = symbolTable.find(operand);
                    if (symbolIt == symbolTable.end()) {
                        std::cerr << "Error at line " << (lineNum + 1)
                            << ": Undefined symbol '" << operand << "'" << std::endl;
                        return false;
                    }
                    resolvedAddress = symbolIt->second;
                }

                if (opcodeValue == 1) { // JRP (Relative Jump)
                    // JRP offset S = TargetAddress - (CurrentAddress + 1)

                    // Convert to signed 32-bit integers for correct subtraction and two's complement calculation.
                    int32_t targetAddressSigned = static_cast<int32_t>(resolvedAddress);
                    int32_t currentAddressSigned = static_cast<int32_t>(lineNum);

                    // Calculate the 32-bit signed offset
                    int32_t relativeOffset_32bit = targetAddressSigned - (currentAddressSigned + 1);

                    // The 5-bit operand field stores this relative offset in two's complement.
                    // We take the 5 LSBs of the 32-bit signed offset.
                    operandValue = static_cast<uint32_t>(relativeOffset_32bit) & OPERAND_MASK_STANDARD;
                }
                else {
                    // For all other instructions (JMP, LDN, STO, SUB, etc.), use the absolute address.
                    operandValue = resolvedAddress;

                    // Check if the operand address exceeds the 5-bit limit (31).
                    if (operandValue > OPERAND_MASK_STANDARD) {
                        std::cerr << "Error at line " << (lineNum + 1)
                            << ": Operand address (" << operandValue
                            << ") exceeds 5-bit limit (31)." << std::endl;
                        return false;
                    }
                }
            }

            // Encode instruction: [3-bit opcode << 13] | [5-bit operand]
            instructionValue = ((static_cast<uint32_t>(opcodeValue) & OPCODE_MASK_STANDARD) << OPCODE_SHIFT) |
                (operandValue & OPERAND_MASK_STANDARD);
        }

        // Convert the 32-bit word to the LSB-first binary string and store it.
        machineCode.push_back(intToBinaryString(instructionValue));
    }

    // Pad with zeros to fill the entire 32-word memory (0-31 addresses)
    while (machineCode.size() < 32) {
        machineCode.push_back(intToBinaryString(0));
    }

    return true;
}

/**
 * @brief Saves the generated machine code to a file.
 *
 * Writes each 32-bit binary string (representing one memory word) as a separate
 * line in the output file. The format is LSB-first as defined by `intToBinaryString()`.
 *
 * @param filename Path for the output machine code file.
 */
void Assembler::saveOutput(const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not create output file '" << filename << "'" << std::endl;
        return;
    }

    // Write each 32-bit word (as an LSB-first binary string) to a new line.
    for (const std::string& line : machineCode) {
        outputFile << line << std::endl;
    }

    outputFile.close();
    std::cout << "Machine code successfully saved to '" << filename << "'" << std::endl;
}

/**
 * @brief Displays the contents of the built symbol table.
 *
 * Prints all defined labels and their corresponding memory addresses
 * in a formatted, columnar table.
 */
void Assembler::displaySymbolTable() const {
    std::cout << "\n--- Symbol Table ---" << std::endl;
    std::cout << "Label\t\tAddress" << std::endl;
    std::cout << "--------------------" << std::endl;

    if (symbolTable.empty()) {
        std::cout << "(Empty)" << std::endl;
    }
    else {
        for (const auto& entry : symbolTable) {
            std::cout << entry.first << "\t\t" << entry.second << std::endl;
        }
    }
    std::cout << "--------------------" << std::endl;
}