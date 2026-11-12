#include "config.h"

#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "game.h"

namespace {

struct Range {
    std::uint64_t min;
    std::uint64_t max;
};

struct Line {
    std::size_t number;
    std::vector<std::string> words;
};

class ConfigReader {
public:
    explicit ConfigReader(std::filesystem::path path)
        : path_(std::move(path)) {}

    // reads two numbers and an optional seed from the input file
    GameConfig read() {
        const Line line = readLine();
        const std::size_t count = line.words.size();
        if (count != 2 && count != 3) {
            fail(line.number, "expected 2 or 3 values, found " + std::to_string(count));
        }

        GameConfig config{};
        config.first = readNumber(line, 0, "first number", {1, Board::MAX_START});
        config.second = readNumber(line, 1, "second number", {1, Board::MAX_START});
        if (count == 3) {
            const std::uint64_t maxSeed = std::numeric_limits<std::uint32_t>::max();
            config.seed = static_cast<std::uint32_t>(readNumber(line, 2, "seed", {0, maxSeed}));
        }
        return config;
    }

private:
    [[noreturn]] void fail(const std::string& message) const {
        throw InputError(path_.string() + ": " + message);
    }

    [[noreturn]] void fail(std::size_t line, const std::string& message) const {
        fail("line " + std::to_string(line) + ": " + message);
    }

    // the file must contain exactly one line with data
    Line readLine() const {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path_, error)) {
            fail("file does not exist or is not a regular file");
        }
        std::ifstream file(path_);
        if (!file) {
            fail("cannot open file for reading");
        }

        std::optional<Line> data;
        std::string text;
        for (std::size_t number = 1; std::getline(file, text); ++number) {
            std::vector<std::string> words = split(text, number == 1);
            if (words.empty()) {
                continue;
            }
            if (data) {
                fail(number, "extra data line, data is already on line " + std::to_string(data->number));
            }
            data = Line{number, std::move(words)};
        }
        if (file.bad()) {
            fail("error while reading file");
        }
        if (!data) {
            fail("file contains no data (expected \"<number> <number> [seed]\")");
        }
        return *data;
    }

    static std::vector<std::string> split(std::string text, bool firstLine) {
        const std::string bom = "\xEF\xBB\xBF";
        if (firstLine && text.rfind(bom, 0) == 0) {
            text.erase(0, bom.size());
        }
        if (const auto comment = text.find('#'); comment != std::string::npos) {
            text.erase(comment);
        }
        std::istringstream stream(text);
        std::vector<std::string> words;
        for (std::string word; stream >> word;) {
            words.push_back(std::move(word));
        }
        return words;
    }

    // checks that the value is a whole number in the allowed range
    std::uint64_t readNumber(const Line& line, std::size_t pos, const std::string& name, Range range) const {
        const std::string& word = line.words[pos];
        const std::string prefix = name + " \"" + word + "\" ";
        const std::string allowed = "[" + std::to_string(range.min) + "; " + std::to_string(range.max) + "]";

        if (word.front() == '-') {
            fail(line.number, prefix + "cannot be negative; allowed range is " + allowed);
        }
        std::uint64_t number = 0;
        const char* end = word.data() + word.size();
        const auto [stop, error] = std::from_chars(word.data(), end, number);
        if (error == std::errc::invalid_argument || stop != end) {
            fail(line.number, prefix + "is not an integer");
        }
        if (error == std::errc::result_out_of_range || number < range.min || number > range.max) {
            fail(line.number, prefix + "is out of range " + allowed);
        }
        return number;
    }

    std::filesystem::path path_;
};

}

GameConfig readConfig(const std::filesystem::path& path) {
    return ConfigReader(path).read();
}