#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <exception>

struct arg {
	bool is_cst;
	uint64_t value; 
};

std::ostream& operator<<(std::ostream& o, arg& a);

enum opcode {COPY, REG, OR, XOR, NOT, AND, NAND, ROMFETCH, RAMFETCH, CONCAT, SLICE, SELECT, MUX};

struct instr {
	uint64_t lhs_id;
	opcode op;
	arg argv[3];
};

/* Utilisé pour l'update / le chargement des ROM et RAM */
struct RAM {
	unsigned int address_width;
	unsigned int data_width;

	arg we_node;
	arg waddr_node;
	arg wdata_node;
};

struct ROM {
	unsigned int address_width;
	unsigned int data_width;
};

struct parsed_program { 
	unsigned int input_number;
	unsigned int output_number;

	std::vector<std::string> idents;
	std::vector<unsigned int> id_widths;

	std::vector<instr> code;
	std::vector<RAM>   rams;
	std::vector<ROM>   roms;
};

parsed_program parse(std::istream&, bool, std::ostream&);

struct parser_error : public std::logic_error {
	parser_error(std::string &w);
};

struct undeclared_id : public parser_error {
	undeclared_id(std::string &w);
};

struct invalid_cst : public parser_error {
	invalid_cst(std::string &w);
};
