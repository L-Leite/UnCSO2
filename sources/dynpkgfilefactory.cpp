#include "dynpkgfilefactory.hpp"

#include <array>

#include <QDebug>

#include "fsutils.hpp"
#include "miscutils.hpp"

using namespace std::string_view_literals;

constexpr const std::array<std::string_view, 4> PACKAGE_ENTRY_KEYS = {
    "lkgui781kl789sd!@#%89&^sd"sv,  // Nexon
    "\x9B\x65\xC7\x9B\xC7\xDF\x8E\x7E\xD4\xC6\x59\x52\x5C\xF7\x22\xFF\xF4\xE8\xFF\xE7\xB5\xC2\x77"sv,  // Tiancity
    "\x86\x39\x53\xBD\x16\x11\x6D\x06\x2A\x84\xF3\x4E\xE0\x4A\xA3"sv,  // Beancity
    "lkgui781kl789sd!@#%89&^sd"sv,  // NexonJP
};

constexpr const std::array<std::string_view, 4> PACKAGE_DATA_KEYS = {
    "^9gErg2Sx7bnk7@#sdfjnh@"sv,  // Nexon
    "\x8E\x5C\xB8\x92\x45\xD1\x90\xBA\x82\x0F\xD9\x7A\x99\x8E\xB3\x87\xF7"sv,  // Tiancity
    "\x1F\x9F\xF8\xF4\x18\xAC\x25\xA2\xBB\x37\x82\x6D\xA8\xAE\xA7\x28\xBA\xDD\xDD\xE4\x6B"sv,  // Beancity
    "^9gErg2Sx7bnk7@#sdfjnh@"sv,  // NexonJP
};

constexpr const std::string_view TFO_PACKAGE_ENTRY_KEY =
    "lkgui781kl789sd!@#%89&^sd"sv;
constexpr const std::string_view TFO_PACKAGE_DATA_KEY =
    "^9gErg2Sx7bnk7@#sdfjnh@"sv;

inline constexpr std::pair<std::string_view, std::string_view>
GetPackageKeysByProvider( GameProvider provider ) noexcept
{
    switch ( provider )
    {
        case GameProvider::Nexon:
            return { PACKAGE_ENTRY_KEYS[0], PACKAGE_DATA_KEYS[0] };
        case GameProvider::Tiancity:
            return { PACKAGE_ENTRY_KEYS[1], PACKAGE_DATA_KEYS[1] };
        case GameProvider::Beancity:
            return { PACKAGE_ENTRY_KEYS[2], PACKAGE_DATA_KEYS[2] };
        case GameProvider::NexonJP:
            return { PACKAGE_ENTRY_KEYS[3], PACKAGE_DATA_KEYS[3] };
        case GameProvider::Tfo:
            return { TFO_PACKAGE_ENTRY_KEY, TFO_PACKAGE_DATA_KEY };
        default:
            return {};
    }
}

DynamicPkgFileFactory::DynamicPkgFileFactory( const fs::path& pkgFilePath )
    : m_PkgFilePath( pkgFilePath ), m_DetectedProvider( GameProvider::Unknown )
{
    if ( this->LoadBaseFileHeader() == false )
    {
        throw std::runtime_error( "Could not load the PKG file's header" );
    }

    if ( this->TryDetectProvider() == false )
    {
        throw std::runtime_error( "Could not detect the game's provider" );
    }

    if ( this->LoadFullFileHeader() == false )
    {
        throw std::runtime_error( "Could not load the PKG file's full header" );
    }
}

DynamicPkgFileFactory::DynamicPkgFileFactory( const fs::path& pkgFilePath,
                                              GameProvider provider )
    : m_PkgFilePath( pkgFilePath ), m_DetectedProvider( GameProvider::Unknown )
{
    if ( this->LoadBaseFileHeader() == false )
    {
        throw std::runtime_error( "Could not load the PKG file's base header" );
    }

    if ( provider == GameProvider::Unknown )
    {
        if ( this->TryDetectProvider() == false )
        {
            throw std::runtime_error( "Could not detect the game's provider" );
        }
    }
    else if ( this->TrySpecificProvider( provider ) == false )
    {
        throw std::invalid_argument( "Failed to use specific game's provider" );
    }

    if ( this->LoadFullFileHeader() == false )
    {
        throw std::runtime_error( "Could not load the PKG file's full header" );
    }
}

