#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include <gsl/gsl>

namespace fs = std::filesystem;

class SpecialFileHandler
{
public:
    SpecialFileHandler( gsl::span<std::uint8_t> fileData, fs::path filePath,
                        bool canDecrypt, bool canDecompress );
    ~SpecialFileHandler();

public:
    gsl::span<std::uint8_t> ProcessData();
    inline fs::path GetNewFilePath() const;

private:
    bool IsFileEncrypted() const;
    bool IsTextureCompressed() const;

    gsl::span<std::uint8_t> DecryptFile();
    gsl::span<std::uint8_t> DecompressTexture() const;

    static void FixDecryptedExtension( fs::path& filePath );

private:
    gsl::span<std::uint8_t> m_FileData;
    fs::path m_FilePath;

    const bool m_bAllowDecryption;
    const bool m_bAllowDecompression;

private:
    SpecialFileHandler() = delete;
    SpecialFileHandler& operator=( const SpecialFileHandler& ) = delete;
    SpecialFileHandler( const SpecialFileHandler& ) = delete;
};

inline fs::path SpecialFileHandler::GetNewFilePath() const
{
    return this->m_FilePath;
}
