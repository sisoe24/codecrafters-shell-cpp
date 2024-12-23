#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

std::vector<std::string> split(const std::string& text, const char& delimiter = ' ') {
    std::string item;
    std::vector<std::string> items;

    bool openQuote = false;
    bool openSingleQuote = false;
    bool openDoubleQuote = false;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (c == '\'' && !openDoubleQuote) {
            openSingleQuote = !openSingleQuote;
            openQuote = !openQuote;
        } else if (c == '"') {
            openDoubleQuote = !openDoubleQuote;
            openQuote = !openQuote;
        }

        if (c == delimiter && !openQuote) {
            if ((openQuote && item == "") || item != "") {
                items.push_back(item);
                item = "";
            }
        } else {
            item += c;
        }
    }

    if (item != "") {
        items.push_back(item);
    }

    // for (std::string& i : items) {
    //     std::cout << "i: " << i << '\n';
    // }

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

    std::string toStr() const {
        std::string execCommand = this->bin + " ";
        for (const std::string& arg : this->args) {
            execCommand += arg + " ";
        }
        return execCommand.c_str();
    }
};

std::string getBinPath(const std::string bin, const std::vector<std::string> paths) {
    for (const std::string& path : paths) {
        const std::string binPath = path + "/" + bin;
        if (std::filesystem::exists(binPath)) {
            return binPath;
        }
    }
    return "";
}

std::string stripQuotes(const std::string& str) {
    if (str.size() <= 2) {
        return str;
    }

    if ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\'')) {
        return str.substr(1, str.size() - 2);
    }

    return str;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::vector<std::string> commands{"type", "pwd", "echo", "exit"};

    const char* path = getenv("PATH");
    const std::vector<std::string> paths = split(path, ':');

    std::cout << "$ ";

    while (true) {

        std::string input;
        std::getline(std::cin, input);

        Command command{input};

        std::string firstArg = "";
        if (command.args.size() > 0) {
            // we naivly assume that we care only for the first argument;
            firstArg = command.args.front();
        }

        if (command.bin == "type") {

            if (firstArg == "") {
                return 1;
            }

            if (std::find(commands.begin(), commands.end(), firstArg) != commands.end()) {
                std::cout << firstArg << " is a shell builtin" << '\n';
            } else {
                const std::string binPath = getBinPath(firstArg, paths);
                if (binPath != "") {
                    std::cout << firstArg << " is " << binPath << '\n';
                } else {
                    std::cout << firstArg << ": not found" << '\n';
                }
            }
        }

        else if (command.bin == "exit") {
            return std::stoi(firstArg);
        }

        else if (command.bin == "pwd") {
            std::cout << std::filesystem::current_path().string() << '\n';
        }

        else if (command.bin == "cd") {
            if (firstArg == "" || firstArg == "~") {
                // technically speaking the env might not be set but we dont care right now
                std::filesystem::current_path(getenv("HOME"));
            } else if (chdir(firstArg.c_str()) != 0) {
                std::cout << "cd: " << firstArg << ": No such file or directory" << '\n';
            }

        } else if (command.bin == "echo") {
            for (std::string arg : command.args) {
                std::cout << stripQuotes(arg) << ' ';
            }
            std::cout << '\n';

        } else {
            if (getBinPath(command.bin, paths) != "") {
                std::system(command.toStr().c_str());
            } else {
                std::cout << command.bin << ": command not found" << '\n';
            }
        }

        std::cout << "$ ";
    }
}
