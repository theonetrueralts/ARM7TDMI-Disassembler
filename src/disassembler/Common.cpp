#include <totr/disassembler/Common.hpp>

std::string totr::Disassembler::get_register_name(std::uint8_t reg, bool use_alias) {
	reg &= 0x0F;
	if (use_alias) {
		switch (reg) {
			case (13): return "SP"; // R13 : Stack Pointer
			case (14): return "LR"; // R14 : Link Register
			case (15): return "PC"; // R15 : Program Counter
		}
	}
	return "R" + std::to_string(reg);
}

std::string totr::Disassembler::get_cond_suffix(std::uint8_t cond) {
	cond &= 0xF;
	switch (cond) {
		case (0): return "EQ";
		case (1): return "NE";
		case (2): return "CS";
		case (3): return "CC";
		case (4): return "MI";
		case (5): return "PL";
		case (6): return "VS";
		case (7): return "VC";
		case (8): return "HI";
		case (9): return "LS";
		case (10): return "GE";
		case (11): return "LT";
		case (12): return "GT";
		case (13): return "LE";
		case (14): return "";   // AL ; Default
		case (15): return "NV";
	}
	return "";
}

std::string totr::Disassembler::print_register_list(std::uint32_t register_list, int length) {
	std::string mnemonic;

	// Each of the 8 bits corresponds to a specific register by index. Eg. Bit 5=R5, Bit 0=R0, etc.
	bool running = false;
	std::uint8_t start = 0;
	for (std::uint8_t r = 0; r < length; r++) {
		if ((register_list >> r) & 0x1) {
			if (!running) {
				running = true;
				start = r;
				mnemonic += get_register_name(start, false);
			}
			else {
				if (r == length-1) {
					if (r - start == 2) mnemonic += ", " + get_register_name(r, false);
					else if (r - start > 2) mnemonic += "-" + get_register_name(r, false);
				}
			}
		}
		else {
			if (running) {
				running = false;
				if (r - start >= 2) mnemonic += "-" + get_register_name(r - 1, false);
				if (register_list >> r) mnemonic += ", ";
			}
		}
	}

	return mnemonic;
}