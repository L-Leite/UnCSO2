#pragma once

#include "archivebasenode.hpp"

#include <set>
#include <vector>

class ArchiveDirectoryNode : public ArchiveBaseNode
{
public:
    ArchiveDirectoryNode( const fs::path& directoryPath,
                          ArchiveDirectoryNode* pParentNode = nullptr );
    virtual ~ArchiveDirectoryNode();

    void AddChild( ArchiveBaseNode* pNewChild );
    ArchiveBaseNode* GetChild( size_t index ) const;
    size_t GetNumOfChildren() const;
    bool HasChildren() const;

    template <typename CompareType>
    inline void SortNodes( CompareType compare );

    void FreeChildren();

    ArchiveBaseNode* GetChildContaining( const fs::path& path ) const;
    int GetLocationOf( const ArchiveBaseNode* pChild ) const;

    virtual QVariant GetData( int column ) override;
    int GetRow() const;

    virtual uint64_t GetDecryptedSize() const override;

    virtual bool IsDirectory() const override;

    virtual bool IsCompressedTexture() const override;
    virtual bool IsFileEncrypted() const override;

    inline bool HasFileChild() const;

private:
    std::set<std::string_view> GetChildrenPkgOwners() const;

private:
    std::vector<ArchiveBaseNode*> m_vChildNodes;
};

template <typename CompareType>
inline void ArchiveDirectoryNode::SortNodes( CompareType compare )
{
    std::sort( this->m_vChildNodes.begin(), this->m_vChildNodes.end(),
               compare );
}

inline bool ArchiveDirectoryNode::HasFileChild() const
{
    for ( auto&& pPkg : this->m_vChildNodes )
    {
        if ( pPkg->IsDirectory() == false )
        {
            return true;
        }
    }

    return false;
}

inline ArchiveDirectoryNode* BaseToDirectoryNode( ArchiveBaseNode* baseNode )
{
    if ( baseNode == nullptr || baseNode->IsDirectory() == false )
        return nullptr;

    return static_cast<ArchiveDirectoryNode*>( baseNode );
}

inline ArchiveDirectoryNode* IndexToDirectoryNode( const QModelIndex& index )
{
    ArchiveBaseNode* pNode =
        static_cast<ArchiveBaseNode*>( index.internalPointer() );

    if ( pNode == nullptr || pNode->IsDirectory() == false )
        return nullptr;

    return static_cast<ArchiveDirectoryNode*>( pNode );
}
