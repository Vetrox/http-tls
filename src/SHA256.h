#pragma once

#include <cstddef>
#include "stdint.h"
#include <vector>
#include <array>

#include <string>

size_t find_l_k_solution(size_t length);
std::vector<bool> add_padding(std::vector<bool> message);
std::array<uint32_t, 8> sha256_hash(std::vector<bool> const& message);
std::array<uint32_t, 8> sha256_hash(std::vector<uint8_t> const& message);

std::string sha256_as_hex(std::array<uint32_t, 8> hash);
