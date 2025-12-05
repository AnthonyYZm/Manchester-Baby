#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <bitset>
#include <algorithm>

// 定义 Manchester Baby 的机器指令集
// 映射助记符到功能码 (Opcode 0-7)
// 参考 Slide 15 [cite: 146-155]
const std::map<std::string, int> OPCODE_MAP = {
    {"JMP", 0}, // Set CI = S
    {"JRP", 1}, // CI = CI + S
    {"LDN", 2}, // A = -S
    {"STO", 3}, // S = A
    {"SUB", 4}, // A = A - S
    {"SUB5", 5},// (SUB 的变体，有时作业会用到，标准是4)
    {"CMP", 6}, // If A < 0, Skip next
    {"STP", 7}, // Stop
    {"VAR", -1} // 伪指令，用于定义变量数据 (Extension)
};

class Assembler {
private:
    // --- 核心数据结构 ---

    // 符号表：存储 Label/Variable 名称及其对应的内存地址 (Line Number 0-31)
    // [cite: 280, 366]
    std::map<std::string, int> symbolTable;

    // 存储清洗后的源代码行（去除了空行和注释）
    std::vector<std::string> sourceLines;

    // 最终生成的机器码：32行，每行32个字符的 '0' 或 '1'
    // [cite: 290-294]
    std::vector<std::string> machineCode;

    // --- 辅助函数 (Helper Functions) ---

    // 去除字符串首尾空格和注释 (处理 ';' 之后的内容) [cite: 369]
    std::string cleanLine(const std::string& line);

    // 将整数转换为 32 位的二进制字符串 (Little-Endian 格式)
    // 注意：Baby 的最低有效位 (LSB) 在左边 [cite: 64, 175]
    std::string toString(int number);

    // 解析一行代码，提取 Label, Opcode, 和 Operand
    // 返回 true 表示解析成功，false 表示语法错误
    bool parseLine(const std::string& line, std::string& label, std::string& opcode, std::string& operand);

public:
    // 构造函数
    Assembler();

    // --- 主要功能模块 ---

    // 1. 读取汇编源文件 [cite: 267]
    bool load(const std::string& filename);

    // 2. 第一遍扫描 (Pass 1)：构建符号表
    // 扫描所有 Label 和 Variable，记录它们对应的行号，不生成代码
    // [cite: 280, 396]
    bool pass1();

    // 3. 第二遍扫描 (Pass 2)：生成机器码
    // 再次扫描，查表翻译 Opcode，用符号表解析 Operand
    // [cite: 365]
    bool pass2();

    // 4. 保存机器码文件
    // 输出格式需兼容模拟器 (32行 x 32位) [cite: 268]
    void saveFile(const std::string& filename);

    // 显示调试信息 (可选，用于 Extension: Reporting progress) [cite: 371]
    void printSymbolTable();
};

#endif
