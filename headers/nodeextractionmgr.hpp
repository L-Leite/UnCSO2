#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <gsl/gsl>

#include <uc2/pkgfile.hpp>

namespace fs = std::filesystem;

class QModelIndex;

namespace uc2
{
class PkgEntry;
}  // namespace uc2

class ArchiveBaseNode;
class ArchiveDirectoryNode;
class ArchiveFileNode;

enum class GameProvider;

class NodeExtractionMgr
{
public:
    NodeExtractionMgr(
        const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>& pkgFiles,
        fs::path outPath, int& outProgressNum, bool canDecrypt,
        bool canDecompress );
    ~NodeExtractionMgr() = default;

public:
    bool LoadPkgFileData( const fs::path& pkgParentPath,
                          uc2::PkgFile* pkgFile );

    inline int GetExtractionProgress() const;
    inline const std::vector<fs::path>& GetExtractedNodesPaths() const;

    // from PkgFileModel
    bool ExtractNodes( const gsl::span<ArchiveBaseNode*> targetNodes,
                       const fs::path& pkgParentPath );
    bool ExtractPackages( gsl::span<uc2::PkgFile*> pkgs,
                          const fs::path& pkgParentPath );

    bool ExtractSingleFileNode( ArchiveFileNode* pFileNode,
                                const fs::path& pkgParentPath,
                                fs::path& outResultPath );

private:
    void AddNodes( const gsl::span<ArchiveBaseNode*> nodes,
                   uc2::PkgFile* ownerPkgFile );
    void AddFileNode( ArchiveFileNode* pFileNode, uc2::PkgFile* ownerPkgFile,
                      fs::path nodeParentDir = {} );
    void AddDirectoryNode( ArchiveDirectoryNode* pDirNode,
                           uc2::PkgFile* ownerPkgFile,
                           fs::path parentNodePath = {} );

    void HandleFileNode( ArchiveFileNode* pFileNode, std::size_t iPkgPathHash,
                         fs::path parentDir );

    bool WriteNodesToDisk();
    bool WritePackageToDisk( uc2::PkgFile* pPkgFile );
    static bool WritePkgEntryInternal( uc2::PkgEntry* pEntry,
                                       fs::path& outFilePath, bool canDecrypt,
                                       bool canDecompress );

    inline bool HasAnyNodes() const;

    // from PkgFileModel
    std::vector<uc2::PkgFile*> GetRequiredPkgFiles(
        const gsl::span<ArchiveBaseNode*> nodes );

private:
    std::vector<uint8_t> m_vLoadedPkgFile;
    // from PkgFileModel
    const std::unordered_map<std::size_t, uc2::PkgFile::ptr_t>& m_PkgFiles;

    fs::path m_OutPath;

    std::vector<std::pair<fs::path, ArchiveFileNode*>> m_vOutNodesData;

    std::vector<fs::path> m_ExtractedNodesPaths;
    int& m_iExtractionProgress;

    const bool m_bAllowDecryption;
    const bool m_bAllowDecompression;

private:
    NodeExtractionMgr() = delete;
    NodeExtractionMgr& operator=( const NodeExtractionMgr& ) = delete;
    NodeExtractionMgr( const NodeExtractionMgr& ) = delete;
};

inline bool NodeExtractionMgr::HasAnyNodes() const
{
    return this->m_vOutNodesData.empty() == false;
}

inline int NodeExtractionMgr::GetExtractionProgress() const
{
    return this->m_iExtractionProgress;
}

inline const std::vector<fs::path>& NodeExtractionMgr::GetExtractedNodesPaths()
    const
{
    return this->m_ExtractedNodesPaths;
}
