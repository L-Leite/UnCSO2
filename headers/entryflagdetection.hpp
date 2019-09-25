#pragma once

#include <gsl/gsl>

namespace uc2
{
class PkgEntry;
}  // namespace uc2

class EntryFlagDetection
{
public:
    EntryFlagDetection( gsl::not_null<uc2::PkgEntry*> pEntry );

public:
    inline bool IsEncryptedFile() const noexcept;
    inline bool IsLzmaTexture() const noexcept;

private:
    void DetectSpecialTypes( gsl::span<uint8_t> entryData ) noexcept;

private:
    bool m_bEncryptedFile;
    bool m_bLzmaTexture;
};

inline bool EntryFlagDetection::IsEncryptedFile() const noexcept
{
    return this->m_bEncryptedFile;
}

inline bool EntryFlagDetection::IsLzmaTexture() const noexcept
{
    return this->m_bLzmaTexture;
}
