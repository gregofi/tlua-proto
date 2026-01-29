#include <sstream>
#include <string>

/// Join multi-line S-expression into a single line for comparisons
inline std::string normalize(const std::string& str) {
    std::istringstream iss(str);
    std::string line, result;
    while (std::getline(iss, line)) {
        auto start = line.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) {
            auto end = line.find_last_not_of(" \t\r\n");
            if (!result.empty())
                result += " ";
            result += line.substr(start, end - start + 1);
        }
    }

    return result;
}

