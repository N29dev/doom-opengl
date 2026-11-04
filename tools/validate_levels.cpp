#include "Level.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::string defaultLevel = "assets/levels/test_arena.json";
    const int count = argc > 1 ? argc - 1 : 1;
    bool valid = true;

    for (int index = 0; index < count; ++index) {
        const std::string filePath = argc > 1 ? argv[index + 1] : defaultLevel;
        Level level;
        std::string error;
        if (!level.load(filePath, error)) {
            std::cerr << "FAIL " << filePath << ": " << error << '\n';
            valid = false;
            continue;
        }
        std::cout << "OK " << filePath << " (" << level.width()
                  << "x" << level.height() << ", reachable exit)\n";
    }

    return valid ? 0 : 1;
}
