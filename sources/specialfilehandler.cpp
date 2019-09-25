#include "specialfilehandler.hpp"

#include <stdexcept>

#include <uc2/encryptedfile.hpp>
#include <uc2/lzmatexture.hpp>

#include <qlogging.h>

#include "indexkeycollections.hpp"
#include "miscutils.hpp"

SpecialFileHandler::SpecialFileHandler( gsl::span<std::uint8_t> fileData,
                                        fs::path filePath, bool canDecrypt,
                                        bool canDecompress )
    : m_FileData( fileData ), m_FilePath( filePath ),
      m_bAllowDecryption( canDecrypt ), m_bAllowDecompression( canDecompress )
{
}

SpecialFileHandler::~SpecialFileHandler() {}

gsl::span<std::uint8_t> SpecialFileHandler::ProcessData()
{
    if ( this->m_bAllowDecryption && this->IsFileEncrypted() )
    {
        this->m_FileData = this->DecryptFile();
    }

    if ( this->m_bAllowDecompression && this->IsTextureCompressed() )
    {
        this->m_FileData = this->DecompressTexture();
    }

    return this->m_FileData;
}

bool SpecialFileHandler::IsFileEncrypted() const
{
    bool bIsEncryptedFile = uc2::EncryptedFile::IsEncryptedFile(
        this->m_FileData.data(), this->m_FileData.size_bytes() );
    return bIsEncryptedFile;
}

bool SpecialFileHandler::IsTextureCompressed() const
{
    bool bIsCompressedTex = uc2::LzmaTexture::IsLzmaTexture(
        this->m_FileData.data(), this->m_FileData.size_bytes() );
    return bIsCompressedTex;
}

gsl::span<std::uint8_t> SpecialFileHandler::DecryptFile()
{
    try
    {
        std::string szFilename = this->m_FilePath.filename().generic_string();
        auto pEncFile = uc2::EncryptedFile::Create(
            szFilename, this->m_FileData.data(), this->m_FileData.size_bytes(),
            CS_INDEX_KEY_COLLECTION );
        auto res = PairToSpan( pEncFile->Decrypt() );

        if ( res.empty() == false )
        {
            SpecialFileHandler::FixDecryptedExtension( this->m_FilePath );
            return res;
        }
    }
    catch ( const std::exception& e )
    {
        qDebug( e.what() );
    }

    return {};
}

gsl::span<std::uint8_t> SpecialFileHandler::DecompressTexture() const
{
    try
    {
        auto pTexFile = uc2::LzmaTexture::Create(
            this->m_FileData.data(), this->m_FileData.size_bytes() );

        uint64_t iNewTexSize = pTexFile->GetOriginalSize();
        uint8_t* pNewTexBuffer = new uint8_t[iNewTexSize];

        if ( pTexFile->Decompress( pNewTexBuffer, iNewTexSize ) == true )
        {
            return { pNewTexBuffer, iNewTexSize };
        }
        else
        {
            delete pNewTexBuffer;
        }
    }
    catch ( const std::exception& e )
    {
        qDebug( e.what() );
    }

    return {};
}

void SpecialFileHandler::FixDecryptedExtension( fs::path& filePath )
{
    std::string szExtension = filePath.extension().generic_string();

    auto pos = szExtension.find_first_of( 'e' );

    if ( pos != std::string::npos )
    {
        std::string szFixedExtension = szExtension.substr( pos + 1 );
        filePath.replace_extension( szFixedExtension );
    }
}
