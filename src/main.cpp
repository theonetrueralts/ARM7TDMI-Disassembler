#include <iostream>
#include <format>
#include <string>
#include <filesystem>
#include <vector>
#include <span>
#include <unordered_map>
#include <optional>
#include <fstream>

#include <totr/disassembler/Common.hpp>
#include <totr/disassembler/ArmDisasm.hpp>
#include <totr/disassembler/ThumbDisasm.hpp>
#include <totr/disassembler/FileUtil.hpp>
#include <totr/disassembler/InstructionProbe.hpp>

namespace td = totr::Disassembler;

void print_instruction(std::ostream& out, td::InstructionData data, td::ArmMode mode, bool ambiguous, bool mode_switched, bool did_mode_override) {
    out << std::format("#0x{:08X}", data.pc) << " : ";

    out << std::format("#0x{:08X}", data.instruction);
    
    out << " : " << data.mnemonic;
    if (ambiguous) {
        if (mode == td::ArmMode::ARM) out << " ; May switch to THUMB.";
        else out << " ; May switch to ARM.";
    }
    else if (did_mode_override) {
        if (mode == td::ArmMode::ARM) out << " ; Mode force switched to ARM.";
        else out << " ; Mode force switched to THUMB.";
    }
    else if (mode_switched) {
        if (mode == td::ArmMode::ARM) out << " ; Mode switch to ARM detected.";
        else out << "; Mode switch to THUMB detected.";
    }

    out << "\n";
}

void usage(const char* exe) {
    std::cout
        << "Usage: " << exe << " <rom file> [options]\n\n"
        << "Options:\n"
        << "  -r, --override <file>  Path to mode-override table\n"
        << "  -o, --out <file>       Write disassembly to <file> instead of stdout\n"
        << "  -d, --dec              Print immediates in decimal (default: hex)\n"
        << "\nExamples:\n"
        << "  " << exe << " example.rom\n"
        << "  " << exe << " example.rom --dec\n"
        << "  " << exe << " example.rom -r overrides.txt -o dump.txt\n";
}

int main(int argc, char* argv[]) {
    // CLI Init
    if (argc < 2) { usage(argv[0]); return 1; }

    bool print_literals_hex = true;
    std::filesystem::path rom_path = argv[1];
    std::optional<std::filesystem::path> out_path;
    std::optional<std::filesystem::path> override_path;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-d" || arg == "--dec") {
            print_literals_hex = false;
        }
        else if (arg == "-o" || arg == "--out") {
            if (i + 1 == argc) { usage(argv[0]); return 1; }
            out_path = argv[++i];
        }
        else if (arg == "-r" || arg == "--override") {
            if (++i == argc) { usage(argv[0]); return 1; }
            override_path = argv[i];
        }
        else {
            std::cerr << "Unknown option: " << arg << '\n';
            usage(argv[0]); return 1;
        }
    }

    std::ofstream outfile;
    std::ostream* out = &std::cout;
    if (out_path) {
        outfile.open(*out_path, std::ios::out | std::ios::trunc);
        if (!outfile) {
            std::cerr << "Cannot write to " << *out_path << "\n";
            return 2;
        }
        out = &outfile;
    }

    auto opcodes = td::load_rom(rom_path.string());
    std::unordered_map<std::uint32_t, td::ArmMode> mode_override_table;
    if (override_path) mode_override_table = td::load_overrides(override_path->string());

    // Main loop
    std::span<const uint8_t> rom{ opcodes };
    td::ArmDisasm d_arm{ print_literals_hex };
    td::ThumbDisasm d_thumb{ print_literals_hex };

    int pc = 0;
    td::ArmMode mode = td::ArmMode::ARM;
    bool did_mode_override = false;
    bool mode_switched = false;mode_switched = false;
    bool ambiguous = false;

    std::uint32_t instr = 0;
    td::InstructionData data;

    *out << "    Addr    :    Instr    : Mnemonic\n";
    *out << "--------------------------------------\n";
    
    while (pc < rom.size()) {
        ambiguous = false;
        mode_switched = false;
        did_mode_override = false;

        instr = td::read_word32_at(rom, pc);

        if (mode == td::ArmMode::ARM) data = d_arm.decode(pc, instr);
        else data = d_thumb.decode(pc, instr);

        pc += data.size;

        if (data.mode_event == td::ModeEvent::BX) {
            ambiguous = true;

            td::ModeGuess guess = probe_mode(rom, pc, d_arm, d_thumb);

            switch (guess) {
                case td::ModeGuess::ARM:
                    mode = td::ArmMode::ARM;
                    mode_switched = true;
                    ambiguous = false;
                    break;
                case td::ModeGuess::THUMB:
                    mode = td::ArmMode::THUMB;
                    mode_switched = true;
                    ambiguous = false;
                    break;
                default: break;
            }
        }
        else if (data.mode_event == td::ModeEvent::ExceptionReturn) {
            mode = td::ArmMode::ARM;
        }
        
        if (auto it = mode_override_table.find(pc); it != mode_override_table.end()) {
            mode = it->second;
            did_mode_override = true;
            ambiguous = false;
        }

        print_instruction(*out, data, mode, ambiguous, mode_switched, did_mode_override);
    }

    return 0;
}
