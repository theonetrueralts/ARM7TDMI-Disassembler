#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace totr::Disassembler {
	enum class ArmMode { ARM, THUMB };
	enum class ModeEvent { None, BX, ExceptionReturn };
	
	struct InstructionData {
		std::uint32_t pc;
		std::uint32_t instruction;
		std::string mnemonic;
		bool is_thumb;
		uint8_t size;
		bool is_valid;
		ModeEvent mode_event;
	};

	std::string get_register_name(std::uint8_t reg, bool use_alias = true);
	std::string get_cond_suffix(std::uint8_t cond);
	std::string print_register_list(std::uint32_t register_list, int length);
	std::uint32_t read_word32_at(std::span<const std::uint8_t> rom, std::uint32_t pc);
} // totr::Disassembler
