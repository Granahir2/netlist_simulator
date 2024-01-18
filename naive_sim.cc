#include <array>
#include <istream>
#include "naive_sim.hh"

void naive_simulation(scheduled_program pp, const std::vector<std::vector<uint64_t>>& rom_bufs, std::istream& i_strm, const std::vector<unsigned>& to_output, int nstep) {
	int N = pp.idents.size();
	std::array<std::vector<uint64_t>, 2> double_buffer = {std::vector<uint64_t>(N, 0), std::vector<uint64_t>(N, 0)};

	std::vector<std::vector<uint64_t>> ram_buffers(pp.rams.size());
	for(auto i = 0u; i < pp.rams.size(); ++i) {
		ram_buffers[i] = std::vector<uint64_t>(1ull << pp.rams[i].address_width);
	}

	const std::vector<std::vector<uint64_t>> rom_buffers = rom_bufs;

	for(int s = 0; (nstep == -1) ? true : s < nstep; ++s) {
		int currb = s % 2;					// Initialisation des entrées
		for(auto i = 0u; i < pp.input_number; ++i) {	
			i_strm.read(reinterpret_cast<char*>(&double_buffer[currb][i]), sizeof(uint64_t));
			if(!i_strm) {
				std::cout << pp.idents[i] << " ? ";
				std::string w;
				std::cin >> w;
				auto l = std::stoull(w, nullptr, 2);
				double_buffer[currb][i] = l;
			}
		}

		auto get_argval = [&](arg a){if(a.is_cst) {return a.value;} else {return double_buffer[currb].at(a.value);}};

		for(auto inst : pp.code) {
			uint64_t& lhs_ref = double_buffer[currb][inst.lhs_id];
			switch(inst.op) {
				case COPY:
					lhs_ref = get_argval(inst.argv[0]); break;
				case REG:
					lhs_ref = double_buffer[(s + 1)%2][inst.argv[0].value]; break;
				case NOT:
					lhs_ref = ~get_argval(inst.argv[0]); break;
				case OR:
					lhs_ref = get_argval(inst.argv[0]) | get_argval(inst.argv[1]); break;
				case XOR:
					lhs_ref = get_argval(inst.argv[0]) ^ get_argval(inst.argv[1]); break;
				case AND:
					lhs_ref = get_argval(inst.argv[0]) & get_argval(inst.argv[1]); break;
				case NAND:
					lhs_ref = ~(get_argval(inst.argv[0]) & get_argval(inst.argv[1])); break;
				case ROMFETCH:
					{uint64_t romid  = get_argval(inst.argv[0]);
					uint64_t addr   = get_argval(inst.argv[1]);
					uint64_t romsize = rom_buffers[romid].size();
					lhs_ref = rom_buffers[romid][addr % romsize]; break;}
				case RAMFETCH:
					{uint64_t ramid  = get_argval(inst.argv[0]);
					uint64_t addr   = get_argval(inst.argv[1]);
					uint64_t ramsize = ram_buffers[ramid].size();
					lhs_ref = ram_buffers[ramid][addr % ramsize]; break;}
				case CONCAT: /* les constantes sont implicitement de taille 1 ici. Normalement ça va. */
					{uint64_t left  = get_argval(inst.argv[0]);
					uint64_t right = get_argval(inst.argv[1]);
					int delta = inst.argv[0].is_cst ? 1 : pp.id_widths[inst.argv[0].value];
					lhs_ref = (left & ((1 << delta) - 1)) | (right << delta); break;}
				case SLICE:
					{uint64_t lo = inst.argv[0].value;
					// On a pas besoin de la valeur "haute" !
					uint64_t  v = get_argval(inst.argv[2]);
					lhs_ref = v >> lo; break;}
				case SELECT:
					{auto index = inst.argv[0].value;
					auto v = get_argval(inst.argv[1]);
					lhs_ref = v >> index; break;}
				case MUX:
					if(get_argval(inst.argv[0]) & 1) {
						lhs_ref = get_argval(inst.argv[2]);
					} else {
						lhs_ref = get_argval(inst.argv[1]);
					}
			}	
		}

		if(to_output.size() == 0) {
		for(auto i = 0u; i < pp.output_number; ++i) {
			std::cout << pp.idents[pp.input_number + i] << " = "
				  << get_argval({false, pp.input_number+i}) % (1ull << pp.id_widths[pp.input_number+i])
				  << ((i == pp.output_number - 1) ? "\n" : ",\t");
		}
		} else {
		for(auto i = 0u; i < to_output.size(); ++i) {
			std::cout << pp.idents[to_output[i]] << " = "
				  << get_argval({false, to_output[i]}) % (1ull << pp.id_widths[to_output[i]])
				  << ((i == to_output.size() - 1) ? "\n" : ",\t");
		}
		}

		for(auto i = 0u; i < pp.rams.size(); ++i) {
			auto r = pp.rams[i];
			if(get_argval(r.we_node) & 1) {
				auto s = ram_buffers[i].size();
				ram_buffers[i][get_argval(r.waddr_node) % s] = get_argval(r.wdata_node);
			}
		}
	}
} 
