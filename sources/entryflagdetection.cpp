#include "entryflagdetection.hpp"

#include <cassert>

#include <uc2/uc2.hpp>

#include "miscutils.hpp"

EntryFlagDetection::EntryFlagDetection( gsl::not_null<uc2::PkgEntry*> pEntry )
    : m_bEncryptedFile( false ), m_bLzmaTexture( false )
{
    // encrypted file's header is bigger than the lzma texture's header
    assert( uc2::EncryptedFile::GetHeaderSize() >=
            uc2::LzmaTexture::GetHeaderSize() );
    const uint64_t iHeaderSize = uc2::EncryptedFile::GetHeaderSize();

    if ( pEntry->GetEncryptedSize() >= iHeaderSize )
    {
        auto dataView = PairToSpan( pEntry->DecryptFile( iHeaderSize ) );

        if ( dataView.empty() == false )
        {
            this->DetectSpecialTypes( dataView );
        }
    }
}

void EntryFlagDetection::DetectSpecialTypes(
    gsl::span<uint8_t> entryData ) noexcept
{
    this->m_bEncryptedFile = uc2::EncryptedFile::IsEncryptedFile(
        entryData.data(), entryData.size_bytes() );
    this->m_bLzmaTexture = uc2::LzmaTexture::IsLzmaTexture(
        entryData.data(), entryData.size_bytes() );
}
