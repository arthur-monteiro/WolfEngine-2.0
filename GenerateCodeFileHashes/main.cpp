#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <xxh64.hpp>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Need path as argument\n";
        return EXIT_FAILURE;
    }

    std::cout << "Computing hashes..\n";

	const std::string path = argv[1];
    const std::string outputFilepath = path + "/CodeFileHashes.h";

    std::cout <<path << "\n";
	std::ofstream outFile(outputFilepath);
    outFile << "// -- AUTO GENERATED FILE --\n\n";
    outFile << "#pragma once\n";
    outFile << "#include <cstdint>\n\n";
    outFile << "namespace Wolf\n"; 
    outFile << "{\n"; 

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::string filepath = entry.path().generic_string();

        std::string extension = filepath.substr(filepath.find_last_of('.') + 1);
        if (extension != "h" && extension != "cpp")
            continue;

        std::string filename = filepath.substr(filepath.find_last_of('/') + 1);
        std::string macroName = "HASH_";
        for (const char character : filename)
        {
	        if (macroName != "HASH_" && std::isupper(character))
                macroName += "_";
            if (character == '.')
            {
                macroName += "_";
                continue;
            }

            macroName += static_cast<char>(std::toupper(static_cast<int>(character)));
        }

        std::ifstream codeFileStream(filepath);
        std::string codeFileString((std::istreambuf_iterator<char>(codeFileStream)), std::istreambuf_iterator<char>());

        uint64_t hash = xxh64::hash(codeFileString.c_str(), codeFileString.size(), 0);

        outFile << "\tconstexpr uint64_t " << macroName << " = " << hash << "ULL;\n";

        std::cout << filename << " : " << hash << '\n';
    }

    outFile << "}\n";
}
