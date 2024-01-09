#pragma once

#include "scheduler.hh"

void naive_simulation(scheduled_program pp, const std::vector<std::vector<uint64_t>>& rom_bufs, std::istream&, int nstep = -1);