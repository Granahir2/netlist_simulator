#include "parser.hh"
#include "scheduler.hh"
#include "naive_sim.hh"
#include "type_checker.hh"
#include <fstream>

int main(int argc, char** argv) {
	if(argc <= 1) {
		std::cerr << "Précisez un fichier de netlist\n";
		return 1;
	}

	std::ifstream ntlst(argv[1]);
	if(!ntlst) {
		std::cerr << "Unable to open netlist " << argv[1] << std::endl;
		return 1;
	}

	bool produce_logs = false;
	std::ofstream log_out_t;
	for(auto i = 0; i < argc; ++i) {
		if(argv[i][0] == '-' && argv[i][1] == 'D') {
			produce_logs = true;
			if(argv[i][2] != '\0') {
				log_out_t.open(&argv[i][2], std::ios::out | std::ios::trunc);
				if(!log_out_t) {
					std::cerr << "Unable to open log file " << &argv[i][2] << std::endl;
					return 1;
				}
			}
		}
	}
	
	auto& log_out = log_out_t.is_open() ? log_out_t : std::clog;

	parsed_program p;
	try{p = parse(ntlst, produce_logs, log_out);} catch(parser_error& e) {std::cerr << "Parse error : " <<  e.what() << std::endl; return 1;}
	try{type_check(p);} catch(type_error& e) {std::cerr << "Type error : " << e.what() << std::endl; return 1;}
	scheduled_program pp;
	try{pp = schedule(p, produce_logs, log_out);} catch(schedule_error& e) {std::cerr << "Scheduling error : " << e.what() << std::endl; return 1;}

	int nstep = -1;

	auto rcnt = 0u;

	std::vector<std::vector<uint64_t>> loaded_roms(pp.roms.size());
	for(auto i = 0u; i < pp.roms.size(); ++i) {
		loaded_roms[i] = std::vector<uint64_t>(1 << pp.roms[i].address_width);
	}

	std::ifstream inputs;

	for(auto i = 0; i < argc; ++i) {
		if(argv[i][0] == '-' && argv[i][1] != '\0') {
			switch(argv[i][1]) {
				case 'n':
					nstep = std::stoi(&argv[i][2]);
					break;
				case 'r':
					if(rcnt < pp.roms.size()) {
						std::ifstream fs(&argv[i][2], std::ios::in | std::ios::binary);
						if(!fs) {
							std::cerr << "Unable to open ROM file" << &argv[i][2] << std::endl;
							return 1;
						}
						auto word_size = pp.roms[rcnt].data_width;
						auto word_cnt  = loaded_roms[rcnt].size();
						if(word_size % 8 != 0) {
							std::cerr << "ROM loading where data width is not a multiple of a byte is not supported" << std::endl;
						}
						uint8_t* buffer = new uint8_t[word_cnt * word_size/8];
						fs.read(reinterpret_cast<char*>(buffer), word_cnt * word_size/8);
						for(unsigned i = 0; i < word_cnt; i += 1) {
							uint64_t eff_word = 0;
							for(unsigned j = 0; j < word_size/8; ++j) { // Words are stored little-endian
								eff_word |= (buffer[i*word_size/8 + j] << (j*8));
							}
							loaded_roms[rcnt][i] = eff_word;
						}

						delete buffer;
						log_out << "Loaded " << fs.gcount() << " bytes of ROM\n";
						rcnt++;
					} else {
						std::cerr << "Too many ROM giles given. (Expected "
							  << pp.roms.size() << ")\n";
						return 1;
					} break;
				case 'i':
					inputs = std::ifstream(&argv[i][2], std::ios::in | std::ios::binary);
					if(!inputs) {
						std::cerr << "Unable to open input file " << &argv[i][2] << std::endl;
						return 1;
					}
			}
		}
	}	

	naive_simulation(pp, loaded_roms, inputs, nstep);
	
	return 0;
}
