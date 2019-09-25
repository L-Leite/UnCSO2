#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include <uc2/pkgindex.hpp>

#include "gamedatainfo.hpp"

namespace fs = std::filesystem;

enum class GameProvider;

class DynamicIndexFileFactory
{
public:
    DynamicIndexFileFactory( const fs::path& indexFilePath,
                             std::vector<uint8_t>& indexData );
    ~DynamicIndexFileFactory();

public:
    uc2::PkgIndex::ptr_t&& GetPkgIndexOwnership() noexcept;

private:
    bool TryDetectProvider();

    static bool TestCsKeys( uc2::PkgIndex* pIndexFile );
    static bool TestTfoKeys( uc2::PkgIndex* pIndexFile );

private:
    const fs::path& m_IndexFilePath;
    std::vector<uint8_t>& m_vIndexData;
    uc2::PkgIndex::ptr_t m_pPkgIndex;
};
