#include "nodeextractionmgr.hpp"

#include <string>
#include <unordered_set>

#include <QDebug>

#include <uc2/pkgentry.hpp>
#include <uc2/pkgfile.hpp>

#include "fsutils.hpp"
#include "miscutils.hpp"

#include "archivedirectorynode.hpp"
#include "archivefilenode.hpp"
#include "specialfilehandler.hpp"

NodeExtractionMgr::NodeExtractionMgr(
    const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>& pkgFiles,
    fs::path outPath, int& outProgressNum, bool canDecrypt, bool canDecompress )
    : m_PkgFiles( pkgFiles ), m_OutPath( outPath ),
      m_iExtractionProgress( outProgressNum ), m_bAllowDecryption( canDecrypt ),
      m_bAllowDecompression( canDecompress )
{
}

void NodeExtractionMgr::AddNodes( const gsl::span<ArchiveBaseNode*> nodes,
                                  uc2::PkgFile* ownerPkgFile )
{
    std::string szPkgFilename( ownerPkgFile->GetFilename() );

    for ( const auto& index : nodes )
    {
        if ( index->IsDirectory() == true )
        {
            auto pDirNode = BaseToDirectoryNode( index );
            Q_ASSERT( pDirNode != nullptr );
            this->AddDirectoryNode( pDirNode, ownerPkgFile );
        }
        else
        {
            ArchiveFileNode* pFileNode = BaseToFileNode( index );
            Q_ASSERT( pFileNode != nullptr );
            this->AddFileNode( pFileNode, ownerPkgFile );
        }
    }
}

void NodeExtractionMgr::AddFileNode( ArchiveFileNode* pFileNode,
                                     uc2::PkgFile* ownerPkgFile,
                                     fs::path nodeParentDir /*= {} */ )
{
    std::size_t iPkgPathHash =
        GenerateHashFromString( ownerPkgFile->GetFilename() );
    this->HandleFileNode( pFileNode, iPkgPathHash, nodeParentDir );
}

void NodeExtractionMgr::AddDirectoryNode( ArchiveDirectoryNode* pDirNode,
                                          uc2::PkgFile* ownerPkgFile,
                                          fs::path parentNodePath /*= {} */ )
{
    for ( std::size_t i = 0; i < pDirNode->GetNumOfChildren(); i++ )
    {
        auto pBaseChild = pDirNode->GetChild( i );

        if ( pBaseChild->IsDirectory() == true )
        {
            auto pDirChild = BaseToDirectoryNode( pBaseChild );
            Q_ASSERT( pDirChild != nullptr );
            Q_ASSERT( pDirChild != pDirNode );

            fs::path newParentNodePath =
                parentNodePath / pDirNode->GetPath().filename();
            qDebug() << newParentNodePath.c_str();

            this->AddDirectoryNode( pDirChild, ownerPkgFile,
                                    newParentNodePath );
        }
        else
        {
            auto pFileChild = BaseToFileNode( pBaseChild );
            Q_ASSERT( pFileChild != nullptr );

            fs::path dirPath = parentNodePath;
            dirPath /= pDirNode->GetPath().filename();
            qDebug() << dirPath.c_str();

            this->AddFileNode( pFileChild, ownerPkgFile, dirPath );
        }
    }
}

bool NodeExtractionMgr::LoadPkgFileData( const fs::path& pkgParentPath,
                                         uc2::PkgFile* pkgFile )
{
    Q_ASSERT( pkgFile != nullptr );

    fs::path ownerPkgPath = pkgParentPath;
    ownerPkgPath /= pkgFile->GetFilename();

    auto [bPkgRead, vPkgData] = ReadFileToBuffer( ownerPkgPath );

    if ( bPkgRead == false )
    {
        return false;
    }

    this->m_vLoadedPkgFile = std::move( vPkgData );
    pkgFile->SetDataBuffer( this->m_vLoadedPkgFile );
    pkgFile->DecryptHeader();
    pkgFile->Parse();

    return true;
}

