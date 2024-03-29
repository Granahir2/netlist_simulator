#include <array>
#include <istream>
#include <ctime>
#include "naive_sim.hh"

void naive_simulation(scheduled_program pp,
const std::vector<std::vector<uint64_t>>& rom_bufs, std::istream& i_strm,
const std::vector<unsigned>& to_output, const std::vector<unsigned>& to_clock,
const char* preload_str, 
bool fast, unsigned lenout, int nstep) {
	int N = pp.idents.size();
	std::array<std::vector<uint64_t>, 2> double_buffer = {std::vector<uint64_t>(N, 0), std::vector<uint64_t>(N, 0)};

	std::vector<std::vector<uint64_t>> ram_buffers(pp.rams.size());
	for(auto i = 0u; i < pp.rams.size(); ++i) {
		ram_buffers[i] = std::vector<uint64_t>(1ull << pp.rams[i].address_width);
	}

	if(pp.rams.size() > 0 && preload_str != nullptr) {
		bool stop = false;		
		auto word_cnt  = ram_buffers[0].size();
		auto word_size = pp.rams[0].data_width;
		for(unsigned i = 0; i < word_cnt && !stop; i += 1) {
			uint64_t eff_word = 0;
			for(unsigned j = 0; j < word_size/8 && !stop; ++j) { // Words are stored little-endian
				eff_word |= (preload_str[i*word_size/8 + j] << (j*8));
				if(preload_str[i*word_size/8 + j] == '\0') {stop = true;}
			}
			ram_buffers[0][i] = eff_word;
		}
	}

	std::time_t last_t = std::time(nullptr);

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

		std::time_t curr_t = std::time(nullptr);
		if(fast || curr_t - last_t != 0) {last_t = curr_t;
			for(auto id : to_clock) {
				double_buffer[currb][id] = 1ull; // Forcefully clock
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

		
		for(auto i = 0u; i < pp.rams.size(); ++i) {
			auto r = pp.rams[i];
			if(get_argval(r.we_node) & 1) {
				auto s = ram_buffers[i].size();
				ram_buffers[i][get_argval(r.waddr_node) % s] = get_argval(r.wdata_node);
			}
			// TODO make that *safer*
			if(lenout != 0) {
				auto w = r.data_width/8;
				for(auto j = 0u; j < (ram_buffers[i].size()*w) && j < lenout; ++j) {
					std::cout << (char)((ram_buffers[i][j / w] >> ((j % w)*8))&0xff);
				}
					std::cout << '\t';
				}
		}
		
		for(auto i = 0u; i < to_output.size(); ++i) {
			std::cout << pp.idents[to_output[i]] << " = "
				  << get_argval({false, to_output[i]}) % (1ull << pp.id_widths[to_output[i]]);
			if(i != to_output.size() - 1) {std::cout << ",\t";}
		}
		std::cout << '\n';	
	}
} 
