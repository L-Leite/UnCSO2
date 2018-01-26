#pragma once

#include <filesystem>

// vs 2017's filesystem header wasnt standard on release
// more info at https://docs.microsoft.com/en-us/cpp/standard-library/filesystem
#ifdef _MSC_VER
namespace std
{
namespace filesystem = std::experimental::filesystem;
}
#endif