bool NodeExtractionMgr::WriteNodesToDisk()
{
    this->m_ExtractedNodesPaths.clear();
    this->m_ExtractedNodesPaths.reserve( this->m_vOutNodesData.size() );

    for ( auto&& nodePair : this->m_vOutNodesData )
    {
        auto nodePath = nodePair.first;
        auto pFileNode = nodePair.second;

        fs::path targetFilePath = this->m_OutPath / nodePath;
        uc2::PkgEntry* pPkgEntry = pFileNode->GetPkgEntry();

        if ( this->WritePkgEntryInternal(
                 pPkgEntry, targetFilePath, this->m_bAllowDecryption,
                 this->m_bAllowDecompression ) == false )
        {
            return false;
        }

        this->m_ExtractedNodesPaths.push_back( targetFilePath );
        this->m_iExtractionProgress++;
    }

    return true;
}

bool NodeExtractionMgr::WritePackageToDisk( uc2::PkgFile* pPkgFile )
{
    std::string_view firstEntryParentView =
        pPkgFile->GetEntries().at( 0 )->GetFilePath().substr(
            1 );  // skip the root path
    fs::path targetParentDir = this->m_OutPath / firstEntryParentView;

    if ( CreateDirIfUnexisting( targetParentDir.parent_path() ) == false )
    {
        return false;
    }

    for ( auto&& entry : pPkgFile->GetEntries() )
    {
        std::string_view entryParentDirView =
            entry->GetFilePath().substr( 1 );  // skip the root path
        fs::path targetFilePath = this->m_OutPath / entryParentDirView;

        if ( this->WritePkgEntryInternal(
                 entry.get(), targetFilePath, this->m_bAllowDecryption,
                 this->m_bAllowDecompression ) == false )
        {
            return false;
        }

        this->m_iExtractionProgress++;
    }

    return true;
}

void NodeExtractionMgr::HandleFileNode( ArchiveFileNode* pFileNode,
                                        std::size_t iPkgPathHash,
                                        fs::path parentDir )
{
    std::size_t iOwnerFilenameHash =
        GenerateHashFromString( pFileNode->GetOwnerPkgFilename() );

    if ( iPkgPathHash != iOwnerFilenameHash )
    {
        return;
    }

    fs::path fullFilePath = parentDir;
    fullFilePath /= pFileNode->GetPath().filename();

    qDebug() << fullFilePath.c_str();

    this->m_vOutNodesData.push_back( { fullFilePath, pFileNode } );
}

bool NodeExtractionMgr::WritePkgEntryInternal( uc2::PkgEntry* pEntry,
                                               fs::path& outFilePath,
                                               bool canDecrypt,
                                               bool canDecompress )
{
    auto decryptedBuffer = PairToSpan( pEntry->DecryptFile() );

    if ( decryptedBuffer.empty() == true )
    {
        return false;
    }

    if ( CreateDirIfUnexisting( outFilePath.parent_path() ) == false )
    {
        return false;
    }

    SpecialFileHandler handler( decryptedBuffer, outFilePath, canDecrypt,
                                canDecompress );

    decryptedBuffer = handler.ProcessData();
    outFilePath = handler.GetNewFilePath();

    bool bFileWritten = WriteBufferToFile( outFilePath, decryptedBuffer );

    if ( bFileWritten == false )
    {
        return false;
    }

    return true;
}

static std::set<std::string_view> GetDirChildrenFiles(
    ArchiveDirectoryNode* pDirNode )
{
    std::set<std::string_view> vChildrenFilenames;

    for ( std::size_t i = 0; i < pDirNode->GetNumOfChildren(); i++ )
    {
        ArchiveBaseNode* pChild = pDirNode->GetChild( i );
        Q_ASSERT( pChild != nullptr );

        if ( pChild->IsDirectory() == true )
        {
            auto vRecursiveChildFiles = GetDirChildrenFiles(
                static_cast<ArchiveDirectoryNode*>( pChild ) );
            vChildrenFilenames.insert( vRecursiveChildFiles.begin(),
                                       vRecursiveChildFiles.end() );
        }
        else
        {
            vChildrenFilenames.insert( static_cast<ArchiveFileNode*>( pChild )
                                           ->GetOwnerPkgFilename() );
        }
    }

    return vChildrenFilenames;
}

