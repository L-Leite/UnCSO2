#include "archivefilenode.hpp"

#include <QLocale>

#include <gsl/gsl>

#include <string>

#include <uc2/pkgentry.hpp>

#include "pkgfilesystemshared.hpp"

ArchiveFileNode::ArchiveFileNode(
    const fs::path& ownerPkgPath, uc2::PkgEntry* pPkgEntry,
    bool isCompressedTex, bool isEncrypted,
    ArchiveDirectoryNode* pParentNode /*= nullptr*/ )
    : ArchiveBaseNode( pPkgEntry->GetFilePath(), pParentNode ),
      m_szOwnerPkgFilename( ownerPkgPath.filename().generic_string() ),
      m_pPkgEntry( pPkgEntry ),
      m_iDecryptedSize( pPkgEntry->GetDecryptedSize() ),
      m_bIsCompressedTexture( isCompressedTex ),
      m_bIsFileEncrypted( isEncrypted )
{
}

ArchiveFileNode::~ArchiveFileNode() {}

QVariant ArchiveFileNode::GetData( int column )
{
    switch ( column )
    {
        case PFS_FileNameColumn:
            return QString::fromStdString(
                this->GetPath().filename().generic_string() );
        case PFS_TypeColumn:
            return this->GetFileTypeColumnString();
        case PFS_SizeColumn:
            return QLocale::system().formattedDataSize(
                gsl::narrow_cast<qint64>( this->GetDecryptedSize() ) );
        case PFS_FlagsColumn:
            return this->BuildFlagsString();
        case PFS_OwnerPkgColumn:
            return QString( this->GetOwnerPkgFilename().data() );
    }

    return QVariant();
}

uint64_t ArchiveFileNode::GetDecryptedSize() const
{
    return this->m_iDecryptedSize;
}

std::string_view ArchiveFileNode::GetOwnerPkgFilename() const
{
    return this->m_szOwnerPkgFilename;
}

bool ArchiveFileNode::IsDirectory() const
{
    return false;
}

bool ArchiveFileNode::IsCompressedTexture() const
{
    return this->m_bIsCompressedTexture;
}

bool ArchiveFileNode::IsFileEncrypted() const
{
    return this->m_bIsFileEncrypted;
}

QString ArchiveFileNode::GetFileTypeColumnString() const
{
    QString typeFmt( tr( "%1 file" ) );
    return typeFmt.arg( this->GetPath().extension().c_str() );
}

QString ArchiveFileNode::BuildFlagsString() const
{
    QString szFlags;

    if ( this->m_bIsCompressedTexture == true )
    {
        szFlags = tr( "Compressed texture" );
    }

    if ( this->m_bIsFileEncrypted == true )
    {
        if ( szFlags.isEmpty() == false )
        {
            szFlags += tr( ", " );
        }

        szFlags += tr( "Encrypted file" );
    }

    return szFlags;
}
