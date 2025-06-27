#pragma once
#include <string>
#include <cstdint>

namespace totr::Disassembler {
	std::string get_register_name(std::uint8_t reg, bool use_alias = true);
	std::string print_register_list(std::uint32_t register_list, int length);
} // totr::Disassembler
