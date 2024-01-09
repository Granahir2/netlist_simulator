#include "parser.hh"

#include <iostream>
#include <map>

std::ostream& operator<<(std::ostream& o, arg& a) {
	if(a.is_cst) {
		return o << '#' << a.value;
	} else {
		return o << '%' << a.value;
	}
}

parser_error::parser_error(std::string& w) : std::logic_error(w) {};
undeclared_id::undeclared_id(std::string& w) : parser_error(w) {};
invalid_cst::invalid_cst(std::string& w)   : parser_error(w)  {};

parsed_program parse(std::istream& input, bool debug, std::ostream& log_out) {
	input >> std::skipws;
	std::map<std::string, uint64_t> ident_number;

	std::string w;
	do { input >> w;} while(w != std::string("INPUT"));
	input >> w;

	/* On parse la section INPUT */
	auto curr_id = 0u;
	while(w	!= std::string("OUTPUT")) {
		if(w.back() == ',') {w = w.substr(0, w.size() - 1);}

		ident_number[w] = curr_id;
		curr_id++;
		input >> w;
	}
	auto number_of_inputs = curr_id;
	input >> w;

	/* On parse la section OUTPUT */
	while(w != std::string("VAR")) {
		if(w.back() == ',') { w = w.substr(0, w.size() - 1);}
		ident_number[w] = curr_id;
		curr_id++;
		input >> w;
	}
	auto number_of_outputs = curr_id - number_of_inputs;
	input >> w;


	/* On parse la section VAR */
	std::vector<std::string> idents(curr_id, "");
	std::vector<unsigned int> id_widths(curr_id, 0);

	while(w != std::string("IN")) {
		std::string identifier;
		int ident_width = 1;

		auto colpos = w.find_first_of(":");
		if(colpos != std::string::npos) {
				identifier = w.substr(0, colpos);
				ident_width = std::stoi(w.substr(colpos+1, w.length() - colpos - 1));
				input >> w;
		} else {
				if(w.back() == ',') {
					identifier = w.substr(0, w.length() - 1);
					input >> w;
				} else {
					identifier = w;
					input >> w;
					if(w.front() == ':') {
						if(w.size() == 1) {input >> w;} else {w = w.substr(1, w.size());};
						ident_width = std::stoi(w);
					}
				}
		}


		/*
		if(w.back() == ',') {
			auto colpos = w.find_first_of(":");
			if(colpos != std::string::npos) {
				identifier  = w.substr(0, colpos);
				ident_width = std::stoi(w.substr(colpos+1, w.length() - colpos - 1));
			} else {
				identifier = w.substr(0, w.length() - 1);
			}
			input >> w;
		} else {
			identifier = w;
			input >> w;
				
			if(w.front() == ':') { // soit c'était un : soit c'était vraiment le field d'après
				input >> w; // c'est la width, sauf peut-être un , à la fin 
				ident_width = std::stoi(w);

				input >> w;
			}
		}*/

		if(ident_number.contains(identifier)) {
			idents[ident_number[identifier]]    = identifier;
			id_widths[ident_number[identifier]] = ident_width; 
		} else {
			ident_number[identifier] = idents.size();
			idents.push_back(identifier);
			id_widths.push_back(ident_width);
		}
	}

	if(debug) {
		for(auto i = 0u; i < idents.size(); ++i) {
			log_out << idents[i] << " : " << id_widths[i] << " : " << ident_number[idents[i]] << std::endl;
		}
	}

	if(input.eof()) {return {number_of_inputs, number_of_outputs, idents, id_widths, std::vector<instr>(), std::vector<RAM>(), std::vector<ROM>()};} // Si il n'y a pas d'instruction


	auto decode_ident_safe = [&](std::string str) {
					if(!ident_number.contains(str)) {throw std::logic_error("Undeclared use of identifier : \"" + str + "\"");}
					return (arg){false, ident_number[str]};
				};
	auto get_arg = [&](){std::string str; input >> str;
			     try { uint64_t i = std::stoll(str); /*if(i > 1) {throw std::logic_error("Multi-bit constant");};*/ return (arg){true, i};}
			     catch(std::invalid_argument &x) {
				return decode_ident_safe(str);
			     }
			    };	

	std::vector<instr> instlist;
	std::vector<ROM> romlist;
	std::vector<RAM> ramlist;

	for(; input >> w;) {
		instr instruction;	

		instruction.lhs_id = decode_ident_safe(w).value;
		input >> w; // On jette le "="
		
		input >> w;
		if(w == std::string("REG")) {
			input >> w;
			instruction.op = REG;
			instruction.argv[0] = decode_ident_safe(w);
		} else if(w == std::string("OR")) {
			instruction.op = OR;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
		} else if(w == std::string("XOR")) {
			instruction.op = XOR;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
		} else if(w == std::string("NOT")) {
			instruction.op = NOT;
			instruction.argv[0] = get_arg();
		} else if(w == std::string("AND")) {
			instruction.op = AND;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
		} else if(w == std::string("NAND")) {
			instruction.op = NAND;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
		} else if(w == std::string("CONCAT")) {
			instruction.op = CONCAT;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
		} else if(w == std::string("SELECT")) {
			instruction.op = SELECT;
			input >> w;
			instruction.argv[0].is_cst = true;
			try {instruction.argv[0].value = std::stoul(w);} catch (std::exception &e) {throw e;};
			instruction.argv[1] = get_arg();
		} else if(w == std::string("SLICE")) {
			instruction.op = SLICE;
			instruction.argv[0].is_cst = true;
			instruction.argv[1].is_cst = true;
			
			try {
				input >> w;
				instruction.argv[0].value = std::stoi(w);
				input >> w;
				instruction.argv[1].value = std::stoi(w);
			} catch (std::exception &e) {throw e;};

			instruction.argv[2] = get_arg();
		} else if(w == std::string("MUX")) {
			instruction.op = MUX;
			instruction.argv[0] = get_arg();
			instruction.argv[1] = get_arg();
			instruction.argv[2] = get_arg();
		} else if(w == std::string("ROM")) {
			instruction.op = ROMFETCH;
			instruction.argv[0].is_cst = true;
			instruction.argv[0].value  = romlist.size();
			try {
				std::string adsz; input >> adsz;
				std::string wdsz; input >> wdsz; 
				romlist.push_back({(unsigned)std::stoul(adsz), (unsigned)std::stoul(wdsz)});	
			} catch(std::exception &e) {throw e;};

			instruction.argv[1] = get_arg();
		} else if(w == std::string("RAM")) {
			instruction.op = RAMFETCH;
			instruction.argv[0].is_cst = true;
			instruction.argv[0].value = ramlist.size();
			try {
				std::string adsz; input >> adsz;
				std::string wdsz; input >> wdsz;
				instruction.argv[1] = get_arg();
				
				auto write_en_node = get_arg();
				auto write_ad_node = get_arg();
				auto write_dta_node = get_arg();

				ramlist.push_back({(unsigned)std::stoul(adsz), (unsigned)std::stoul(wdsz),
						   write_en_node, write_ad_node,
						   write_dta_node});
			} catch(std::exception &e) {throw e;};
		} else {
			instruction.op = COPY;
			try {instruction.argv[0] = {true, std::stoul(w, nullptr, 2)};} catch(std::invalid_argument &e) {
				instruction.argv[0] = decode_ident_safe(w);
			}
		}
		instlist.push_back(instruction);
	}

	if(debug) {
		for(auto inst : instlist) {
			log_out << '%' << inst.lhs_id << " = ";
			if(inst.op == COPY) log_out << "COPY " << inst.argv[0]; 
			else if(inst.op == REG) log_out << "REG "   << inst.argv[0]; 
			else if(inst.op == OR) log_out << "OR "     << inst.argv[0] << ' ' << inst.argv[1];
			else if(inst.op == XOR) log_out << "XOR "   << inst.argv[0] << ' ' << inst.argv[1]; 
			else if(inst.op == NOT) log_out << "NOT "   << inst.argv[0]; 
			else if(inst.op == AND) log_out << "AND "   << inst.argv[0] << ' ' << inst.argv[1];
			else if(inst.op == NAND) log_out << "NAND " << inst.argv[0] << ' ' << inst.argv[1]; 
			else if(inst.op == ROMFETCH) log_out << "ROMFETCH " << inst.argv[0] << ' ' << inst.argv[1]; 
			else if(inst.op == RAMFETCH) log_out << "RAMFETCH " << inst.argv[0] << ' ' << inst.argv[1]; 
			else if(inst.op == CONCAT) log_out << "CONCAT " << inst.argv[0] << ' ' << inst.argv[1]; 
			else if(inst.op == SLICE) log_out << "SLICE " << inst.argv[0] << ' ' << inst.argv[1] << ' ' << inst.argv[2]; 
			else if(inst.op == SELECT) log_out << "SELECT " << inst.argv[0] << ' ' << inst.argv[1];
			else if(inst.op == MUX) {log_out << "MUX " << inst.argv[0] << ' ' << inst.argv[1] << ' ' << inst.argv[2];}
			log_out << '\n'; 
		}

		for(auto r : ramlist) {
			log_out << "RAM AW = " << r.address_width << " DW = " << r.data_width
				<< ' ' << r.we_node << ' ' << r.waddr_node << ' ' << r.wdata_node << '\n';
		}

		for(auto r : romlist) {
			log_out << "ROM AW = " << r.address_width << " DW = " << r.data_width << '\n';
		}
	}
	return {number_of_inputs, number_of_outputs, idents, id_widths, instlist, ramlist, romlist};
}
