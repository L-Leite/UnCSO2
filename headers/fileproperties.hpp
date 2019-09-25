#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <gsl/gsl>
#include <unordered_map>

namespace fs = std::filesystem;

#include <uc2/pkgfile.hpp>

#include "gamedatainfo.hpp"

class FileProperties
{
public:
    FileProperties();
    ~FileProperties();

public:
    inline const GameDataInfo& GetGameDataInfo() const;
    inline uint64_t GetPkgFilesNum() const;
    inline uint64_t GetFileEntries() const;
    inline uint64_t GetEncryptedFiles() const;
    inline uint64_t GetPlainFiles() const;
    inline std::string_view GetMd5Hash() const;

    void SetPkgFileProperties( GameProvider provider,
                               gsl::not_null<uc2::PkgFile*> pPkgFile );
    void SetIndexFileProperties(
        GameProvider provider,
        const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>& pkgFiles );

    void Reset();

private:
    inline void SetProvider( GameProvider provider );
    inline void SetMd5Hash( std::string_view md5HashView );

private:
    GameDataInfo m_GameDataInfo;

    uint64_t m_iFileEntries;
    uint64_t m_iPkgFilesNum;
    uint64_t m_iEncryptedFiles;
    uint64_t m_iPlainFiles;

    std::array<char, 32 + 1> m_Md5Hash;
};

inline const GameDataInfo& FileProperties::GetGameDataInfo() const
{
    return this->m_GameDataInfo;
}

inline uint64_t FileProperties::GetPkgFilesNum() const
{
    return this->m_iPkgFilesNum;
}

inline uint64_t FileProperties::GetFileEntries() const
{
    return this->m_iFileEntries;
}

inline uint64_t FileProperties::GetEncryptedFiles() const
{
    return this->m_iEncryptedFiles;
}

inline uint64_t FileProperties::GetPlainFiles() const
{
    return this->m_iPlainFiles;
}

inline std::string_view FileProperties::GetMd5Hash() const
{
    return this->m_Md5Hash.data();
}

inline void FileProperties::SetProvider( GameProvider provider )
{
    this->m_GameDataInfo = GameDataInfo::CreateWithProvider( {}, provider );
}

inline void FileProperties::SetMd5Hash( std::string_view md5HashView )
{
    assert( md5HashView.size() + 1 == this->m_Md5Hash.max_size() );
    std::copy( md5HashView.begin(), md5HashView.end(), this->m_Md5Hash.data() );
    this->m_Md5Hash[this->m_Md5Hash.max_size() - 1] = '\0';
}
