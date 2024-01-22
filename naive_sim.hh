#pragma once

#include "scheduler.hh"

void naive_simulation(scheduled_program pp, const std::vector<std::vector<uint64_t>>& rom_bufs,
std::istream&, 
const std::vector<unsigned>& to_output, const std::vector<unsigned>& to_clock, 
const char* preload_str,
bool fast = false, unsigned nwatch = 0, int nstep = -1);
