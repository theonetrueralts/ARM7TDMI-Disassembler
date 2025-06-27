#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace totr::Disassembler {
	struct InstructionDataArm {
		std::uint32_t pc;
		std::uint32_t instruction;
		std::string mnemonic;
	};

	class ArmDisasm {
	public:
		explicit ArmDisasm(bool print_literals_hex = true) : m_print_literals_hex(print_literals_hex) {}

		InstructionDataArm decode(std::uint32_t pc, const std::uint32_t instr) const;
		
		void toggle_print_literals_hex() { m_print_literals_hex = !m_print_literals_hex; }
	private:
		bool m_print_literals_hex;

		// Utility Methods
		std::string print_literal(uint32_t v, bool prefix_hash = true) const;
		
		std::string get_cond_suffix(std::uint8_t cond) const;
		
		std::uint32_t rotr32(std::uint32_t value, std::uint32_t rot) const;
		
		std::string build_shift_op(const std::uint32_t instr) const;

		// Format Dispatchers
		InstructionDataArm dispatch_arm(std::uint32_t pc, const std::uint32_t instr) const;

		InstructionDataArm dispatch_data_proc_psr(std::uint32_t pc, const std::uint32_t instr) const;

		// Format Decoders
		InstructionDataArm dis_branch_exchange(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_branch(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_data_proc(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_psr_trans_MRS(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_psr_trans_MSR_reg(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_psr_trans_MSR_imm(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_mul_mla(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_mul_mla_long(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_single_data_trans(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_halfword_data_trans_reg(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_halfword_data_trans_imm(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_block_data_trans(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_single_data_swap(std::uint32_t pc, const std::uint32_t instr) const;
		
		InstructionDataArm dis_swi(std::uint32_t pc, const std::uint32_t instr) const;
	};
} // totr::Disassembler
