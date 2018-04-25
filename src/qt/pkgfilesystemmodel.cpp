#include "stdafx.h"
#include "pkgfilesystemmodel.h"

#include <atomic>	  
#include <chrono>
#include <thread>

#include <QCoreApplication>
#include <QCollator>	
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>

#include <CryptoPP/md5.h>

#include "cso2bsp.h"
#include "cso2vtf.h"
#include "decryption.h"
#include "mainwindow.h"
#include "pkgfilesystem.h"   
#include "pkgfilesystemnode.h"
#include "pkgindex.h"	

#define STATUS_BAR_PRINT( message, ...) m_pParentWindow->GetStatusBar()->showMessage( QString( "%1: " message ).arg( __func__, ##__VA_ARGS__ ) )

using namespace std::chrono_literals;


QVector<CPkgFileSystemNode*> CPkgFileSystemModel::m_DirectoryNodes;

CPkgFileSystemModel::CPkgFileSystemModel( CMainWindow* pParent /*= Q_NULLPTR*/ ) : QAbstractItemModel( pParent )
{		
	m_pParentWindow = pParent;
	m_iSortColumn = 0;
	m_SortOrder = Qt::AscendingOrder;
	m_bForceSort = true;
	m_bGenerated = false;
	m_pRoot = new CPkgFileSystemNode();
	m_bShouldDecryptEncFiles = true;
	m_bShouldRenameEncFiles = true;
	m_bShouldDecompVtfFiles = true;
	m_bShouldDecompBspFiles = true;
	m_bShouldReplaceShadowblock = true;
	m_bShouldFixBspLumps = true;
}

CPkgFileSystemModel::~CPkgFileSystemModel()
{
	delete m_pRoot;
	m_DirectoryNodes.empty();
}

QVariant CPkgFileSystemModel::data( const QModelIndex &index, int role ) const
{
	if ( !index.isValid() )
		return QVariant();

	CPkgFileSystemNode* pNode = static_cast<CPkgFileSystemNode*>(index.internalPointer());

	if ( role == Qt::CheckStateRole && index.column() == 0 )
		return m_CheckedIndexes.contains( index ) ? Qt::Checked : Qt::Unchecked;

	if ( role != Qt::DisplayRole )
		return QVariant();

	return pNode->GetData( index.column() );
}

bool CPkgFileSystemModel::setData( const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole*/ )
{
	if ( role != Qt::CheckStateRole )
		return QAbstractItemModel::setData( index, value, role );

	if ( value == Qt::Checked )
		m_CheckedIndexes.insert( index );
	else
		m_CheckedIndexes.remove( index );

	if ( hasChildren( index ) )
		UpdateNodeChildren( index, value );

	emit dataChanged( index, index );
	return true;
}

QVariant CPkgFileSystemModel::headerData( int section, Qt::Orientation orientation,
	int role ) const
{
	switch ( role )
	{
		case Qt::TextAlignmentRole:
			return Qt::AlignLeft;

		case Qt::DisplayRole:
			if ( orientation == Qt::Horizontal )
			{
				switch ( section )
				{
					case FileNameColumn:
						return "Name";
					case TypeColumn:
						return "Type";
					case PackedSizeColumn:
						return "Packed size";
					case UnpackedSizeColumn:
						return "Unpacked size";
				}
			}
			break;
	}

	return QAbstractItemModel::headerData( section, orientation, role );
}

QModelIndex CPkgFileSystemModel::index( int row, int column, const QModelIndex &parent ) const
{
	if ( !hasIndex( row, column, parent ) )
		return QModelIndex();

	CPkgFileSystemNode* pParentNode;

	if ( !parent.isValid() )
		pParentNode = m_pRoot;
	else
		pParentNode = static_cast<CPkgFileSystemNode*>(parent.internalPointer());

	CPkgFileSystemNode* pChildNode = pParentNode->GetChildren().value( row );

	if ( pChildNode )
		return createIndex( row, column, pChildNode );
	else
		return QModelIndex();
}

