#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const char SINGLE_QUOTE = '\'';
const char DOUBLE_QUOTE = '"';
const char ESCAPE_CHAR = '\\';

std::vector<std::string> split(const std::string& text, const char& delimiter = ' ') {
    std::string item;
    std::vector<std::string> items;

    bool openQuote = false;
    bool openSingleQuote = false;
    bool openDoubleQuote = false;
    bool openEscape = false;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (c == ESCAPE_CHAR) {
            openEscape = true;
            char nextChar = text[i + 1];

            if (!openQuote) {
                c = nextChar;
                i++;
            } else if (!openSingleQuote) {
                if (nextChar == DOUBLE_QUOTE || nextChar == '$' || nextChar == ESCAPE_CHAR) {
                    c = nextChar;
                    i++;
                }
            }
        }

        if (!openEscape && c == SINGLE_QUOTE && !openDoubleQuote) {
            openSingleQuote = !openSingleQuote;
            openQuote = !openQuote;
            continue;
        }

        if (!openEscape && c == DOUBLE_QUOTE && !openSingleQuote) {
            openDoubleQuote = !openDoubleQuote;
            openQuote = !openQuote;
            continue;
        }

        if (c == delimiter && !openQuote) {
            if ((openEscape && item == "") || item != "") {
                items.push_back(item);
                item = "";
            }
        } else {
            item += c;
        }

        openEscape = false;
    }

    if (item != "") {
        items.push_back(item);
    }

    // for (std::string& i : items) {
    //     std::cout << "  - " << i << '\n';
    // }

    return items;
}

int file_fd = -1;
int stdout_fd = dup(STDOUT_FILENO);
int stderr_fd = dup(STDERR_FILENO);

bool isRedirect(const std::string& c) {
    return c == ">" || c == "1>" || c == ">>" || c == "1>>" || c == "2>" || c == "2>>";
}

struct Command {
    std::string bin;
    std::vector<std::string> args;
    bool redirect = false;

    Command(std::string input) {
        std::vector<std::string> parts{split(input)};

        this->bin = parts.front();
        this->args = std::vector(parts.begin() + 1, parts.end());

        std::string output;
        std::string redirect;
        std::vector<std::string> args;

        for (size_t i = 0; i < this->args.size(); ++i) {
            const std::string arg = this->args[i];
            if (isRedirect(arg)) {
                redirect = arg;
                output = this->args[i + 1];
                break;
            } else {
                args.push_back(arg);
            }
        }

        if (redirect != "") {

            this->redirect = true;

            if (redirect == ">" || redirect == "1>") {
                file_fd = open(output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(file_fd, STDOUT_FILENO);

            } else if (redirect == ">>" || redirect == "1>>") {
                file_fd = open(output.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                dup2(file_fd, STDOUT_FILENO);

            } else if (redirect == "2>") {
                file_fd = open(output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(file_fd, STDERR_FILENO);

            } else if (redirect == "2>>") {
                file_fd = open(output.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                dup2(file_fd, STDERR_FILENO);
            }
        }

        this->args = args;
    }

    int execute() {
        pid_t pid = fork();

        if (pid == -1) {
            return -1;
        }

        if (pid == 0) {
            std::vector<char*> cargs;
            cargs.push_back(const_cast<char*>(this->bin.c_str()));

            for (const std::string& arg : this->args) {
                cargs.push_back(const_cast<char*>(arg.c_str()));
            }
            cargs.push_back(nullptr);

            execvp(this->bin.c_str(), cargs.data());
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        }
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
                    std::cerr << firstArg << ": not found" << '\n';
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
                std::cerr << "cd: " << firstArg << ": No such file or directory" << '\n';
            }

        } else if (command.bin == "echo") {
            for (std::string arg : command.args) {
                std::cout << arg + " ";
            }
            std::cout << '\n';

        } else {
            if (getBinPath(command.bin, paths) != "") {
                command.execute();
            } else {
                std::cerr << command.bin << ": command not found" << '\n';
            }
        }

        if (command.redirect) {
            dup2(stdout_fd, STDOUT_FILENO);
            dup2(stderr_fd, STDERR_FILENO);
            close(file_fd);
        }

        std::cout << "$ ";
    }
}
