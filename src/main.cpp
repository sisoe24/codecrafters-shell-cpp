#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> split(std::string& text, const char& delimiter = ' ') {
    std::string item;
    std::vector<std::string> items;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (c == delimiter) {
            items.push_back(item);
            item = "";
        } else {
            item += c;
        }
    }

    if (item != "") {
        items.push_back(item);
    }

    return items;
}

struct Command {
    std::string bin;
    std::vector<std::string> args;

    Command(std::string input) {
        std::vector<std::string> parts{split(input)};

        this->bin = parts.front();
        this->args = std::vector(parts.begin() + 1, parts.end());
    }
};

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::vector<std::string> commands{"type", "pwd", "echo", "exit"};

    std::cout << "$ ";

    while (true) {

        std::string input;
        std::getline(std::cin, input);

        Command command{input};

        if (command.bin == "type") {
            if (std::find(commands.begin(), commands.end(), command.args[0]) != commands.end()) {
                std::cout << command.args[0] << " is a shell builtin" << std::endl;
            } else {
                std::cout << command.args[0] << ": not found" << std::endl;
            }
        }

        else if (command.bin == "exit" && command.args[0] == "0") {
            return 0;
        }

        else if (command.bin == "echo") {
            for (std::string arg : command.args) {
                std::cout << arg << ' ';
            }
            std::cout << '\n';

        } else {

            std::cout << command.bin << ": command not found" << '\n';
        }

        std::cout << "$ ";
    }
}
