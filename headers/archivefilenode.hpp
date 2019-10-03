#pragma once

#include <string_view>

#include "archivebasenode.hpp"

namespace uc2
{
class PkgEntry;
}  // namespace uc2

class ArchiveDirectoryNode;

class ArchiveFileNode : public ArchiveBaseNode
{
public:
    ArchiveFileNode( const fs::path& ownerPkgPath, uc2::PkgEntry* pPkgEntry,
                     bool isCompressedTex, bool isEncrypted,
                     ArchiveDirectoryNode* pParentNode = nullptr );
    virtual ~ArchiveFileNode();

    virtual QVariant GetData( int column ) override;

    virtual uint64_t GetDecryptedSize() const override;

    std::string_view GetOwnerPkgFilename() const;

    virtual bool IsDirectory() const override;

    virtual bool IsCompressedTexture() const override;
    virtual bool IsFileEncrypted() const override;

    inline uc2::PkgEntry* GetPkgEntry() const;

private:
    QString GetFileTypeColumnString() const;
    QString BuildFlagsString() const;

private:
    std::string m_szOwnerPkgFilename;

    uc2::PkgEntry* m_pPkgEntry;

    uint64_t m_iDecryptedSize;

    bool m_bIsCompressedTexture;
    bool m_bIsFileEncrypted;
};

inline uc2::PkgEntry* ArchiveFileNode::GetPkgEntry() const
{
    return this->m_pPkgEntry;
}

inline ArchiveFileNode* BaseToFileNode( ArchiveBaseNode* baseNode )
{
    if ( baseNode == nullptr || baseNode->IsDirectory() == true )
        return nullptr;

    return static_cast<ArchiveFileNode*>( baseNode );
}

inline ArchiveFileNode* IndexToFileNode( const QModelIndex& index )
{
    ArchiveBaseNode* pNode =
        static_cast<ArchiveBaseNode*>( index.internalPointer() );

    if ( pNode->IsDirectory() == true )
        return nullptr;

    return static_cast<ArchiveFileNode*>( pNode );
}
