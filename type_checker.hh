#pragma once
#include "parser.hh"


struct type_error     : public std::logic_error {type_error(std::string&);};
struct illegal_length : public type_error {illegal_length(std::string&);};
struct incompatible_lengths : public type_error {incompatible_lengths(std::string&);};
void type_check(const parsed_program&);
