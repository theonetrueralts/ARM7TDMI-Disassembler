#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

#include "Common.hpp"

namespace totr::Disassembler {
    std::unordered_map<std::uint32_t, ArmMode> load_overrides(const std::string& filepath);
    std::vector<uint8_t> load_rom(const std::string& path);
} // totr::Disassembler
