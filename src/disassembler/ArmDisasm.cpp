#include <totr/disassembler/ArmDisasm.hpp>
#include <totr/disassembler/Common.hpp>

#include <format>

namespace td = totr::Disassembler;

td::InstructionDataArm td::ArmDisasm::decode(std::uint32_t pc, const std::uint32_t instr) const {
	return dispatch_arm(pc, instr);
}

/* --- Utility Methods --- */
std::string td::ArmDisasm::print_literal(uint32_t v, bool prefix_hash) const {
	if (m_print_literals_hex) {
		if (prefix_hash) return std::format("#0x{:X}", v);
		return std::format("0x{:X}", v);
	}
	if (prefix_hash) return std::format("#{}", v);
	return std::format("{}", v);
}

std::uint32_t td::ArmDisasm::rotr32(std::uint32_t value, std::uint32_t rot) const {
	rot &= 31;
	return (value >> rot) | (value << ((32 - rot) & 31));
}

std::string td::ArmDisasm::build_shift_op(const std::uint32_t instr) const {
	/*
	|...1 .................0|
	|1_0_9_8_7_6_5_4_3_2_1_0|
	|______shift_____|__Rm__|
	*/
	const std::uint8_t shift = (instr >> 4) & 0xFF;     // Bit 11-4
	const std::uint8_t op2_register = instr & 0xF;      // Bit 3-0
	const std::uint8_t shift_type = (shift & 0x6) >> 1; // Bit 6-5
	const bool shift_by_reg = shift & 0x1;              // Bit 4

	std::string mnemonic;
	if (shift_by_reg) {
		/*
		|...1 .........0|
		|1_0_9_8_7_6_5_4|
		|__shift__|_ST|0|
		*/
		const std::uint8_t shift_register = (shift & 0xF0) >> 4; // Bit 7-4

		mnemonic += get_register_name(op2_register) + ", ";

		switch (shift_type) {
			case (0): mnemonic += "LSL "; break; // Logical Shift Left
			case (1): mnemonic += "LSR "; break; // Logical Shift Right
			case (2): mnemonic += "ASR "; break; // Arithmetic Shift Right
			case (3): mnemonic += "ROR "; break; // Rotate Right
		}

		mnemonic += get_register_name(shift_register);
	}
	else {
		/*
		|...1 .........0|
		|1_0_9_8_7_6_5_4|
		|__imm__|0|_ST|0|
		*/
		const std::uint8_t immediate = (shift & 0xF8) >> 3; // Bit 11-7

		if (shift_type == 0 && immediate == 0) {
			mnemonic += get_register_name(op2_register);
		}
		else if (shift_type == 3 && immediate == 0) {
			mnemonic += get_register_name(op2_register) + ", RRX";
		}
		else {
			mnemonic += get_register_name(op2_register) + ", ";
			switch (shift_type) {
				case 0: mnemonic += "LSL "; break; // Logical Shift Left
				case 1: mnemonic += "LSR "; break; // Logical Shift Right
				case 2: mnemonic += "ASR "; break; // Arithmetic Shift Right
				case 3: mnemonic += "ROR "; break; // Rotate Right
			}
			std::uint8_t shown = (immediate == 0) ? 32 : immediate;
			mnemonic += print_literal(shown);
		}
	}

	return mnemonic;
}

/* ---  Format Dispatchers --- */

