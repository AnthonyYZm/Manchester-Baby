/**
 * @file Assembler.h
 * @brief Header file for the Manchester Baby Computer Assembler
 *
 * This file contains the class definition for the Assembler that translates
 * assembly language into machine code for the Manchester Baby Computer.
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <bitset>
#include <algorithm>

 /**
  * @brief Manchester Baby instruction set mapping
  *
  * Maps assembly mnemonics to their corresponding opcodes.
  */
const std::map<std::string, int> OPCODE_MAP = {
    {"JMP", 0},  ///< Set CI = S (Jump to address)
    {"JRP", 1},  ///< CI = CI + S (Relative jump)
    {"LDN", 2},  ///< A = -S (Load negative)
    {"STO", 3},  ///< S = A (Store accumulator)
    {"SUB", 4},  ///< A = A - S (Subtract)
    {"SUB5", 5}, ///< Alternative opcode for SUB
    {"CMP", 6},  ///< If A < 0, skip next instruction
    {"STP", 7},  ///< Stop execution
    {"ADO", 8},  ///< Add (Extended)
    {"MPY", 9},  ///< Multiply (Extended)
    {"LDI", 10}, ///< Load Immediate (Extended)
    {"IND", 11}, ///< Indirect Load (Extended)
    {"LDIX", 12}, ///< Load Immediate Index (Extended)
    {"SUBX", 13}, ///< Subtract Indexed (Extended)
    {"VAR", -1}  ///< Pseudo-instruction for variable declaration
};
/**
 * @class Assembler
 * @brief Manchester Baby Computer Assembler
 *
 * This class translates assembly language programs into machine code
 * for the Manchester Small-Scale Experimental Machine (The Baby).
 * It performs two-pass assembly with symbol table generation.
 */
class Assembler {
private:
    std::map<std::string, int> symbolTable;
    std::vector<std::string> sourceLines;
    std::vector<std::string> machineCode;

    std::string cleanLine(const std::string& line);
    std::string intToBinaryString(int number);
    bool parseLine(const std::string& line, std::string& label,
        std::string& opcode, std::string& operand);

public:
    Assembler();
    bool loadSource(const std::string& filename);
    bool buildSymbolTable();
    bool generateMachineCode();
    void saveOutput(const std::string& filename);
    void displaySymbolTable() const;
    size_t getSourceLineCount() const { return sourceLines.size(); }
    size_t getMachineCodeLineCount() const { return machineCode.size(); }
    bool isAssemblyComplete() const { return !machineCode.empty(); }
};

#endif // ASSEMBLER_H