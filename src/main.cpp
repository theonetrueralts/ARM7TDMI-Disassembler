#include <iostream>
#include <cstdint>
#include <vector>
#include <span>
#include <format>

#include <totr/disassembler/ArmDisasm.hpp>
#include <totr/disassembler/ThumbDisasm.hpp>
#include <totr/disassembler/Common.hpp>
#include <unordered_map>

namespace td = totr::Disassembler;

enum class Mode { ARM, THUMB };

void print_instruction(td::InstructionData data, Mode mode, bool did_mode_override, bool ambiguous) {
    std::cout << std::format("#0x{:04X}", data.pc) << " : ";

    std::cout << std::format("#0x{:08X}", data.instruction);
    
    std::cout << " : " << data.mnemonic;
    if (ambiguous) {
        if (mode == Mode::ARM) std::cout << " ; May switch to THUMB.";
        else std::cout << " ; May switch to ARM.";
    }
    else if (did_mode_override) {
        if (mode == Mode::ARM) std::cout << " ; Mode force switched to ARM.";
        else std::cout << " ; Mode force switched to THUMB.";
    }

    std::cout << std::endl;
}

std::uint32_t read_word32_at(std::span<const std::uint8_t> rom, std::uint32_t pc) {
    if (pc + 3 >= rom.size()) {
        return std::uint32_t(rom[pc]) | (std::uint32_t(rom[pc + 1]) << 8);
    }
    return std::uint32_t(rom[pc])          |
        (std::uint32_t(rom[pc + 1]) << 8)  |
        (std::uint32_t(rom[pc + 2]) << 16) |
        (std::uint32_t(rom[pc + 3]) << 24);
}

int main() {
    std::vector <std::uint8_t> opcodes = {
        0x00, 0x00, 0xA0, 0xE3, // 0x00 : MOV R0, #0
        0x00, 0x10, 0xA0, 0xE3, // 0x04 : MOV R1, #0
        0x01, 0x00, 0x80, 0xE2, // 0x08 : ADD R0, R0, #1
        0x02, 0x10, 0x81, 0xE2, // 0x0C : ADD R1, R1, #2
        0x01, 0x30, 0x8F, 0xE2, // 0x10 : ADR R3, #25
        0x13, 0xFF, 0x2F, 0xE1, // 0x14 : BX R3
        0x03, 0x30,             // 0x18 : ADD R0, #3
        0x04, 0x31,             // 0x1A : ADD R1, #4
        0x02, 0xA3,             // 0x1C : ADR R3, #8
        0x18, 0x47,             // 0x1E : BX R3
        0x05, 0x00, 0x80, 0xE2, // 0x20 : ADD R0, R0, #5
        0x06, 0x10, 0x81, 0xE2, // 0x24 : ADD R1, R1, #6
        0xFE, 0xFF, 0xFF, 0xEA  // 0x28 : B #40
    };

    std::unordered_map<std::uint32_t, Mode> mode_override_table = {
        { 0x18, Mode::THUMB },
        { 0x20, Mode::ARM }
    };

    std::span<const uint8_t> rom{ opcodes };

    bool print_literals_hex = true;
    td::ArmDisasm d_arm{ print_literals_hex };
    td::ThumbDisasm d_thumb{ print_literals_hex };

    int pc = 0;

    Mode mode = Mode::ARM;
    bool did_mode_override = false;
    bool ambiguous = false;

    std::uint32_t instr = 0;
    td::InstructionData data;

    std::cout << "  Addr   : Instr   : Mnemonic" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    while (pc < rom.size()) {
        ambiguous = false;
        did_mode_override = false;

        instr = read_word32_at(rom, pc);

        if (mode == Mode::ARM) data = d_arm.decode(pc, instr);
        else data = d_thumb.decode(pc, instr);

        if (data.mode_event == td::ModeEvent::BX) {
            ambiguous = true;
        }
        else if (data.mode_event == td::ModeEvent::ExceptionReturn) {
            mode = Mode::ARM;
        }
        
        pc += data.size;
        if (auto it = mode_override_table.find(pc); it != mode_override_table.end()) {
            mode = it->second;
            did_mode_override = true;
            ambiguous = false;
        }

        print_instruction(data, mode, did_mode_override, ambiguous);
    }

    return 0;
}
