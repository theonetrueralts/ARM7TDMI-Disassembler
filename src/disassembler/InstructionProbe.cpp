#include <span>
#include <cstdint>

#include <totr/disassembler/InstructionProbe.hpp>

namespace td = totr::Disassembler;

td::ModeGuess td::probe_mode(std::span<const uint8_t> rom, uint32_t pc, const td::ArmDisasm& arm, const td::ThumbDisasm& thumb) {
    if (pc >= rom.size()) return ModeGuess::NEITHER;

    uint32_t instr = read_word32_at(rom, pc);
    bool arm_ok = false;
    bool thumb_ok = false;

    InstructionData data = arm.decode(pc, instr);
    arm_ok = data.is_valid;

    if (pc + 1 < rom.size()) {
        InstructionData data = thumb.decode(pc, instr);
        thumb_ok = data.is_valid;
    }

    if (arm_ok && !thumb_ok) return ModeGuess::ARM;
    if (!arm_ok && thumb_ok) return ModeGuess::THUMB;
    if (arm_ok && thumb_ok) return ModeGuess::BOTH;
    return ModeGuess::NEITHER;
}