QModelIndex CPkgFileSystemModel::index( const CPkgFileSystemNode *node, int column ) const
{
	CPkgFileSystemNode* pParentNode = node ? const_cast<CPkgFileSystemNode*>(node)->GetParentNode() : Q_NULLPTR;

	if ( node == m_pRoot || !pParentNode )
		return QModelIndex();

	int iVisualRow = pParentNode->GetLocationOf( const_cast<CPkgFileSystemNode*>(node) );
	return createIndex( iVisualRow, column, const_cast<CPkgFileSystemNode*>(node) );
}

QModelIndex CPkgFileSystemModel::parent( const QModelIndex &index ) const
{
	if ( !index.isValid() || !m_bGenerated )
		return QModelIndex();

	CPkgFileSystemNode* pChildItem = static_cast<CPkgFileSystemNode*>(index.internalPointer());
	CPkgFileSystemNode* pParentItem = pChildItem->GetParentNode();

	if ( !pParentItem || pParentItem == m_pRoot )
		return QModelIndex();

	return createIndex( pParentItem->GetRow(), 0, pParentItem );
}

int CPkgFileSystemModel::rowCount( const QModelIndex &parent ) const
{
	CPkgFileSystemNode* pParentItem;

	if ( parent.column() > 0 )
		return 0;

	if ( !parent.isValid() )
		pParentItem = m_pRoot;
	else
		pParentItem = static_cast<CPkgFileSystemNode*>(parent.internalPointer());

	return pParentItem->GetChildren().count();
}

int CPkgFileSystemModel::columnCount( const QModelIndex &parent ) const
{
	return (parent.column() > 0) ? 0 : NumColumns;
}

Qt::ItemFlags CPkgFileSystemModel::flags( const QModelIndex &index ) const
{
	if ( !index.isValid() )
		return Qt::NoItemFlags;

	Qt::ItemFlags iFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if ( index.column() == 0 )
		iFlags |= Qt::ItemIsUserCheckable;

	return iFlags;
}

class PkgFileSystemModelSorter
{
public:
	inline PkgFileSystemModelSorter( int column ) : sortColumn( column )
	{
		naturalCompare.setNumericMode( true );
		naturalCompare.setCaseSensitivity( Qt::CaseSensitive );
	}

	bool compareNodes( const CPkgFileSystemNode *l,
		const CPkgFileSystemNode *r ) const
	{
		switch ( sortColumn )
		{
			case CPkgFileSystemModel::FileNameColumn:
			{
				// place directories before files
				bool left = l->IsDirectory();
				bool right = r->IsDirectory();
				if ( left ^ right )
					return left;

				return naturalCompare.compare( l->GetFileName(), r->GetFileName() ) < 0;
			}
			case CPkgFileSystemModel::TypeColumn:
			{
				int compare = naturalCompare.compare( l->GetFileType(), r->GetFileType() );
				if ( compare == 0 )
					return naturalCompare.compare( l->GetFileName(), r->GetFileName() ) < 0;

				return compare < 0;
			}
			case CPkgFileSystemModel::PackedSizeColumn:
			{
				// Directories go first
				bool left = l->IsDirectory();
				bool right = r->IsDirectory();
				if ( left ^ right )
					return left;

				qint64 sizeDifference = l->GetFilePackedSize() - r->GetFilePackedSize();
				if ( sizeDifference == 0 )
					return naturalCompare.compare( l->GetFileName(), r->GetFileName() ) < 0;

				return sizeDifference < 0;
			}
			case CPkgFileSystemModel::UnpackedSizeColumn:
			{
				// Directories go first
				bool left = l->IsDirectory();
				bool right = r->IsDirectory();
				if ( left ^ right )
					return left;

				qint64 sizeDifference = l->GetFileUnpackedSize() - r->GetFileUnpackedSize();
				if ( sizeDifference == 0 )
					return naturalCompare.compare( l->GetFileName(), r->GetFileName() ) < 0;

				return sizeDifference < 0;
			}
		}
		Q_ASSERT( false );
		return false;
	}

	bool operator()( const CPkgFileSystemNode *l,
		const CPkgFileSystemNode *r ) const
	{
		return compareNodes( l, r );
	}


private:
	QCollator naturalCompare;
	int sortColumn;
};

