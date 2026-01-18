#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <print>

#include "lexer.h"

std::string readFileContents(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return contents;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source-file>\n";
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    bool tokenize = std::ranges::find(args, "--tokenize") != args.end();

    auto sourceFileIt = std::ranges::find_if(args, [](const std::string& arg) {
        return !arg.starts_with("--");
    });
    if (sourceFileIt == args.end()) {
        std::cerr << "Error: No source file provided.\n";
        return 1;
    }

    std::string sourceFile = *sourceFileIt;
    std::string sourceCode = readFileContents(sourceFile);

    if (tokenize) {
        auto tokens = Lexer::tokenize(sourceCode);
        for (const auto& token : tokens) {
            std::print("[{}] ", tokenKindToStr(token.kind));
        }
        std::cout << std::endl;
    }
}
