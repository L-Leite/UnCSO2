#include "dynindexfilefactory.hpp"

#include <array>

#include <QDebug>

#include "indexkeycollections.hpp"

using namespace std::string_view_literals;

DynamicIndexFileFactory::DynamicIndexFileFactory(
    const fs::path& indexFilePath, std::vector<uint8_t>& indexData )
    : m_IndexFilePath( indexFilePath ), m_vIndexData( indexData )
{
    if ( this->TryDetectProvider() == false )
    {
        throw std::runtime_error( "Could not detect the game's provider" );
    }
}

DynamicIndexFileFactory::~DynamicIndexFileFactory() {}

uc2::PkgIndex::ptr_t&& DynamicIndexFileFactory::GetPkgIndexOwnership() noexcept
{
    return std::move( this->m_pPkgIndex );
}

bool DynamicIndexFileFactory::TryDetectProvider()
{
    Q_ASSERT( this->m_vIndexData.empty() == false );

    std::string szFilename = this->m_IndexFilePath.filename().generic_string();

    this->m_pPkgIndex = uc2::PkgIndex::Create( szFilename, this->m_vIndexData );

    bool bDetected =
        DynamicIndexFileFactory::TestCsKeys( this->m_pPkgIndex.get() );

    if ( bDetected == true )
    {
        return true;
    }

    bDetected = DynamicIndexFileFactory::TestTfoKeys( this->m_pPkgIndex.get() );

    if ( bDetected == true )
    {
        return true;
    }

    this->m_pPkgIndex.release();
    return false;
}

bool DynamicIndexFileFactory::TestCsKeys( uc2::PkgIndex* pIndexFile )
{
    pIndexFile->SetKeyCollection( &CS_INDEX_KEY_COLLECTION );

    try
    {
        pIndexFile->ValidateHeader();
        pIndexFile->Parse();
    }
    catch ( const std::exception& e )
    {
        qDebug() << e.what();
        return false;
    }

    return true;
}

bool DynamicIndexFileFactory::TestTfoKeys( uc2::PkgIndex* pIndexFile )
{
    pIndexFile->SetKeyCollection( &TFO_INDEX_KEY_COLLECTION );

    try
    {
        pIndexFile->ValidateHeader();
        pIndexFile->Parse();
    }
    catch ( const std::exception& e )
    {
        qDebug() << e.what();
        return false;
    }

    return true;
}
