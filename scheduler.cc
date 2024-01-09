#include "scheduler.hh"
#include <list>
#include <iostream>

schedule_error::schedule_error(std::string& w) : std::logic_error(w) {};

struct graph_node {
	instr val;
	enum {VISITED, NOTVISITED, INPROGRESS} status = NOTVISITED;
	std::list<int> linked_by;
};

void treat_node(int index, std::vector<graph_node>& grph, std::vector<int>& retval) {
	grph[index].status = graph_node::INPROGRESS;
	for(auto parent_i : grph[index].linked_by) {
		switch(grph[parent_i].status) {
			case graph_node::INPROGRESS:
				{auto s = "%" + std::to_string(grph[index].val.lhs_id);
				 s += " ";
				throw schedule_error(s);}
			case graph_node::NOTVISITED:
				try { treat_node(parent_i, grph, retval);
				} catch(schedule_error& e) {
					{auto s = "%" + std::to_string(grph[index].val.lhs_id);
					s += " "; s += e.what();
					throw schedule_error(s);}
				} break;
			case graph_node::VISITED:
				break;
		}
	}
	grph[index].status = graph_node::VISITED;
	retval.push_back(index);
};

// Trash les status !
std::vector<int> topological_sort(std::vector<graph_node>& grph) {
	std::vector<int> retval;
	
	for(auto i = 0u; i < grph.size(); ++i) {
		if(grph[i].status == graph_node::NOTVISITED) {treat_node(i, grph, retval);}
	}

	return retval;
}

std::vector<uint64_t> depends_on(instr inst) {
	std::vector<uint64_t> retval;
	auto register_dep = [&](arg to_reg){
		if(!to_reg.is_cst){retval.push_back(to_reg.value);}};

	switch(inst.op) {
		case MUX:
			register_dep(inst.argv[2]);
			[[fallthrough]];
		case OR:
		case XOR:
		case AND:
		case NAND:
		case CONCAT:
			register_dep(inst.argv[1]);
			[[fallthrough]];
		case COPY:
		case NOT:
			register_dep(inst.argv[0]);
			break;

		case ROMFETCH:
		case RAMFETCH:
		case SELECT:
			register_dep(inst.argv[1]); break;

		case SLICE:
			register_dep(inst.argv[2]); break;
		case REG:
			break;	
	}
	return retval;
}

scheduled_program schedule(parsed_program pp, bool debug, std::ostream& log_out) {
	std::vector<graph_node> to_sort(pp.code.size());

	std::vector<int> index_of_setting_instr(pp.idents.size(), -1); // Index de l'instruction qui set, -1 si y'en a pas 
	for(auto i = 0u; i < pp.code.size(); ++i) {
		auto lhs_id = pp.code[i].lhs_id;
		index_of_setting_instr[lhs_id] = i;
	}

	for(auto i = 0u; i < pp.code.size(); ++i) {
		auto inst = pp.code[i];
		auto dependencies = depends_on(inst);
		to_sort[i].val = inst;
		to_sort[i].status = graph_node::NOTVISITED;
		for(auto dep : dependencies) {
			if(index_of_setting_instr[dep] != -1) {
				to_sort[i].linked_by.push_back(index_of_setting_instr[dep]);
			}
		}
	}

	auto sorted = topological_sort(to_sort);

	decltype(pp.code) new_code(pp.code.size());
	for(auto i = 0u; i < pp.code.size(); ++i) {
		new_code[i] = pp.code[sorted[i]];
	}
	pp.code = new_code;

	if(debug) {
		log_out << "Scheduled code :\n";
		for(auto inst : pp.code) {
			log_out << '%' << inst.lhs_id << " = ";
			if(inst.op == COPY) log_out << "COPY " << inst.argv[0]; 
			if(inst.op == REG) log_out << "REG "   << inst.argv[0]; 
			if(inst.op == OR) log_out << "OR "     << inst.argv[0] << ' ' << inst.argv[1];
			if(inst.op == XOR) log_out << "XOR "   << inst.argv[0] << ' ' << inst.argv[1]; 
			if(inst.op == NOT) log_out << "NOT "   << inst.argv[0]; 
			if(inst.op == AND) log_out << "AND "   << inst.argv[0] << ' ' << inst.argv[1];
			if(inst.op == NAND) log_out << "NAND " << inst.argv[0] << ' ' << inst.argv[1]; 
			if(inst.op == ROMFETCH) log_out << "ROMFETCH " << inst.argv[0] << ' ' << inst.argv[1]; 
			if(inst.op == RAMFETCH) log_out << "RAMFETCH " << inst.argv[0] << ' ' << inst.argv[1]; 
			if(inst.op == CONCAT) log_out << "CONCAT " << inst.argv[0] << ' ' << inst.argv[1]; 
			if(inst.op == SLICE) log_out << "SLICE " << inst.argv[0] << ' ' << inst.argv[1] << ' ' << inst.argv[2]; 
			if(inst.op == SELECT) log_out << "SELECT " << inst.argv[0] << ' ' << inst.argv[1];
			log_out << '\n';
		}
	}

	return (scheduled_program)pp;
}