std::vector<uc2::PkgFile*> NodeExtractionMgr::GetRequiredPkgFiles(
    const gsl::span<ArchiveBaseNode*> nodes )
{
    std::unordered_set<uc2::PkgFile*> uniquePkgFiles;

    for ( const auto& node : nodes )
    {
        if ( node->IsDirectory() == true )
        {
            auto vDirFileNodes = GetDirChildrenFiles(
                static_cast<ArchiveDirectoryNode*>( node ) );

            for ( auto&& childPkgFilename : vDirFileNodes )
            {
                std::size_t iOwnerHash =
                    GenerateHashFromString( childPkgFilename );
                uc2::PkgFile* pPkgFile =
                    this->m_PkgFiles.at( iOwnerHash ).get();
                Q_ASSERT( pPkgFile != nullptr );

                uniquePkgFiles.insert( pPkgFile );
            }
        }
        else
        {
            auto pFileNode = static_cast<ArchiveFileNode*>( node );
            std::size_t iOwnerHash =
                GenerateHashFromString( pFileNode->GetOwnerPkgFilename() );
            uc2::PkgFile* pPkgFile = this->m_PkgFiles.at( iOwnerHash ).get();
            Q_ASSERT( pPkgFile != nullptr );

            uniquePkgFiles.insert( pPkgFile );
        }
    }

    return { uniquePkgFiles.begin(), uniquePkgFiles.end() };
}

bool NodeExtractionMgr::ExtractNodes(
    const gsl::span<ArchiveBaseNode*> targetNodes,
    const fs::path& pkgParentPath )
{
    auto vPkgFiles = this->GetRequiredPkgFiles( targetNodes );

    for ( auto&& pPkgFile : vPkgFiles )
    {
        Q_ASSERT( pPkgFile != nullptr );

        this->AddNodes( targetNodes, pPkgFile );

        if ( this->HasAnyNodes() == false )
        {
            continue;
        }

        const bool bDataLoaded =
            this->LoadPkgFileData( pkgParentPath, pPkgFile );

        if ( bDataLoaded == false )
        {
            return false;
        }

        const bool bFilesWritten = this->WriteNodesToDisk();

        if ( bFilesWritten == false )
        {
            return false;
        }
    }

    return true;
}

bool NodeExtractionMgr::ExtractPackages( gsl::span<uc2::PkgFile*> pkgs,
                                         const fs::path& pkgParentPath )
{
    for ( auto&& pPkgFile : pkgs )
    {
        Q_ASSERT( pPkgFile != nullptr );

        auto ownerPkgPath = pkgParentPath;
        ownerPkgPath /= pPkgFile->GetFilename();

        auto [bPkgRead, vPkgData] = ReadFileToBuffer( ownerPkgPath );

        if ( bPkgRead == false )
        {
            return false;
        }

        pPkgFile->SetDataBuffer( vPkgData );
        pPkgFile->DecryptHeader();
        pPkgFile->Parse();

        bool bRes = this->WritePackageToDisk( pPkgFile );

        if ( bRes == false )
        {
            return false;
        }
    }

    return true;
}

bool NodeExtractionMgr::ExtractSingleFileNode( ArchiveFileNode* pFileNode,
                                               const fs::path& pkgParentPath,
                                               fs::path& outResultPath )
{
    std::size_t iOwnerHash =
        GenerateHashFromString( pFileNode->GetOwnerPkgFilename() );
    auto pPkgFile = this->m_PkgFiles.at( iOwnerHash ).get();
    Q_ASSERT( pPkgFile != nullptr );

    this->AddFileNode( pFileNode, pPkgFile );

    const bool bDataLoaded = this->LoadPkgFileData( pkgParentPath, pPkgFile );

    if ( bDataLoaded == false )
    {
        return false;
    }

    const bool bWritten = this->WriteNodesToDisk();

    if ( bWritten == false )
    {
        return false;
    }

    Q_ASSERT( this->GetExtractedNodesPaths().size() == 1 );
    outResultPath = this->GetExtractedNodesPaths().at( 0 );

    return true;
}