void CPkgFileSystemModel::sort( int column, Qt::SortOrder order )
{
	if ( m_SortOrder == order && m_iSortColumn == column && !m_bForceSort )
		return;

	emit layoutAboutToBeChanged();
	QModelIndexList oldList = persistentIndexList();
	QVector<QPair<CPkgFileSystemNode*, int> > oldNodes;
	const int nodeCount = oldList.count();
	oldNodes.reserve( nodeCount );
	for ( int i = 0; i < nodeCount; ++i )
	{
		const QModelIndex &oldNode = oldList.at( i );
		QPair<CPkgFileSystemNode*, int> pair( GetNode( oldNode ), oldNode.column() );
		oldNodes.append( pair );
	}

	if ( !(m_iSortColumn == column && m_SortOrder != order && !m_bForceSort) )
	{
		for ( const auto& node : m_DirectoryNodes )
		{
			if ( !node->GetChildren().isEmpty() )
				sortChildren( column, index( const_cast<CPkgFileSystemNode*>(node) ) );
		}

		m_iSortColumn = column;
		m_bForceSort = false;
	}
	m_SortOrder = order;

	QModelIndexList newList;
	const int numOldNodes = oldNodes.size();
	newList.reserve( numOldNodes );
	for ( int i = 0; i < numOldNodes; ++i )
	{
		const QPair<CPkgFileSystemNode*, int> &oldNode = oldNodes.at( i );
		newList.append( index( oldNode.first, oldNode.second ) );
	}
	changePersistentIndexList( oldList, newList );
	emit layoutChanged();
}


void CPkgFileSystemModel::sortChildren( int column, const QModelIndex &parent )
{
	CPkgFileSystemNode *indexNode = GetNode( parent );
	if ( indexNode->GetChildren().count() == 0 )
		return;

	QVector<CPkgFileSystemNode*> values = indexNode->GetChildren();

	PkgFileSystemModelSorter ms( column );
	std::sort( values.begin(), values.end(), ms );

	indexNode->GetChildren().clear();
	const int numValues = values.count();
	indexNode->GetChildren().reserve( numValues );
	for ( int i = 0; i < numValues; ++i )
	{
		indexNode->GetChildren().append( values.at( i ) );
	}
}

CPkgFileSystemNode* CPkgFileSystemModel::GetNode( const QModelIndex &index ) const
{
	if ( !index.isValid() )
		return m_pRoot;
	CPkgFileSystemNode* indexNode = static_cast<CPkgFileSystemNode*>(index.internalPointer());
	Q_ASSERT( indexNode );
	return indexNode;
}

