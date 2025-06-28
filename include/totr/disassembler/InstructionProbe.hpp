#pragma once

#include <cstdint>
#include <span>

#include "Common.hpp"
#include "ArmDisasm.hpp"
#include "ThumbDisasm.hpp"

namespace totr::Disassembler {
    enum class ModeGuess { ARM, THUMB, BOTH, NEITHER };

    ModeGuess probe_mode(std::span<const std::uint8_t> rom, std::uint32_t pc, const ArmDisasm& arm, const ThumbDisasm& thumb);
} // totr::Disassembler
