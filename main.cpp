#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>

#include "config.h"
#include "game.h"

namespace {

    constexpr const char* DEFAULT_FILE = "input.txt";

    std::uint32_t randomSeed() {
        std::random_device device;
        return device();
    }

}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [input_file]\n";
        return EXIT_FAILURE;
    }
    const char* path = argc == 2 ? argv[1] : DEFAULT_FILE;

    // read the input file and play the game, errors in the input are printed
    try {
        const GameConfig config = readConfig(path);
        Game game(config.first, config.second, config.seed.value_or(randomSeed()), std::cout);
        game.run();
    } catch (const InputError& error) {
        std::cerr << "Input error: " << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "Internal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}