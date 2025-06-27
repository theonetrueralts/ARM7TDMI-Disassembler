#pragma once
#include <string>
#include <cstdint>

namespace totr::Disassembler {
	struct InstructionData {
		std::uint32_t pc;
		std::uint32_t instruction;
		std::string mnemonic;
		bool is_thumb;
		int advance_by;
	};

	std::string get_register_name(std::uint8_t reg, bool use_alias = true);
	std::string get_cond_suffix(std::uint8_t cond);
	std::string print_register_list(std::uint32_t register_list, int length);
} // totr::Disassembler
