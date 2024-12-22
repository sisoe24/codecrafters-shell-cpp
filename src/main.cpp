#include <iostream>

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "$ ";

    while (true) {
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit 0") {
            return 0;
        }

        std::cout << input << ": command not found" << '\n';
        std::cout << "$ ";
    }
}
