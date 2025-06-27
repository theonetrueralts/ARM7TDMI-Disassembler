#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace totr::Disassembler {
	struct InstructionDataThumb {
		std::uint32_t pc;
		std::uint16_t instruction;
		std::string mnemonic;
	};

	class ThumbDisasm {
	public:
		explicit ThumbDisasm(bool print_literals_hex = true) : m_print_literals_hex(print_literals_hex) {}

		InstructionDataThumb decode(std::uint32_t pc, const std::uint32_t instr) const;
		
		void toggle_print_literals_hex() { m_print_literals_hex = !m_print_literals_hex; }
	private:
		bool m_print_literals_hex;

		// Utility Methods
		std::string print_literal(uint32_t v, bool prefix_hash = true) const;

		// Format Dispatchers
		InstructionDataThumb thumb_dispatcher(std::uint32_t pc, std::uint16_t instr) const;

		// Format Decoders
		InstructionDataThumb dis_move_shifted_reg(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_add_sub(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_mov_cmp_add_sub_imm(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_alu_ops(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_hi_reg_ops_bx(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_pc_rel_load(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_load_store_reg_off(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_load_store_sign_ext(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_load_store_imm_off(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_load_store_halfword(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_sp_rel_load_store(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_load_address(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_add_off_to_sp(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_push_pop_reg(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_multi_load_store(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_cond_branch(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_swi(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_uncond_branch(std::uint32_t pc, const std::uint16_t instr) const;
		
		InstructionDataThumb dis_long_branch_link(std::uint32_t pc, const std::uint16_t instr) const;
	};
} // totr::Disassembler