DynamicPkgFileFactory::~DynamicPkgFileFactory() {}

uc2::PkgFile::ptr_t&& DynamicPkgFileFactory::GetPkgFileOwnership() noexcept
{
    return std::move( this->m_pPkgFile );
}

bool DynamicPkgFileFactory::LoadBaseFileHeader()
{
    const uint64_t iBaseHeaderSize = uc2::PkgFile::GetHeaderSize( false );

    auto [bPkgRead, vPkgData] =
        ReadFileToBuffer( this->m_PkgFilePath, iBaseHeaderSize );

    if ( bPkgRead == false )
    {
        return false;
    }

    this->m_vPkgFileData = std::move( vPkgData );
    return true;
}

bool DynamicPkgFileFactory::LoadFullFileHeader()
{
    // const uint64_t iFullHeaderSize = this->m_pPkgFile->GetFullHeaderSize();

    auto [bPkgRead, vPkgData] =
        ReadFileToBuffer( this->m_PkgFilePath /*, iFullHeaderSize */ );

    if ( bPkgRead == false )
    {
        return false;
    }

    this->m_vPkgFileData = std::move( vPkgData );
    this->m_pPkgFile->SetDataBuffer( this->m_vPkgFileData );
    return this->m_pPkgFile->DecryptHeader();
}

bool DynamicPkgFileFactory::TryDetectProvider()
{
    Q_ASSERT( this->m_vPkgFileData.empty() == false );

    this->m_pPkgFile = uc2::PkgFile::Create(
        this->m_PkgFilePath.filename().generic_string(), this->m_vPkgFileData );

    auto csRes = DynamicPkgFileFactory::TestCsKeys( this->m_pPkgFile.get(),
                                                    this->m_vPkgFileData );

    if ( csRes.first == true )
    {
        this->m_DetectedProvider = csRes.second;
        return true;
    }

    if ( DynamicPkgFileFactory::TestTfoKeys( this->m_pPkgFile.get() ) == true )
    {
        this->m_DetectedProvider = GameProvider::Tfo;
        return true;
    }

    this->m_pPkgFile.release();
    return false;
}

bool DynamicPkgFileFactory::TrySpecificProvider( GameProvider provider )
{
    Q_ASSERT( provider != GameProvider::Unknown );
    Q_ASSERT( this->m_vPkgFileData.empty() == false );

    this->m_DetectedProvider = provider;

    auto [entryKey, dataKey] =
        GetPackageKeysByProvider( this->m_DetectedProvider );

    this->m_pPkgFile = uc2::PkgFile::Create(
        this->m_PkgFilePath.filename().generic_string(), this->m_vPkgFileData,
        CopyViewToNewStr( entryKey ), CopyViewToNewStr( dataKey ) );

    if ( this->m_pPkgFile->DecryptHeader() == false )
    {
        this->m_pPkgFile.release();
        return false;
    }

    return true;
}

std::pair<bool, GameProvider> DynamicPkgFileFactory::TestCsKeys(
    uc2::PkgFile* pPkgFile, std::vector<uint8_t>& vPkgFileData )
{
    std::vector<uint8_t> vBackupPkgData = vPkgFileData;

    for ( size_t i = 0; i < PACKAGE_ENTRY_KEYS.size(); i++ )
    {
        try
        {
            pPkgFile->SetEntryKey( PACKAGE_ENTRY_KEYS[i].data() );
            pPkgFile->SetDataKey( PACKAGE_DATA_KEYS[i].data() );

            if ( pPkgFile->DecryptHeader() == true )
            {
                return { true,
                         GetGameProviderByIndex( gsl::narrow_cast<int>( i ) ) };
            }

            vPkgFileData = vBackupPkgData;
        }
        catch ( const std::exception& e )
        {
            qDebug() << e.what();
            continue;
        }
    }

    return { false, GameProvider::Unknown };
}

bool DynamicPkgFileFactory::TestTfoKeys( uc2::PkgFile* pPkgFile )
{
    pPkgFile->SetTfoPkg( true );
    pPkgFile->SetEntryKey( TFO_PACKAGE_ENTRY_KEY.data() );
    pPkgFile->SetDataKey( TFO_PACKAGE_DATA_KEY.data() );

    return pPkgFile->DecryptHeader();
}
