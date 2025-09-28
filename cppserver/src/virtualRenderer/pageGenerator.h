#ifndef PAGEGENERATOR_H
#define PAGEGENERATOR_H

#include <string>
#include <filesystem>

// Generate a serialized binary protobuf page for a user
std::string generatePage(const std::string& user,
                         const std::filesystem::path& exeDir);

#endif // PAGEGENERATOR_H
