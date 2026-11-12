#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "task.h"

// a player can add 1, double a number or divide it by 3 if it divides evenly
enum class Action {
    Add,
    Double,
    Divide
};

struct Move {
    Action action;
    std::size_t index;
};

struct MoveResult {
    Move move;
    std::uint64_t before;
    std::uint64_t after;
};

class Board {
public:
    static constexpr std::size_t COUNT = 2;
    static constexpr std::uint64_t FACTOR = 10;
    static constexpr std::uint64_t MAX_START = 1'000'000'000'000;

    Board(std::uint64_t first, std::uint64_t second);

    bool canDo(Move move) const;
    MoveResult make(Move move);

    std::uint64_t value(std::size_t index) const {
        return nums_.at(index);
    }

    std::uint64_t sum() const {
        return nums_[0] + nums_[1];
    }

    std::uint64_t startSum() const {
        return startSum_;
    }

    std::uint64_t goal() const {
        return startSum_ * FACTOR;
    }

    // the game is won when the sum gets 10 times bigger than at the start
    bool isWon() const {
        return sum() >= goal();
    }

private:
    std::array<std::uint64_t, COUNT> nums_;
    std::uint64_t startSum_;
};

// picks a random move, the best strategy is not needed here
class RandomStrategy {
public:
    explicit RandomStrategy(std::uint32_t seed)
        : rng_(seed) {}

    // a player can't do the same action twice in a row
    Move pick(const Board& board, std::optional<Action> last);

private:
    std::mt19937 rng_;
};

class Protocol {
public:
    explicit Protocol(std::ostream& out)
        : out_(out) {}

    void printHeader(const Board& board, std::uint32_t seed);
    void printMove(const std::string& player, const MoveResult& result, const Board& board);
    void print(const std::string& message);

    std::size_t moves() const {
        return moves_;
    }

private:
    std::ostream& out_;
    std::size_t moves_ = 0;
    int width_ = 1;
};

// each player makes one move per turn, the winner prints a message and stops
Task play(std::string name, RandomStrategy strategy, Board& board, Protocol& log);

// starts the game and lets the three players move in turns
class Game {
public:
    static constexpr std::size_t PLAYERS = 3;
    static constexpr std::size_t MAX_MOVES = 100'000;

    Game(std::uint64_t first, std::uint64_t second, std::uint32_t seed, std::ostream& out);

    void run();

private:
    Board board_;
    Protocol log_;
    std::uint32_t seed_;
    std::vector<std::string> names_;
    std::vector<Task> players_;
    bool played_ = false;
};