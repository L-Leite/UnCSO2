#include "fileproperties.hpp"

#include <uc2/pkgentry.hpp>

FileProperties::FileProperties()
    : m_GameDataInfo(), m_iFileEntries( 0 ), m_iPkgFilesNum( 0 ),
      m_iEncryptedFiles( 0 ), m_iPlainFiles( 0 ), m_Md5Hash()
{
}

FileProperties::~FileProperties() {}

void FileProperties::Reset()
{
    this->m_GameDataInfo.Reset();
    this->m_iFileEntries = 0;
    this->m_iPkgFilesNum = 0;
    this->m_iEncryptedFiles = 0;
    this->m_iPlainFiles = 0;
    this->m_Md5Hash.fill( '\0' );
}

void FileProperties::SetPkgFileProperties(
    GameProvider provider, gsl::not_null<uc2::PkgFile*> pPkgFile )
{
    this->SetProvider( provider );
    this->SetMd5Hash( pPkgFile->GetMd5Hash() );

    auto& pkgEntries = pPkgFile->GetEntries();

    const uint64_t iFileEntries = pkgEntries.size();
    uint64_t iPlainFiles = 0;
    uint64_t iEncryptedFiles = 0;

    for ( auto&& entry : pkgEntries )
    {
        if ( entry->IsEncrypted() == true )
        {
            iEncryptedFiles++;
        }
    }

    iPlainFiles = iFileEntries - iEncryptedFiles;

    this->m_iFileEntries = iFileEntries;
    this->m_iEncryptedFiles = iEncryptedFiles;
    this->m_iPlainFiles = iPlainFiles;
}

void FileProperties::SetIndexFileProperties(
    GameProvider provider,
    const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>& pkgFiles )
{
    this->SetProvider( provider );

    uint64_t iFileEntries = 0;
    uint64_t iEncryptedFiles = 0;
    uint64_t iPlainFiles = 0;

    for ( auto&& pkgPair : pkgFiles )
    {
        iFileEntries += pkgPair.second->GetEntries().size();

        for ( auto&& entry : pkgPair.second->GetEntries() )
        {
            if ( entry->IsEncrypted() == true )
            {
                iEncryptedFiles++;
            }
            else
            {
                iPlainFiles++;
            }
        }
    }

    this->m_iPkgFilesNum = pkgFiles.size();
    this->m_iFileEntries = iFileEntries;
    this->m_iEncryptedFiles = iEncryptedFiles;
    this->m_iPlainFiles = iPlainFiles;
}