bool CPkgFileSystemModel::GenerateFileSystem()
{
	std::vector<std::string> vPkgFileNames;

	if ( !GetPkgFileNames( g_PkgDataPath, vPkgFileNames ) )
	{
		DBG_WPRINTF( L"GetPkgFileNames failed! Data path: %s\n", g_PkgDataPath.c_str() );
		QMessageBox msgBox( "Error", "GetPkgFileNames failed!", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}

	DBG_PRINTF( "got %llu packages\n", vPkgFileNames.size() );
	STATUS_BAR_PRINT( "got %2 packages", vPkgFileNames.size() );

	QProgressDialog procDlg( m_pParentWindow );
	procDlg.setRange( 0, vPkgFileNames.size() );
	procDlg.setWindowTitle( "Loading package entries" );
	procDlg.setLabelText( "" );
	procDlg.setCancelButtonText( "Cancel" );
	procDlg.setWindowModality( Qt::WindowModal );
	procDlg.setFixedWidth( QLabel( "This is the very length of the progressdialog due to hidpi reasons." ).sizeHint().width() );
	procDlg.show();

	static std::atomic<int> iCurrentPkg;
	static std::atomic<const std::string*> szCurrentPkgFile;
	static std::atomic<bool> bShouldStop;
	static std::atomic<int> iThreadReturnCode;

	iCurrentPkg = 0;
	szCurrentPkgFile = nullptr;
	bShouldStop = false;
	iThreadReturnCode = 0;

	std::thread loadThread( [&]()
	{
		for ( const auto& szFileName : vPkgFileNames )
		{
			if ( bShouldStop )
			{
				iThreadReturnCode = 2;
				return;
			}

			szCurrentPkgFile.store( &szFileName, std::memory_order_relaxed );

			if ( !LoadPkgEntries( szFileName, this ) )
			{
				DBG_PRINTF( "LoadPkgEntries failed! Data file: %s\n", szFileName.c_str() );			 				
				iThreadReturnCode = 1;
				return;

			}

			iCurrentPkg++;
		}
	} );  	
							
	while ( std::this_thread::sleep_for( 5ms ), iCurrentPkg < vPkgFileNames.size() )
	{
		if ( procDlg.wasCanceled() )
		{
			bShouldStop = true;
			break;
		}

		procDlg.setValue( iCurrentPkg );
		procDlg.setLabelText( szCurrentPkgFile.load( std::memory_order_consume )->c_str() );
		QCoreApplication::processEvents();
	}

	if ( loadThread.joinable() )
		loadThread.join();

	if ( iThreadReturnCode == 1 )
	{
		STATUS_BAR_PRINT( "LoadPkgEntries failed!" );
		QMessageBox msgBox( "Error", "LoadPkgEntries failed!", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}

	if ( iThreadReturnCode == 2 )
	{
		STATUS_BAR_PRINT( "LoadPkgEntries was canceled." );
		QMessageBox msgBox( "Warning", "LoadPkgEntries was canceled.", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}	   	

	sort( FileNameColumn );

	m_bGenerated = true;

	return true;
}

void CPkgFileSystemModel::CleanFileSystem()
{
	m_DirectoryNodes.clear();

	if ( m_pRoot )
	{
		delete m_pRoot;
		m_pRoot = new CPkgFileSystemNode();
	}		  

	m_bForceSort = true;
}

void CPkgFileSystemModel::CreateChild( CCSO2PkgEntry* pPkgEntry )
{
	std::filesystem::path filePath = pPkgEntry->GetEntryPath();

	CPkgFileSystemNode* pParent = m_pRoot;

	for ( const auto& subPath : filePath )
	{
		CPkgFileSystemNode* pChild = pParent->GetChildContaining( subPath );

		if ( !pChild )
		{
			bool bSubPathHasExtension = subPath.has_extension();

			pChild = new CPkgFileSystemNode( filePath, subPath, bSubPathHasExtension ? pPkgEntry : Q_NULLPTR, pParent );
			pParent->GetChildren().append( pChild );

			if ( !bSubPathHasExtension )
				m_DirectoryNodes.append( pChild );
		}

		pParent = pChild;
	}
}

void CPkgFileSystemModel::UpdateNodeChildren( const QModelIndex &modelIndex, const QVariant &value )
{
	if ( !hasChildren( modelIndex ) )
		return;

	for ( int i = 0; i < rowCount( modelIndex ); i++ )
		setData( index( i, 0, modelIndex ), value, Qt::CheckStateRole );
}

bool CPkgFileSystemModel::IsFileContentEncrypted( uint8_t* pFileBuffer, uint32_t iBufferSize )
{
	CSO2EncFileHeader_t* pFile = (CSO2EncFileHeader_t*) pFileBuffer;	  	

	if ( !pFile || pFile->iVersion != CSO2_ENCRYPTED_FILE_VER )
		return false;

	if ( pFile->iEncryption < PKGCIPHER_DES || pFile->iEncryption > PKGCIPHER_RIJNDAEL )
		return false;

	if ( pFile->iFileSize < 0 || pFile->iFileSize > iBufferSize )
		return false;

	return true;
}

bool CPkgFileSystemModel::DecryptEncFile( std::filesystem::path& szFilePath, uint8_t*& pBuffer, uint32_t& iBufferSize )
{	
	if ( !pBuffer )
		return false;

	if ( !iBufferSize )
	{
		DBG_PRINTF( "iBufferSize is null, pretending it's fine\n" );
		return true;
	}

	CSO2EncFileHeader_t* pHeader = (CSO2EncFileHeader_t*) pBuffer;
	pBuffer += sizeof( CSO2EncFileHeader_t );
	iBufferSize = pHeader->iFileSize;

	uint8_t digestedKey[CryptoPP::Weak::MD5::DIGESTSIZE];	   

	if ( !GeneratePkgListKey( pHeader->iFlag, szFilePath.filename().string().c_str(), digestedKey ) )
	{				
		DBG_PRINTF( "GeneratePkgListKey failed! Filename: %s Flag: %i\n", szFilePath.filename().string().c_str(), pHeader->iFlag );
		return false;
	}

	if ( !DecryptBuffer( pHeader->iEncryption, pBuffer, pHeader->iFileSize, digestedKey, sizeof( digestedKey ) ) )
	{
		DBG_PRINTF( "DecryptBuffer failed! Filename: %s Encryption: %s Filesize: %u (0x%X)\n", szFilePath.filename().string().c_str(), PkgCipherToString( pHeader->iFlag ), pHeader->iFileSize, pHeader->iFileSize );
		return false;
	}	  	

	return true;
}

bool CPkgFileSystemModel::ExtractCheckedNodes()
{
	if ( m_CheckedIndexes.isEmpty() )
	{
		DBG_PRINTF( "m_CheckedIndexes is empty!\n" );
		STATUS_BAR_PRINT( "m_CheckedIndexes is empty!" );
		QMessageBox msgBox( "Error", "Please select an item...", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}

	DBG_PRINTF( "m_CheckedIndexes has %i indexes\n", m_CheckedIndexes.count() );
	STATUS_BAR_PRINT( "m_CheckedIndexes has %2 indexes", m_CheckedIndexes.count() );

	QProgressDialog procDlg( m_pParentWindow );			
	procDlg.setWindowTitle( "Extracting package files" );
	procDlg.setLabelText( "Counting files..." );
	procDlg.setCancelButtonText( "Cancel" );
	procDlg.setWindowModality( Qt::WindowModal );
	procDlg.setFixedWidth( QLabel( "This is the very length of the progressdialog due to hidpi reasons." ).sizeHint().width() );  // credits to rpcs3 team
	procDlg.setRange( 0, m_CheckedIndexes.count() );
	procDlg.show();			

	static std::atomic<int> iCurrentEntry = 0;
	static std::atomic<CPkgFileSystemNode*> pCurrentNode;
	static std::atomic<bool> bShouldStop;
	static std::atomic<int> iThreadReturnCode;

	iCurrentEntry = 0;		 	
	pCurrentNode = nullptr;
	bShouldStop = false;
	iThreadReturnCode = 0;

	static size_t iShadowblockHash = std::hash<std::string>{}("toolsshadowblock.vmt");

	std::thread extractThread( [&]()
	{
		for ( const auto& index : m_CheckedIndexes )
		{
			iCurrentEntry++;

			if ( bShouldStop )
			{
				iThreadReturnCode = 2;
				return;
			}

			CPkgFileSystemNode* pNode = static_cast<CPkgFileSystemNode*>(index.internalPointer());

			Q_ASSERT( pNode );

			if ( pNode->IsDirectory() )
				continue;

			uint8_t* pBuffer = Q_NULLPTR;
			uint32_t iBufferSize = NULL;

			pCurrentNode = pNode;

			if ( ShouldReplaceShadowblock() && iShadowblockHash == pNode->GetFileNameHash() )
			{
				// it replaces with this (CRLF)
				// "LightmappedGeneric"
				// {
				// 		"$translucent" "1"
				// 		"$alpha" "0"
				// }			   
				static const char* szNewVmt = "\"LightmappedGeneric\"\r\n{\r\n\t\"$translucent\"\t\"1\"\r\n\t\"$alpha\"\t\"0\"\r\n}\r\n";
				static const size_t iNewVmtSize = strlen( szNewVmt );
				iBufferSize = iNewVmtSize;
				pBuffer = new uint8_t[iNewVmtSize];
				memcpy( pBuffer, szNewVmt, iNewVmtSize );
			}
			else if ( !pNode->GetPkgEntry()->ReadPkgEntry( &pBuffer, &iBufferSize ) )
			{
				DBG_WPRINTF( L"Failed to read %s!\n", pNode->GetFilePath().c_str() );
				iThreadReturnCode = 1;
				return;
			}

			uint8_t* pRealBuffer = pBuffer;

			std::filesystem::path targetFile = g_OutPath;
			targetFile += pNode->GetFilePath();

			std::filesystem::path targetPath = targetFile;
			targetPath.remove_filename();																					  
			std::error_code errorCode;
			std::filesystem::create_directories( targetPath, errorCode );	

			if ( errorCode )
			{
				DBG_WPRINTF( L"Couldn't create directory \"%s\"! Error: %S\n", targetPath.wstring().c_str(), errorCode.message().c_str() );
				delete[] pRealBuffer;
				iThreadReturnCode = 1;
				return;
			}

			if ( ShouldDecompBspFiles() && IsBspFile( pBuffer ) )
			{
				if ( !DecompressBsp( pBuffer, iBufferSize, ShouldFixBspLumps() ) )
				{
					DBG_WPRINTF( L"Couldn't decompress bsp \"%s\"!\n", targetFile.wstring().c_str() );
					delete[] pRealBuffer;
					iThreadReturnCode = 1;
					return;
				}

				pRealBuffer = pBuffer;
			}

			if ( ShouldDecompVtfFiles() && IsCompressedVtf( pBuffer ) )
			{
				if ( !DecompressVtf( pBuffer, iBufferSize ) )
				{
					DBG_WPRINTF( L"Couldn't decompress vtf \"%s\"!\n", targetFile.wstring().c_str() );
					delete[] pRealBuffer;
					iThreadReturnCode = 1;
					return;
				}

				pRealBuffer = pBuffer;
			}

			if ( ShouldDecryptEncFiles() && IsFileContentEncrypted( pBuffer, iBufferSize ) )
			{
				if ( !DecryptEncFile( targetFile, pBuffer, iBufferSize ) )
				{
					DBG_WPRINTF( L"Couldn't decrypt \"%s\"!\n", targetFile.wstring().c_str() );
					delete[] pRealBuffer;
					iThreadReturnCode = 1;
					return;
				}

				if ( ShouldRenameEncFiles() )
				{
					std::string szFileExtension = targetFile.extension().string();
					szFileExtension.erase( szFileExtension.find( 'e' ), 1 );
					targetFile.replace_extension( szFileExtension );
				}
			}						

			HANDLE hTargetFile = CreateFileW( targetFile.c_str(), GENERIC_WRITE, NULL, Q_NULLPTR, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, Q_NULLPTR );

			if ( hTargetFile == INVALID_HANDLE_VALUE )
			{
				DBG_WPRINTF( L"Couldn't create file %s!\n", targetFile.c_str() );
				delete[] pRealBuffer;
				iThreadReturnCode = 1;
				return;
			}

			DWORD dwBytesWritten = NULL;
			BOOL bResult = WriteFile( hTargetFile, pBuffer, iBufferSize, &dwBytesWritten, Q_NULLPTR );

			if ( !bResult || dwBytesWritten != iBufferSize )
			{
				DBG_WPRINTF( L"Couldn't write file %s! Result: %s Bytes written: 0x%X (%i)\n", targetFile.c_str(), bResult ? L"TRUE" : L"FALSE", dwBytesWritten, dwBytesWritten );	  				
				delete[] pRealBuffer;
				CloseHandle( hTargetFile );
				iThreadReturnCode = 1;
				return;
			}

			delete[] pRealBuffer;
			CloseHandle( hTargetFile );					
		}
	} );

	while ( std::this_thread::sleep_for( 5ms ), iCurrentEntry < m_CheckedIndexes.count() )
	{
		if ( procDlg.wasCanceled() || iThreadReturnCode != 0 )
		{
			bShouldStop = true;
			break;
		}

		procDlg.setValue( iCurrentEntry );
		procDlg.setLabelText( pCurrentNode.load()->GetFilePath().string().c_str() );
		QCoreApplication::processEvents();
	}			

	if ( extractThread.joinable() )
		extractThread.join();		

	if ( iThreadReturnCode == 1 )
	{
		STATUS_BAR_PRINT( "ExtractCheckedNodes failed!" );
		QMessageBox msgBox( "Error", "ExtractCheckedNodes failed!", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}

	if ( iThreadReturnCode == 2 )
	{
		STATUS_BAR_PRINT( "ExtractCheckedNodes was canceled!" );
		QMessageBox msgBox( "Warning", "ExtractCheckedNodes was canceled!", QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
		msgBox.exec();
		return false;
	}

	STATUS_BAR_PRINT( "Unpacked %2 files successfully", m_CheckedIndexes.size() );
	QMessageBox msgBox( "Unpacking successful", QString( "Unpacked %1 files successfully" ).arg( m_CheckedIndexes.size() ), QMessageBox::Information, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
	msgBox.exec();	   
	return true;
}
