#include <format>
#include <string>
#include <cstdint>

#include <totr/disassembler/ThumbDisasm.hpp>
#include <totr/disassembler/Common.hpp>

namespace td = totr::Disassembler;

td::InstructionData td::ThumbDisasm::decode(std::uint32_t pc, const std::uint32_t instr) const {
	return thumb_dispatcher(pc, instr);
}

/* --- Utility Methods --- */

std::string td::ThumbDisasm::print_literal(std::uint32_t v, bool prefix_hash) const {
	if (m_print_literals_hex) {
		if (prefix_hash) return std::format("#0x{:X}", v);
		return std::format("0x{:X}", v);
	}
	if (prefix_hash) return std::format("#{}", v);
	return std::format("{}", v);
}

/* ---  Format Dispatchers --- */

td::InstructionData td::ThumbDisasm::thumb_dispatcher(std::uint32_t pc, std::uint32_t instr) const {
	std::uint8_t high_byte = instr >> 8;

	// Ordered from most to least specific instruction set format masks.
	if ((high_byte & 0xFF) == 0xB0)      return dis_add_off_to_sp(pc, instr);
	else if ((high_byte & 0xFF) == 0xDF) return dis_swi(pc, instr);
	else if ((high_byte & 0xFC) == 0x40) return dis_alu_ops(pc, instr);
	else if ((high_byte & 0xFC) == 0x44) return dis_hi_reg_ops_bx(pc, instr);
	else if ((high_byte & 0xF6) == 0xB4) return dis_push_pop_reg(pc, instr);
	else if ((high_byte & 0xF8) == 0x18) return dis_add_sub(pc, instr);
	else if ((high_byte & 0xF8) == 0x48) return dis_pc_rel_load(pc, instr);
	else if ((high_byte & 0xF2) == 0x50) return dis_load_store_reg_off(pc, instr);
	else if ((high_byte & 0xF2) == 0x52) return dis_load_store_sign_ext(pc, instr);
	else if ((high_byte & 0xF8) == 0xE0) return dis_uncond_branch(pc, instr);
	else if ((high_byte & 0xF0) == 0x80) return dis_load_store_halfword(pc, instr);
	else if ((high_byte & 0xF0) == 0x90) return dis_sp_rel_load_store(pc, instr);
	else if ((high_byte & 0xF0) == 0xA0) return dis_load_address(pc, instr);
	else if ((high_byte & 0xF0) == 0xC0) return dis_multi_load_store(pc, instr);
	else if ((high_byte & 0xF0) == 0xD0) return dis_cond_branch(pc, instr);
	else if ((high_byte & 0xF0) == 0xF0) return dis_long_branch_link(pc, instr);
	else if ((high_byte & 0xE0) == 0x00) return dis_move_shifted_reg(pc, instr);
	else if ((high_byte & 0xE0) == 0x20) return dis_mov_cmp_add_sub_imm(pc, instr);
	else if ((high_byte & 0xE0) == 0x60) return dis_load_store_imm_off(pc, instr);
	return { pc, instr, "Invalid instruction.", true, 2, false, ModeEvent::None };
}

/* --- Format Decoders --- */

