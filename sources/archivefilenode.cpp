#include "archivefilenode.hpp"

#include <QLocale>

#include <gsl/gsl>

#include <string>

#include <uc2/pkgentry.hpp>

#include "pkgfilesystemshared.hpp"

ArchiveFileNode::ArchiveFileNode(
    const fs::path& ownerPkgPath, uc2::PkgEntry* pPkgEntry,
    ArchiveDirectoryNode* pParentNode /*= nullptr*/ )
    : ArchiveBaseNode( pPkgEntry->GetFilePath(), pParentNode ),
      m_szOwnerPkgFilename( ownerPkgPath.filename().generic_string() ),
      m_pPkgEntry( pPkgEntry ),
      m_iDecryptedSize( pPkgEntry->GetDecryptedSize() )
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

QString ArchiveFileNode::GetFileTypeColumnString() const
{
    QString typeFmt( tr( "%1 file" ) );
    return typeFmt.arg( this->GetPath().extension().c_str() );
}