td::InstructionDataArm td::ArmDisasm::dispatch_arm(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0|I|___op__|s|___Rn__|___Rd__|_______Operand 2_______| - Data Processing / PCR Transfer
	|__Cond_|0 0 0 0 0 0|A|A|___Rd__|___Rn__|___Rs__|1 0 0 1|___Rm__| - Multiply
	|__Cond_|0 0 0 0 1|U|A|S|__RdHi_|__RdLo_|___Rs__|1 0 0 1|___Rm__| - Multiply Long
	|__Cond_|0 0 0 1 0|B|0 0|___Rn__|___Rd__|0 0 0 0 1 0 0 1|___Rm__| - Single Data Swap
	|__Cond_|0 0 0 1|0 0 1 0|1 1 1 1|1 1 1 1|1 1 1 1|0 0 0 1|__Rn___| - Branch & Exchange
	|__Cond_|0 0 0|P|U|0|W|L|___Rn__|___Rd__|0 0 0 0 1|S|H|1|___Rm__| - Halfword Data Transfer: Register Offset
	|__Cond_|0 0 0|P|U|1|W|L|___Rn__|___Rd__|_offset|1|S|H|1|_offset| - Halfword Data Transfer: Immediate Offset
	|__Cond_|0 1|I|P|U|B|W|L|___Rn__|___Rd__|_________Offset________| - Single Data Transfer
	|__Cond_|0 1 1|- - - - - - - - - - - - - - - - - - - -|1|- - - -| - Undefined
	|__Cond_|1 0 0|P|U|S|W|L|___Rn__|_________Register List_________| - Block Data Transfer
	|__Cond_|1 0 1|L|_____________________offset____________________| - Branch
	|__Cond_|1 1 0|P|U|N|W|L|___Rn__|__CRd__|__CP#__|_____Offset____| - Coprocessor Data Transfer
	|__Cond_|1 1 1 0|_Cp Op_|__CRn__|__CRd__|__CP#__|__CP_|0|__CRm__| - Coprocessor Data Operation
	|__Cond_|1 1 1 0|Cp Op|L|__CRn__|__CRd__|__CP#__|__CP_|1|__CRm__| - Coprocessor Reguster Transfer
	|__Cond_|1 1 1 1|_________________Comment Field_________________| - Software Interupt
	*/
	if ((instr & 0x0FFFFFF0) == 0x012FFF10) return dis_branch_exchange(pc, instr);

	uint8_t primary_type = (instr & 0x0E000000) >> 25;
	switch (primary_type) {
		case 0b000:
			if ((instr & 0x000000F0) == 0x90) {
				uint8_t op24 = (instr >> 23) & 0x3;
				if (op24 == 0) return dis_mul_mla(pc, instr);
				else if (op24 == 0b01) return dis_mul_mla_long(pc, instr);
				else if (op24 == 0b10) return dis_single_data_swap(pc, instr);
				break;
			}
			else if ((instr & 0x00000090) == 0x90) {
				if ((instr & 0x00400000) != 0) return dis_halfword_data_trans_imm(pc, instr);
				return dis_halfword_data_trans_reg(pc, instr);
			}
			[[fallthrough]];
		case 0b001:
			return dispatch_data_proc_psr(pc, instr);
		case 0b010:
		case 0b011:
			if (primary_type == 0b011 && (instr & 0x00000010) != 0) return { pc, instr, "Undefined." };
			return dis_single_data_trans(pc, instr);
		case 0b100: return dis_block_data_trans(pc, instr);
		case 0b101: return dis_branch(pc, instr);
		case 0b110: return { pc, instr, "Coprocessor Data Transfer unimplemented" };
		case 0b111:
			if ((instr & 0x01000000) != 0) return dis_swi(pc, instr);
			if ((instr & 0x00000010) != 0) return { pc, instr, "Coprocessor Register Transfer unimplemented" };
			return { pc, instr, "Coprocessor Data Operation unimplemented" };
	}

	return { pc, instr, "Invalid instruction." };
}

td::InstructionDataArm td::ArmDisasm::dispatch_data_proc_psr(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 1 0|P|0 0 1 1 1 1|___Rd__|0 0 0 0 0 0 0 0 0 0 0 0| - MRS
	|__Cond_|0 0 0 1 0|P|1 0 1 0 0 1 1 1 1 1 0 0 0 0 0 0 0 0|___Rm__| - MSR Register
	|__Cond_|0 0|I|___op__|s|___Rn__|___Rd__|_______Operand 2_______| - MSR Immediate
	|__Cond_|0 0|I|1 0|P|1 0 1 0 0 0 1 1 1 1|_____Source Operand____| - Datta Processing
	*/
	const std::uint32_t MRS_MASK = 0x0FBF0FFF, MRS_TEST = 0x010F0000;
	const std::uint32_t MSR_R_MASK = 0x0FBFFFF0, MSR_R_TEST = 0x0129F000;
	const std::uint32_t MSR_I_MASK = 0x0DBFF000, MSR_I_TEST = 0x0128F000;

	if ((instr & MRS_MASK) == MRS_TEST) return dis_psr_trans_MRS(pc, instr);
	if ((instr & MSR_R_MASK) == MSR_R_TEST) return dis_psr_trans_MSR_reg(pc, instr);
	if ((instr & MSR_I_MASK) == MSR_I_TEST) return dis_psr_trans_MSR_imm(pc, instr);
	return dis_data_proc(pc, instr);
}

