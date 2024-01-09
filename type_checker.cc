#include "type_checker.hh"

type_error::type_error(std::string& w) : std::logic_error(w) {};
illegal_length::illegal_length(std::string& w) : type_error(w) {};
incompatible_lengths::incompatible_lengths(std::string& w) : type_error(w) {};

void type_check(const parsed_program& p) {
	for(auto i = 0u; i < p.id_widths.size(); ++i) {
		if(p.id_widths[i] == 0 || p.id_widths[i] > 64) {
			std::string s;
			s += "Identifier ";
			s += p.idents[i];
			s += " has illegal size " + std::to_string(p.id_widths[i]);
			throw illegal_length(s);
		}
	}


	auto get_width = [&](arg a) {if(a.is_cst) {return 1u;} else {return p.id_widths[a.value];}};
	auto get_name  = [&](arg a) {if(a.is_cst) {return std::string("<CST>");} else {return p.idents[a.value];};};

	for(auto inst : p.code) {
		auto width = 0u;
		switch(inst.op) {
			case OR:
			case AND:
			case NAND:
			case XOR:
				if(get_width(inst.argv[0]) != get_width(inst.argv[1])) {
					auto s = "Can't combine " + get_name(inst.argv[0]) + " and "
						+ get_name(inst.argv[1]) + " through a binary operation";
					throw incompatible_lengths(s);
				}
				[[fallthrough]];
			case NOT:
			case REG:
				width = get_width(inst.argv[0]);
				break;
			case COPY:
				width = inst.argv[0].is_cst ? p.id_widths[inst.lhs_id] : get_width(inst.argv[0]);
				break;
			case ROMFETCH:
				if(p.roms[inst.argv[0].value].address_width != get_width(inst.argv[1])) {
					auto s = "Bad address width in access to ROM through " + get_name(inst.argv[1]);
					throw incompatible_lengths(s);
				}
				width = p.rams[inst.argv[0].value].data_width;
				break;
			case RAMFETCH:	
				if(p.rams[inst.argv[0].value].address_width != get_width(inst.argv[1])) {
					auto s = "Bad address width in access to RAM through " + get_name(inst.argv[1]);
					throw incompatible_lengths(s);
				}
				width = p.rams[inst.argv[0].value].data_width;
				break;
			case CONCAT:
				width = get_width(inst.argv[0]) + get_width(inst.argv[1]);
				break;
			case SLICE:
				if(inst.argv[1].value < inst.argv[0].value) {
					auto s = std::string("SLICE indexes in wrong order");
					throw incompatible_lengths(s);
				}
				if(inst.argv[1].value >= get_width(inst.argv[2])) {
					auto s = std::string("SLICE index out of bounds");
					throw incompatible_lengths(s);
				} 
				width = inst.argv[1].value - inst.argv[0].value + 1; // La fin de la range est *inclusive*
				break;
			case SELECT:
				if(inst.argv[0].value >= get_width(inst.argv[1])) {
					auto s = std::string("SELECT index out of bounds");
					throw incompatible_lengths(s);
				}
				width = 1;
				break;
			case MUX:
				if(get_width(inst.argv[0]) != 1) {
					auto s = "Cannot use MUX with " + get_name(inst.argv[0]) + " as selector";
					throw incompatible_lengths(s);
				}
				if(get_width(inst.argv[1]) != get_width(inst.argv[2])) {
					auto s = "Cannot MUX together " + get_name(inst.argv[1]) + " and " + get_name(inst.argv[2]);
					throw incompatible_lengths(s);
				}
				width = get_width(inst.argv[1]);
		}
		if(width != p.id_widths[inst.lhs_id]) {
			auto s = "Non-matching lengths in assignment of " + p.idents[inst.lhs_id];
			throw incompatible_lengths(s);
		}
	}

	for(auto r : p.rams) {
		if(r.address_width != get_width(r.waddr_node)) {
			auto s = "Bad write address size through bus " + get_name(r.waddr_node);
			throw incompatible_lengths(s);
		}
		if(r.data_width != get_width(r.wdata_node)) {
			auto s = "Bad write data bus size through " + get_name(r.wdata_node);
			throw incompatible_lengths(s);
		}
		if(get_width(r.we_node) != 1) {
			auto s = "Write enable " + get_name(r.we_node) + " should be a wire";
			throw incompatible_lengths(s);
		}
	}
}
