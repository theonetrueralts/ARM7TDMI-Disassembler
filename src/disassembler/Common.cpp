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
