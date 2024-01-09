#pragma once

#include "parser.hh"

struct scheduled_program : public parsed_program {};

struct schedule_error : public std::logic_error {
	schedule_error(std::string& w);
};

scheduled_program schedule(parsed_program, bool, std::ostream&); 
