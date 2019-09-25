#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <QCoreApplication>
#include <QModelIndex>
#include <QVariant>

namespace fs = std::filesystem;

class ArchiveDirectoryNode;

class ArchiveBaseNode
{
    Q_DECLARE_TR_FUNCTIONS( ArchiveBaseNode )

public:
    ArchiveBaseNode( const fs::path& itemPath,
                     ArchiveDirectoryNode* pParentNode = nullptr );
    virtual ~ArchiveBaseNode();

    ArchiveDirectoryNode* GetParentNode() const;
    void SetParentNode( ArchiveDirectoryNode* pNewParent );

    virtual QVariant GetData( int column ) = 0;
    int GetRow() const;

    inline const fs::path& GetPath() const;
    inline size_t GetPathHash() const;
    virtual uint64_t GetDecryptedSize() const = 0;

    virtual bool IsDirectory() const = 0;

    virtual bool IsCompressedTexture() const = 0;
    virtual bool IsFileEncrypted() const = 0;

protected:
    ArchiveDirectoryNode* m_pParentNode;

    fs::path m_NodePath;
    size_t m_iPathHash;
};

inline const fs::path& ArchiveBaseNode::GetPath() const
{
    return this->m_NodePath;
}

inline size_t ArchiveBaseNode::GetPathHash() const
{
    return this->m_iPathHash;
}

inline ArchiveBaseNode* IndexToGenericNode( const QModelIndex& index )
{
    return static_cast<ArchiveBaseNode*>( index.internalPointer() );
}
