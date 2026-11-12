#include "game.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace {

constexpr Action ACTIONS[] = {Action::Add, Action::Double, Action::Divide};

const char* symbol(Action action) {
    switch (action) {
        case Action::Add:    return "+ 1";
        case Action::Double: return "* 2";
        case Action::Divide: return "/ 3";
    }
    return "?";
}

std::string winMessage(const std::string& name, const Board& board) {
    const std::string sum = std::to_string(board.sum());
    const std::string goal = std::to_string(board.goal());
    return name + ": I won! Sum " + sum + " >= " + goal + ". Finishing.";
}

std::uint32_t seedFor(std::uint32_t gameSeed, std::size_t player) {
    std::seed_seq sequence{gameSeed, static_cast<std::uint32_t>(player)};
    std::uint32_t seed = 0;
    sequence.generate(&seed, &seed + 1);
    return seed;
}

}

Board::Board(std::uint64_t first, std::uint64_t second) : nums_{first, second}, startSum_{first + second} {
    for (std::uint64_t num : nums_) {
        if (num == 0 || num > MAX_START) {
            throw std::invalid_argument("Numbers must be in range [1; " + std::to_string(MAX_START) + "]");
        }
    }
}

// dividing by 3 is only allowed when the number divides evenly
bool Board::canDo(Move move) const {
    if (move.index >= COUNT) {
        return false;
    }
    return move.action != Action::Divide || nums_[move.index] % 3 == 0;
}

// does one of the three actions with the chosen number
MoveResult Board::make(Move move) {
    if (!canDo(move)) {
        throw std::logic_error("Illegal move");
    }
    std::uint64_t& num = nums_[move.index];
    const std::uint64_t before = num;
    switch (move.action) {
        case Action::Add:    num += 1; break;
        case Action::Double: num *= 2; break;
        case Action::Divide: num /= 3; break;
    }
    return {move, before, num};
}

// collects all moves that are allowed right now and picks one of them
Move RandomStrategy::pick(const Board& board, std::optional<Action> last) {
    std::vector<Move> options;
    for (Action action : ACTIONS) {
        for (std::size_t i = 0; i < Board::COUNT; ++i) {
            const Move move{action, i};
            if (action != last && board.canDo(move)) {
                options.push_back(move);
            }
        }
    }
    std::uniform_int_distribution<std::size_t> random(0, options.size() - 1);
    return options[random(rng_)];
}

void Protocol::printHeader(const Board& board, std::uint32_t seed) {
    width_ = static_cast<int>(std::to_string(board.goal() * 2).size());
    out_ << "Game of three coroutines\n";
    out_ << "Initial numbers: " << board.value(0) << " and " << board.value(1) << '\n';
    out_ << "Initial sum: " << board.startSum() << ", goal: sum >= " << board.goal() << '\n';
    out_ << "Seed: " << seed << "\n\n";
}

// one line of the game log
void Protocol::printMove(const std::string& player, const MoveResult& result, const Board& board) {
    ++moves_;
    const Move& move = result.move;
    out_ << std::setw(5) << moves_ << ". " << player << ":  ";
    out_ << 'x' << move.index + 1 << ' ' << symbol(move.action) << "   ";
    out_ << std::setw(width_) << result.before << " -> ";
    out_ << std::left << std::setw(width_) << result.after << std::right << "   ";
    out_ << "numbers (" << board.value(0) << ", " << board.value(1) << "), sum " << board.sum() << '\n';
}

void Protocol::print(const std::string& message) {
    out_ << '\n' << message << '\n';
}

// the player coroutine
Task play(std::string name, RandomStrategy strategy, Board& board, Protocol& log) {
    std::optional<Action> lastAction;

    while (true) {
        const Move move = strategy.pick(board, lastAction);
        log.printMove(name, board.make(move), board);
        lastAction = move.action;

        // the winner prints a message and stops
        if (board.isWon()) {
            log.print(winMessage(name, board));
            co_return;
        }
        co_await std::suspend_always{};  // next player's turn
    }
}

// creates three player coroutines
Game::Game(std::uint64_t first, std::uint64_t second, std::uint32_t seed, std::ostream& out)
    : board_(first, second), log_(out), seed_(seed) {
    for (std::size_t i = 0; i < PLAYERS; ++i) {
        names_.push_back("Coroutine " + std::to_string(i + 1));
        players_.push_back(play(names_.back(), RandomStrategy(seedFor(seed, i)), board_, log_));
    }
}

void Game::run() {
    if (played_) {
        throw std::logic_error("Game has already been played");
    }
    played_ = true;

    log_.printHeader(board_, seed_);
    // players move in turns until one of them wins
    for (std::size_t move = 0; move < MAX_MOVES; ++move) {
        const std::size_t current = move % PLAYERS;
        players_[current].resume();
        if (players_[current].done()) {
            const std::string moves = std::to_string(log_.moves());
            log_.print("Winner: " + names_[current] + " (total moves: " + moves + ")");
            return;
        }
    }
    log_.print("Game stopped: no winner after " + std::to_string(MAX_MOVES) + " moves.");
}