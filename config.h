#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>

class InputError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct GameConfig {
    std::uint64_t first;
    std::uint64_t second;
    std::optional<std::uint32_t> seed;
};

// input file is one line like 7 4 31, two numbers from 1 to 10^12 and an optional seed from 0 to 4294967295
GameConfig readConfig(const std::filesystem::path& path);