/* --- Format Decoders --- */

td::InstructionDataArm td::ArmDisasm::dis_branch_exchange(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 1|0 0 1 0|1 1 1 1|1 1 1 1|1 1 1 1|0 0 0 1|__Rn___|
	*/
	const uint8_t cond = (instr >> 28) & 0xF; // Bit 31-28
	const uint8_t op_register = instr & 0xF;  // Bit 3-0

	std::string mnemonic = "BX";
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += get_register_name(op_register);

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_branch(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|1 0 1|L|_____________________offset____________________|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;   // Bit 31-28
	bool link = (instr >> 24) & 0x1;                 // Bit 24
	const std::uint32_t offset = instr & 0x00FFFFFF; // Bit 32-0

	std::string mnemonic = "B";
	if (link) mnemonic += "L";
	mnemonic += get_cond_suffix(cond) + " ";

	std::int32_t address = std::bit_cast<std::int32_t>(std::uint32_t(offset) << 8) >> 6; // Sign extend and shift left 2
	address += pc + 8;

	mnemonic += print_literal(address);

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_data_proc(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0|I|___op__|s|___Rn__|___Rd__|_______Operand 2_______|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;          // Bit 31-28
	const bool is_immediate = (instr >> 25) & 0x1;          // Bit 25
	const std::uint8_t opcode = (instr >> 21) & 0xF;        // Bit 24-21
	const bool set_con = (instr >> 20) & 0x1;               // Bit 20
	const std::uint8_t op1_register = (instr >> 16) & 0xF;  // Bit 19-16
	const std::uint8_t dest_register = (instr >> 12) & 0xF; // Bit 15-12

	std::string mnemonic;
	switch (opcode) {
		case (0): mnemonic += "AND"; break;
		case (1): mnemonic += "EOR"; break;
		case (2): mnemonic += "SUB"; break;
		case (3): mnemonic += "RSB"; break;
		case (4): mnemonic += "ADD"; break;
		case (5): mnemonic += "ADC"; break;
		case (6): mnemonic += "SBC"; break;
		case (7): mnemonic += "RSC"; break;
		case (8): mnemonic += "TST"; break;
		case (9): mnemonic += "TEQ"; break;
		case (10): mnemonic += "CMP"; break;
		case (11): mnemonic += "CMN"; break;
		case (12): mnemonic += "ORR"; break;
		case (13): mnemonic += "MOV"; break;
		case (14): mnemonic += "BIC"; break;
		case (15): mnemonic += "MVN"; break;
	}

	mnemonic += get_cond_suffix(cond);

	if (opcode >= 8 && opcode <= 11)
		mnemonic += " ";
	else {
		if (set_con) mnemonic += "S";
		mnemonic += " ";
	}

	if (opcode < 8 || opcode > 11)
		mnemonic += get_register_name(dest_register) + ", ";

	if (opcode != 13 && opcode != 15)
		mnemonic += get_register_name(op1_register) + ", ";

	// Operand 2
	if (is_immediate) {
		/*
		|...1 .................0|
		|1_0_9_8_7_6_5_4_3_2_1_0|
		|_rotate|___immediate___|
		*/
		const std::uint8_t rotate = (instr >> 8) & 0xF; // Bit 11-8
		const std::uint8_t immediate = instr & 0xFF;    // Bit 7-0

		std::uint32_t literal = rotr32(immediate, rotate * 2);
		mnemonic += print_literal(literal);
	}
	else {
		mnemonic += build_shift_op(instr);
	}

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_psr_trans_MRS(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 1 0|P|0 0 1 1 1 1|___Rd__|0 0 0 0 0 0 0 0 0 0 0 0|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;          // Bit 31-28
	const bool source_psr = (instr >> 22) & 0x1;            // Bit 22
	const std::uint8_t dest_register = (instr >> 12) & 0xF; // Bit 15-12

	std::string mnemonic = "MRS";
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += (source_psr ? "SPSR" : "CPSR");

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_psr_trans_MSR_reg(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 1 0|P|1 0 1 0 0 1 1 1 1 1 0 0 0 0 0 0 0 0|___Rm__|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF; // Bit 31-28
	const bool dest_psr = (instr >> 22) & 0x1;     // Bit 22
	const std::uint8_t src_register = instr & 0xF; // Bit 3-0

	std::string mnemonic = "MSR";
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += std::string(dest_psr ? "SPSR" : "CPSR") + ", ";
	mnemonic += get_register_name(src_register);

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_psr_trans_MSR_imm(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0|I|1 0|P|1 0 1 0 0 0 1 1 1 1|_____Source Operand____|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF; // Bit 31-28
	const bool is_immediate = (instr >> 25) & 0x1; // Bit 25
	const bool dest_psr = (instr >> 22) & 0x1;     // Bit 22

	std::string mnemonic = "MSR";
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += (dest_psr ? "SPSR_flg " : "CPSR_flg ");

	// Source Operand
	if (is_immediate) {
		/*
		|...1 .................0|
		|1_0_9_8_7_6_5_4_3_2_1_0|
		|_rotate|___immediate___|
		*/
		const std::uint8_t rotate = (instr >> 8) & 0xF; // Bit 11-8
		const std::uint8_t immediate = instr & 0xFF;    // Bit 7-0

		std::uint32_t literal = rotr32(immediate, rotate * 2);
		mnemonic += print_literal(literal);
	}
	else {
		const std::uint8_t src_register = instr & 0xF; // Bit 3-0

		mnemonic += get_register_name(src_register);
	}

	return {pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_mul_mla(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 0 0 0|A|A|___Rd__|___Rn__|___Rs__|1 0 0 1|___Rm__|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;          // Bit 31-28
	const bool accumulate = (instr >> 21) & 0x1;            // Bit 21
	const bool set_con = (instr >> 20) & 0x1;               // Bit 20
	const std::uint8_t dest_register = (instr >> 16) & 0xF; // Bit 19-16
	const std::uint8_t op1_register = (instr >> 12) & 0xF;  // Bit 15-12
	const std::uint8_t op2_register = (instr >> 8) & 0xF;   // Bit 11-8
	const std::uint8_t op3_register = instr & 0xF;          // Bit 3-0

	std::string mnemonic = (accumulate ? "MLA" : "MUL");
	mnemonic += get_cond_suffix(cond);
	mnemonic += (set_con ? "S " : " ");
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += get_register_name(op3_register) + ", ";
	mnemonic += get_register_name(op2_register);
	if (accumulate) mnemonic += ", " + get_register_name(op1_register);

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_mul_mla_long (std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 0 1|U|A|S|__RdHi_|__RdLo_|___Rs__|1 0 0 1|___Rm__|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;               // Bit 31-28
	const bool is_unsigned = (instr >> 22) & 0x1;                // Bit 22
	const bool accumulate = (instr >> 21) & 0x1;                 // Bit 21
	const bool set_flags = (instr >> 20) & 0x1;                  // Bit 20
	const std::uint8_t dest_register_high = (instr >> 16) & 0xF; // Bit 19-16
	const std::uint8_t dest_register_low = (instr >> 12) & 0xF;  // Bit 15-12
	const std::uint8_t op1_register = (instr >> 8) & 0xF;        // Bit 11-8
	const std::uint8_t op2_register = instr & 0xF;               // Bit 3-0

	std::string mnemonic = (is_unsigned ? "S" : "U");
	mnemonic += (accumulate ? "MLAL" : "MULL");
	mnemonic += get_cond_suffix(cond);
	mnemonic += (set_flags ? "S " : " ");
	mnemonic += get_register_name(dest_register_low) + ", ";
	mnemonic += get_register_name(dest_register_high) + ", ";
	mnemonic += get_register_name(op2_register) + ", ";
	mnemonic += get_register_name(op1_register);

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_single_data_trans(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 1|I|P|U|B|W|L|___Rn__|___Rd__|_________Offset________|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;            // Bit 31-28
	const bool is_register = (instr >> 25) & 0x1;             // Bit 25
	const bool pre_index = (instr >> 24) & 0x1;               // Bit 24
	const bool add_offset = (instr >> 23) & 0x1;              // Bit 23
	const bool transfer_byte = (instr >> 22) & 0x1;           // Bit 22
	const bool write_back = (instr >> 21) & 0x1;              // Bit 21
	const bool is_load = (instr >> 20) & 0x1;                 // Bit 20
	const std::uint8_t base_register = (instr >> 16) & 0xF;   // Bit 19-16
	const std::uint8_t target_register = (instr >> 12) & 0xF; // Bit 15-12

	std::string mnemonic = (is_load ? "LDR" : "STR");
	mnemonic += get_cond_suffix(cond);
	if (transfer_byte) mnemonic += "B";
	if (transfer_byte && !pre_index)  mnemonic += 'T';
	mnemonic += " " + get_register_name(target_register) + ", ";

	std::string address;
	if (is_register) {
		/*
		|...1 .................0|
		|1_0_9_8_7_6_5_4_3_2_1_0|
		|__shift__|_ST|0|___Rm__|
		*/
		const uint8_t shift_type = (instr >> 5) & 0x3;    // Bit 6-5
		const uint8_t shift_amount = (instr >> 7) & 0x1F; // Bit 4
		const uint8_t offset_register = instr & 0xF;      // Bit 3-0

		const bool offset_zero = (shift_type == 0) &&
						     	 (shift_amount == 0) &&
							     (offset_register == 0) &&
								 (add_offset == 1);
		std::string offset = build_shift_op(instr);

		if (pre_index) {
			address = "[" + get_register_name(base_register);
			if (!offset_zero) {
				address += ", ";
				if (!add_offset) address += "-";
				address += offset;
			}
			address += "]";
			if (write_back) address += "!";
		}
		else {
			address = "[" + get_register_name(base_register) + "]";
			if (!offset_zero) {
				address += ", ";
				if (!add_offset) address += "-";
				address += offset;
			}
		}
	}else {
		const std::uint16_t immediate = instr & 0xFFF; // Bit 11-0

		if (pre_index) {
			address = "[" + get_register_name(base_register);
			if (immediate != 0) {
				address += ", #";
				if (!add_offset) address += "-";
				address += print_literal(immediate, false);
			}
			address += "]";
			if (write_back) address += "!";
		} else {
			address = "[" + get_register_name(base_register) + "]";
			if (immediate != 0) {
				address += ", #";
				if (!add_offset) address += "-";
				address += print_literal(immediate, false);
			}
		}
	}
	mnemonic += address;

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_halfword_data_trans_reg(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0|P|U|0|W|L|___Rn__|___Rd__|0 0 0 0 1|S|H|1|___Rm__|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;            // Bit 31-28
	const bool pre_index = (instr >> 24) & 0x1;               // Bit 24
	const bool add_offset = (instr >> 23) & 0x1;              // Bit 23
	const bool write_back = (instr >> 21) & 0x1;              // Bit 21
	const bool is_load = (instr >> 20) & 0x1;                 // Bit 20
	const std::uint8_t base_register = (instr >> 16) & 0xF;   // Bit 19-16
	const std::uint8_t target_register = (instr >> 12) & 0xF; // Bit 15-12
	const std::uint8_t sh = (instr >> 5) & 0x3;               // Bit 6-5
	const std::uint8_t offset_register = instr & 0xF;         // Bit 3-0

	// SH=0b10 and SH=0b11 is only valid during load operations
	if (!is_load && (sh == 2 || sh == 3)) return { pc, instr, "Unknown instruction" };

	std::string mnemonic = (is_load ? "LDR" : "STR");
	switch (sh) {
		case 1: mnemonic += "H"; break;               // Transfer halfword
		case 2: if (is_load) mnemonic += "SB"; break; // Load sign extended byte
		case 3: if (is_load) mnemonic += "SH"; break; // Load sign extended halfword
	}
	mnemonic += get_cond_suffix(cond) + " ";

	mnemonic += get_register_name(target_register) + ", ";

	bool omit_offset = (offset_register == 0) && add_offset;
	std::string address;
	if (pre_index) {
		address = "[" + get_register_name(base_register);
		if (!omit_offset) {
			address += ", ";
			if (!add_offset) address += "-";
			address += get_register_name(offset_register);
		}
		address += "]";
		if (write_back) address += "!";
	} else {
		address = "[" + get_register_name(base_register) + "]";
		if (!omit_offset) {
			address += ", ";
			if (!add_offset) address += "-";
			address += get_register_name(offset_register);
		}
	}
	mnemonic += address;

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_halfword_data_trans_imm(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0|P|U|1|W|L|___Rn__|___Rd__|_offset|1|S|H|1|_offset|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;            // Bit 31-28
	const bool pre_index = (instr >> 24) & 0x1;               // Bit 24
	const bool add_offset = (instr >> 23) & 0x1;              // Bit 23
	const bool write_back = (instr >> 21) & 0x1;              // Bit 21
	const bool is_load = (instr >> 20) & 0x1;                 // Bit 20
	const std::uint8_t base_register = (instr >> 16) & 0xF;   // Bit 19-16
	const std::uint8_t target_register = (instr >> 12) & 0xF; // Bit 15-12
	const std::uint8_t offset_high = (instr >> 8) & 0xF;      // Bit 11-8
	const std::uint8_t sh = (instr >> 5) & 0x3;               // Bit 6-5
	const std::uint8_t offset_low = instr & 0xF;              // Bit 3-0
	const std::uint8_t offset = (offset_high << 4) | offset_low;

	// SH=0b10 and SH=0b11 is only valid during load operations
	if (!is_load && (sh == 2 || sh == 3)) return { pc, instr, "Invalid instruction." };

	std::string mnemonic = (is_load ? "LDR" : "STR");
	switch (sh) {
		case 1: mnemonic += "H"; break;               // Transfer halfword
		case 2: if (is_load) mnemonic += "SB"; break; // Load sign extended byte
		case 3: if (is_load) mnemonic += "SH"; break; // Load sign extended halfword
	}
	mnemonic += get_cond_suffix(cond) + " ";

	mnemonic += get_register_name(target_register) + ", ";

	bool omitOffset = (offset == 0) && add_offset;
	std::string address;
	if (pre_index) {
		address = "[" + get_register_name(base_register);
		if (!omitOffset) {
			address += ", #";
			if (!add_offset) address += "-";
			address += print_literal(offset, false);
		}
		address += "]";
		if (write_back) address += "!";
	}
	else {
		address = "[" + get_register_name(base_register) + "]";
		if (!omitOffset) {
			address += ", #";
			if (!add_offset) address += "-";
			address += print_literal(offset, false);
		}
	}
	mnemonic += address;

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_block_data_trans(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|1 0 0|P|U|S|W|L|___Rn__|_________Register List_________|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;          // Bit 31-28
	const bool pre_index = (instr >> 24) & 0x1;             // Bit 24
	const bool add_offset = (instr >> 23) & 0x1;            // Bit 23
	const bool load_psr = (instr >> 22) & 0x1;              // Bit 22
	const bool write_back = (instr >> 21) & 0x1;            // Bit 21
	const bool is_load = (instr >> 20) & 0x1;               // Bit 20
	const std::uint8_t base_register = (instr >> 16) & 0xF; // Bit 19-16
	const std::uint16_t register_list = instr & 0xFFFF;     // Bit 15-0

	std::string mnemonic = (is_load ? "LDM" : "STM");
	switch ((pre_index << 1) | add_offset) {
		case 0b00: mnemonic += "DA"; break; // Post-Decrement
		case 0b01: break;              // IA ; Post - Increment; Default
		case 0b10: mnemonic += "DB"; break; // Pre-Decrement
		case 0b11: mnemonic += "IB"; break; // // Pre-Increment
	}
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += get_register_name(base_register);
	if (write_back) mnemonic += "!";

	mnemonic += ", {" + print_register_list(register_list, 16) + "}";
	if (load_psr) mnemonic += "^";

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_single_data_swap(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|0 0 0 1 0|B|0 0|___Rn__|___Rd__|0 0 0 0 1 0 0 1|___Rm__|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;          // Bit 31-28
	const bool is_byte = (instr >> 22) & 0x1;               // Bit 22
	const std::uint8_t base_register = (instr >> 16) & 0xF; // Bit 19-16
	const std::uint8_t dest_register = (instr >> 12) & 0xF; // Bit 15-12
	const std::uint8_t source_register = instr & 0xF;       // Bit 3-0

	std::string mnemonic = "SWP";
	mnemonic += get_cond_suffix(cond);
	mnemonic += (is_byte ? "B " : " ");
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += get_register_name(source_register) + ", ";
	mnemonic += "[" + get_register_name(base_register) + "]";

	return { pc, instr, mnemonic };
}

td::InstructionDataArm td::ArmDisasm::dis_swi(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|__Cond_|1 1 1 1|_________________Comment Field_________________|
	*/
	const std::uint8_t cond = (instr >> 28) & 0xF;    // Bit 31-28
	const std::uint32_t comment = instr & 0x00FFFFFF; // Bit 23-0

	std::string mnemonic = "SWI";
	mnemonic += get_cond_suffix(cond) + " ";
	mnemonic += print_literal(comment);

	return { pc, instr, mnemonic };
}
