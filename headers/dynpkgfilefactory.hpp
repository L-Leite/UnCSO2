#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include <uc2/pkgfile.hpp>

#include "gamedatainfo.hpp"

namespace fs = std::filesystem;

enum class GameProvider;

class DynamicPkgFileFactory
{
public:
    DynamicPkgFileFactory( const fs::path& pkgFilePath );
    DynamicPkgFileFactory( const fs::path& pkgFilePath, GameProvider provider );
    ~DynamicPkgFileFactory();

public:
    uc2::PkgFile::ptr_t&& GetPkgFileOwnership() noexcept;
    inline GameProvider GetProvider() const noexcept;

private:
    bool LoadBaseFileHeader();
    bool LoadFullFileHeader();

    bool TryDetectProvider();
    bool TrySpecificProvider( GameProvider provider );

    static std::pair<bool, GameProvider> TestCsKeys(
        uc2::PkgFile* pPkgFile, const std::vector<uint8_t>& vPkgFileData );
    static bool TestTfoKeys( uc2::PkgFile* pPkgFile );

private:
    const fs::path& m_PkgFilePath;

    std::vector<uint8_t> m_vPkgFileData;
    GameProvider m_DetectedProvider;

    uc2::PkgFile::ptr_t m_pPkgFile;
};

inline GameProvider DynamicPkgFileFactory::GetProvider() const noexcept
{
    return this->m_DetectedProvider;
}