td::InstructionData td::ThumbDisasm::dis_move_shifted_reg(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___0___0_|___Op__|_______Offset______|_____Rs____|_____Rd____|
	*/
	const std::uint8_t opcode = (instr >> 11) & 0x3;      // Bit 12-11
	const std::uint8_t offset = (instr >> 6) & 0x1F;      // Bit 10-6
	const std::uint8_t src_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;       // Bit 2-0

	std::string mnemonic;
	switch (opcode) {
		case(0): mnemonic += "LSL "; break;
		case(1): mnemonic += "LSR "; break;
		case(2): mnemonic += "ASR "; break;
		// Case 3 falls into Add/Subtract format
	}
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += get_register_name(src_register) + ", ";
	mnemonic += print_literal(offset);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_add_sub(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|..........1 ..................0|
	|5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|0 0 0 1 1|I|O|RnOff|__Rs_|__Rd_|

	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___0___0___1___1_|_I_|_Op|___Rn/nn___|_____Rs____|_____Rd____|
	*/
	const bool is_imm = (instr >> 10) & 0x1;              // Bit 10
	const bool opcode = (instr >> 9) & 0x1;               // Bit 9
	const std::uint8_t value = (instr >> 6) & 0x7;        // Bit 8-6
	const std::uint8_t src_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;       // Bit 2-0

	std::string mnemonic = (opcode ? "SUB " : "ADD ");
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += get_register_name(src_register) + ", ";
	mnemonic += (is_imm ? print_literal(value) : get_register_name(value));

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_mov_cmp_add_sub_imm(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___0___1_|___Op__|_____Rd____|_____________Offset____________|
	*/
	const std::uint8_t opcode = (instr >> 11) & 0x3; // Bit 12-11
	const std::uint8_t target = (instr >> 8) & 0x7;  // Bit 10-8
	const std::uint8_t offset = instr & 0xFF;        // Bit 7-0

	std::string mnemonic;
	switch (opcode) {
		case(0): mnemonic += "MOV "; break;
		case(1): mnemonic += "CMP "; break;
		case(2): mnemonic += "ADD "; break;
		case(3): mnemonic += "SUB "; break;
	}
	mnemonic += get_register_name(target) + ", ";
	mnemonic += print_literal(offset);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_alu_ops(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___0___0___0___0_|_______Op______|_____Rs____|____Rd_____|
	*/
	const std::uint8_t opcode = (instr >> 6) & 0xF;       // Bit 9-6
	const std::uint8_t src_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;       // Bit 2-0

	std::string mnemonic;
	switch (opcode) {
		case (0): mnemonic += "AND "; break;
		case (1): mnemonic += "EOR "; break;
		case (2): mnemonic += "LSL "; break;
		case (3): mnemonic += "LSR "; break;
		case (4): mnemonic += "ASR "; break;
		case (5): mnemonic += "ADC "; break;
		case (6): mnemonic += "SBC "; break;
		case (7): mnemonic += "ROR "; break;
		case (8): mnemonic += "TST "; break;
		case (9): mnemonic += "NEG "; break;
		case (10): mnemonic += "CMP "; break;
		case (11): mnemonic += "CMN "; break;
		case (12): mnemonic += "ORR "; break;
		case (13): mnemonic += "MUL "; break;
		case (14): mnemonic += "BIC "; break;
		case (15): mnemonic += "MVN "; break;
	}
	mnemonic += get_register_name(dest_register) + ", ";
	mnemonic += get_register_name(src_register);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_hi_reg_ops_bx(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___0___0___0___1_|___Op__|_H1|_H2|___Rs/Hs___|___Rd/Hd___|
	*/
	const std::uint8_t opcode = (instr >> 8) & 0x3;       // Bit 9-8
	const bool high_op1 = (instr >> 7) & 0x1;             // Bit 7
	const bool high_op2 = (instr >> 6) & 0x1;             // Bit 6
	const std::uint8_t src_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;       // Bit 2-0

	std::string mnemonic;
	switch (opcode) {
		case(0):
			if (!high_op1 && !high_op2) return { pc, instr, "Invalid instruction.", true, 2 };
			mnemonic += "ADD ";
			break;
		case(1):
			if (!high_op1 && !high_op2) return { pc, instr, "Invalid instruction.", true, 2 };
			mnemonic += "CMP ";
			break;
		case(2):
			if (!high_op1 && !high_op2) return { pc, instr, "Invalid instruction.", true, 2 };
			mnemonic += "MOV ";
			break;
		case(3):
			if (high_op1) return { pc, instr, "Invalid instruction.", true, 2 };
			mnemonic += "BX ";

			if (high_op2) mnemonic += get_register_name(src_register + 8); // Registers 8-15
			else mnemonic += get_register_name(src_register);              // Registers 0-7
			
			return { pc, instr, mnemonic, true, 2, true, ModeEvent::BX };
	}

	if (high_op1) mnemonic += get_register_name(dest_register + 8) + ", "; // Registers 8-15
	else mnemonic += get_register_name(dest_register) + ", ";              // Registers 0-7

	if (high_op2) mnemonic += get_register_name(src_register + 8); // Registers 8-15
	else mnemonic += get_register_name(src_register);              // Registers 0-7

	return { pc, instr, mnemonic, true, 2 , true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_pc_rel_load(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___0___0___1_|_____Rd____|______________Word_____________|
	*/
	const std::uint8_t dest_register = (instr >> 8) & 0x7; // Bit 10-8
	const std::uint8_t immediate = instr & 0xFF;           // Bit 7-0

	std::string mnemonic = "LDR ";
	mnemonic += get_register_name(dest_register);
	mnemonic += ", [PC, ";
	mnemonic += print_literal((std::uint16_t)immediate << 2);
	mnemonic += "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_load_store_reg_off(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___0___1_|_L_|_B_|_0_|_____Ro____|_____Rb____|_____Rd____|
	*/
	const bool is_load = (instr >> 11) & 0x1;                // Bit 11
	const bool is_sign_extended = (instr >> 10) & 0x1;       // Bit 10
	const std::uint8_t offset_register = (instr >> 6) & 0x7; // Bit 8-6
	const std::uint8_t base_register = (instr >> 3) & 0x7;   // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;          // Bit 2-0

	std::string mnemonic = (is_load ? "LDR" : "STR");
	mnemonic += (is_sign_extended ? "B " : " ");
	mnemonic += get_register_name(dest_register) + " ";
	mnemonic += "[" + get_register_name(base_register) + ", ";
	mnemonic += get_register_name(offset_register) + "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_load_store_sign_ext(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___0___1_|_H_|_S_|_1_|_____Ro____|_____Rb____|_____Rd____|
	*/
	const std::uint8_t hs = (instr >> 10) & 0x3;             // Bit 11-10
	const std::uint8_t offset_register = (instr >> 6) & 0x7; // Bit 8-6
	const std::uint8_t base_register = (instr >> 3) & 0x7;   // Bit 5-3
	const std::uint8_t dest_register = instr & 0x7;          // Bit 2-0

	std::string mnemonic;
	switch (hs) {
		case 0b00: mnemonic += "STRH "; break;
		case 0b01: mnemonic += "LDRH "; break;
		case 0b10: mnemonic += "LDSB "; break;
		case 0b11: mnemonic += "LDSH "; break;
	}
	mnemonic += get_register_name(dest_register) + " ";
	mnemonic += "[" + get_register_name(base_register) + ", ";
	mnemonic += get_register_name(offset_register) + "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_load_store_imm_off(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_0___1___1_|_B_|_L_|_______Offset______|_____Rb____|_____Rd____|
	*/
	const bool is_byte = (instr >> 12) & 0x1;              // Bit 12
	const bool is_load = (instr >> 11) & 0x1;              // Bit 11
	const std::uint8_t offset = (instr >> 6) & 0x1F;       // Bit 10-6
	const std::uint8_t base_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t target_register = instr & 0x7;      // Bit 2-0

	std::string mnemonic = (is_load ? "LDR" : "STR" );
	mnemonic += (is_byte ? "B " : " ");
	mnemonic += get_register_name(target_register) + " ";
	mnemonic += "[" + get_register_name(base_register) + ", ";
	
	if (is_byte) mnemonic += print_literal(offset) + "]";
	else mnemonic += print_literal(offset << 2) + "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_load_store_halfword(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___0___0_|_L_|_______Offset______|_____Rb____|_____Rd____|
	*/
	const bool is_load = (instr >> 11) & 0x1;              // Bit 11
	const std::uint8_t immediate = (instr >> 6) & 0x1F;    // Bit 10-6
	const std::uint8_t base_register = (instr >> 3) & 0x7; // Bit 5-3
	const std::uint8_t target_register = instr & 0x7;      // Bit 2-0
	
	std::string mnemonic = (is_load ? "LDRH " : "STRH ");
	mnemonic += get_register_name(target_register) + " ";
	mnemonic += "[" + get_register_name(base_register) + ", ";
	mnemonic += print_literal(immediate << 1) + "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_sp_rel_load_store(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___0___1_|_L_|_____Rd____|___________Immediate___________|
	*/
	const bool is_load = (instr >> 11) & 0x1;              // Bit 11
	const std::uint8_t dest_register = (instr >> 8) & 0x7; // Bit 10-8
	const std::uint8_t immediate = instr & 0xFF;           // Bit 7-0

	std::string mnemonic = (is_load ? "LDR " : "STR ");
	mnemonic += get_register_name(dest_register) + " ";
	mnemonic += "[SP, " + print_literal((std::uint16_t)immediate << 2) + "]";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_load_address(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___1___0_|_SP|_____Rd____|___________Immediate___________|
	*/
	const bool source = (instr >> 11) & 0x1;               // Bit 11
	const std::uint8_t dest_register = (instr >> 8) & 0x7; // Bit 10-8
	const std::uint8_t immediate = instr & 0xFF;           // Bit 7-0

	std::string mnemonic;
	if (source) mnemonic = "ADD " + get_register_name(dest_register) + ", SP, ";
	else mnemonic = "ADR " + get_register_name(dest_register) + ", ";

	std::uint32_t literal = static_cast<std::uint16_t>(immediate) << 2;
	if (!source) literal += pc + 4;

	mnemonic += print_literal(literal);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_add_off_to_sp(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___1___1___0___0___0___0_|_S_|_________Immediate_________|
	*/
	const bool sign = (instr >> 7) & 0x1;        // Bit 7
	const std::uint8_t immediate = instr & 0x7F; // Bit 6-0

	std::string mnemonic = "ADD SP, ";
	mnemonic += (sign ? "#-" : "#");
	mnemonic += print_literal((std::uint16_t)immediate << 2, false);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_push_pop_reg(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___1___1_|_L_|_1___0_|_R_|_________Register List_________|
	*/
	const bool is_load = (instr >> 11) & 0x1;        // Bit 11
	const bool store_lr = (instr >> 8) & 0x1;        // Bit 8
	const std::uint8_t register_list = instr & 0xFF; // Bit 7-0

	std::string mnemonic = (is_load ? "PUSH {" : "POP {");
	mnemonic += print_register_list(register_list, 8);

	if (store_lr) {
		mnemonic += (is_load ? ", PC" : ", LR");
	}
	mnemonic += "}";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_multi_load_store(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___0___1___1_|_L_|_1___0_|_R_|_________Register List_________|
	*/
	const bool is_load = (instr >> 11) & 0x1;              // Bit 11
	const std::uint8_t base_register = (instr >> 8) & 0x7; // Bit 10-8
	const std::uint8_t register_list = instr & 0xFF;       // Bit 7-0

	std::string mnemonic = (is_load ? "LDMIA " : "STMIA ");
	mnemonic += get_register_name(base_register) + "!, {";
	mnemonic += print_register_list(register_list, 8) + "}";

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_cond_branch(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___1___0___1_|______Cond_____|_____________Offset____________|
	*/
	const std::uint8_t cond = (instr >> 8) & 0xF; // Bit 10-8
	const std::uint8_t offset = instr & 0xFF;     // Bit 7-0

	if (cond == 0xE) return { pc, instr, "Invalid instruction.", true, 2, false, ModeEvent::None };

	std::string mnemonic = "B";
	mnemonic += get_cond_suffix(cond) + " ";

	std::int8_t signed_offset = static_cast<std::int8_t>(offset);
	std::int8_t address = static_cast<std::int8_t>(signed_offset) << 1;
	mnemonic += print_literal(address + pc + 4);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_swi(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___1___0___1___1___1___1___1_|____________Comment____________|
	*/
	const std::uint8_t comment = instr & 0xFF;     // Bit 7-0

	std::string mnemonic = "SWI ";
	mnemonic += print_literal(comment);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_uncond_branch(std::uint32_t pc, const std::uint16_t instr) const {
	/*
	|_15|_14|_13|_12|_11|_10|_9_|_8_|_7_|_6_|_5_|_4_|_3_|_2_|_1_|_0_|
	|_1___1___1___0___0_|___________________Offset__________________|
	*/
	const std::uint16_t offset = instr & 0x7FF; // Bit 10-0

	std::string mnemonic = "B ";

	std::int16_t signed_offset = static_cast<std::int16_t>(offset << 5);
	std::int32_t address = static_cast<std::int32_t>(signed_offset) >> 4;
	mnemonic += print_literal(address + pc + 4);

	return { pc, instr, mnemonic, true, 2, true, ModeEvent::None };
}

td::InstructionData td::ThumbDisasm::dis_long_branch_link(std::uint32_t pc, const std::uint32_t instr) const {
	/*
	|..3 ..................2 ..................1 ..................0|
	|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
	|_______Insruction at PC+2______|1 1 1 1|H|________Offset_______|
	|____Offset-low half (H = 1)____|____Offset-high half (H = 0)___|
	*/
	const uint16_t next_instr = (instr >> 16) & 0xFFFF; // Bit 31-16
	const bool is_high_offset = (instr >> 11) & 0x1;    // Bit 11
	const std::uint16_t high_offset = instr & 0x7FF;    // Bit 10-0

	const uint8_t next_instr_sig = ((next_instr >> 11) & 0x1F); // Bit 15-11 in next instruction.
	if (is_high_offset || next_instr_sig != 0x1F) return { pc, instr, "Invalid instruction.", true, 2, false, ModeEvent::None };

	const uint16_t low_offset = next_instr & 0x7FF; // Bit 10-0 in next instruction.
	std::int32_t address = static_cast<std::int32_t>((high_offset << 12) | (low_offset << 1));
	if (high_offset & 0x0400) address |= 0xFF800000; // Sign extend the address

	std::string mnemonic = "BL " + print_literal(address + pc + 4);

	return { pc, instr, mnemonic, true, 4, true, ModeEvent::None };
}
