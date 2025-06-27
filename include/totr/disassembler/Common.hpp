#pragma once
#include <string>
#include <cstdint>

namespace totr::Disassembler {
	std::string get_register_name(std::uint8_t reg, bool use_alias = true);
} // totr::Disassembler
