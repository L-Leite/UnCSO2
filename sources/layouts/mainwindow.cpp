#include "mainwindow.hpp"

#include <chrono>
#include <future>
#include <thread>

#include <QDebug>
#include <QDesktopServices>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QSettings>

#include "aboutdialog.hpp"
#include "fileproperties.hpp"
#include "indexpropertiesdialog.hpp"
#include "loadindexdialog.hpp"
#include "pkgpropertiesdialog.hpp"

#include "archivefilenode.hpp"
#include "busywinwrapper.hpp"
#include "nodeextractionmgr.hpp"
#include "pkgfilesystemshared.hpp"

using namespace std::chrono_literals;
using namespace std::string_view_literals;

// the index file name should be the same in cso2 and tfo
constexpr const std::string_view INDEX_FILENAME =
    "1b87c6b551e518d11114ee21b7645a47.pkg"sv;

extern QString GetCurrentCommit();
extern QString GetAppVersion();

CMainWindow::CMainWindow( QWidget* pParent )
    : MainWindowInit( pParent ), m_Model( this ),
      m_ErrorBoxWidget( this->errorBox, this->errorBoxMsg, this->errorBoxBtn ),
      m_StatusWidget( this->lblStatus, this->pbStatus ),
      m_LastOpenDir( QDir::homePath() ), m_LastExtractDir( QDir::homePath() ),
      m_bShouldDecrypt( true ), m_bShouldDecompress( true )
{
    this->SetLoadedFilename();

    this->treeView->setModel( &this->m_Model );

    const int iLongColWidth =
        QLabel( "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn" )
            .sizeHint()
            .width();
    this->treeView->setColumnWidth( PFS_FileNameColumn, iLongColWidth );

    this->ConnectActions();
    this->CreateRecentFilesMenu();

    this->LoadSettings();

    this->m_StatusWidget.SetVisible( false );
    this->SetArchiveOptionsEnabled( false );
}

CMainWindow::~CMainWindow()
{
    this->m_TempDir.remove();
}

void CMainWindow::OnIndexFileAccepted( GameDataInfo info )
{
    if ( this->m_Model.IsGenerated() == true )
    {
        this->m_Model.ResetModel();
    }

    const fs::path indexPath = info.GetGameDataPath() / INDEX_FILENAME;

    bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w( this, tr( "Loading index's packages" ) );
        return this->DoLoadIndexJob( indexPath, info.GetGameProvider() );
    }();

    if ( bLoaded == false )
    {
        QString convertedIndexPath =
            QString::fromStdString( indexPath.generic_string() );
        this->ShowError( tr( "Failed to load index's <code>%1</code> packages" )
                             .arg( convertedIndexPath ),
                         this->m_Model.GetError() );
        return;
    }

    this->SetArchiveOptionsEnabled( true );

    // FIXME: index entry is loaded as a package
    // this->AddToRecentFiles( info.GetGameDataPath() );
    this->SetLoadedFilename( info.GetGameDataPath().generic_string() );
}

void CMainWindow::closeEvent( QCloseEvent* event )
{
    this->HandleExit();
    event->accept();
}

static bool isValidArchiveDrag( const QMimeData* data )
{
    return ( ( data->hasUrls() ) && ( data->urls().size() == 1 ) );
}

void CMainWindow::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << event;

    if ( this->m_Model.IsBusy() == true )
    {
        return;
    }

    if ( event->source() == nullptr &&
         isValidArchiveDrag( event->mimeData() ) == true )
    {
        event->acceptProposedAction();
    }
}

void CMainWindow::dropEvent( QDropEvent* event )
{
    qDebug() << event;

    if ( this->m_Model.IsBusy() == true )
    {
        return;
    }

    auto pMimeData = event->mimeData();

    if ( event->source() != nullptr ||
         isValidArchiveDrag( pMimeData ) == false )
    {
        return;
    }

    event->acceptProposedAction();

    fs::path droppedFilePath =
        pMimeData->urls().at( 0 ).toLocalFile().toStdString();
    this->LoadPackage( droppedFilePath );
}

void CMainWindow::ConnectActions()
{
    this->connect( this->action_Open, &QAction::triggered, this,
                   &CMainWindow::OnFileOpen );
    this->connect( this->action_Open_Index, &QAction::triggered, this,
                   &CMainWindow::OnIndexFileOpen );
    this->connect( this->action_Extract_All, &QAction::triggered, this,
                   &CMainWindow::OnExtractAll );
    this->connect( this->action_Properties, &QAction::triggered, this,
                   &CMainWindow::OnProperties );
    this->connect( this->action_Quit, &QAction::triggered, this,
                   &CMainWindow::OnQuitButton );

    this->connect( this->actionClear_list, &QAction::triggered, this,
                   &CMainWindow::OnRecentFileClear );

    this->connect( this->action_FilePreview, &QAction::triggered, this,
                   &CMainWindow::OnPreviewClick );
    this->connect( this->action_Extract, &QAction::triggered, this,
                   &CMainWindow::OnExtractClick );

    this->connect( this->actionDecrypt_e_files, &QAction::toggled, this,
                   &CMainWindow::OnDecryptToggle );
    this->connect( this->actionDecompress_textures, &QAction::toggled, this,
                   &CMainWindow::OnDecompressToggle );

    this->connect( this->actionAbout_UnCSO2, &QAction::triggered, this,
                   &CMainWindow::OnAbout );
    this->connect( this->actionAbout_Qt, &QAction::triggered, this,
                   &QApplication::aboutQt );

    this->connect( this->treeView, &PkgFileView::selected, &this->m_Model,
                   &PkgFileModel::OnSelectionChanged );
    this->connect( this->treeView, &PkgFileView::doubleClicked, this,
                   &CMainWindow::OnItemDoubleClicked );
}

void CMainWindow::CreateRecentFilesMenu()
{
    for ( size_t i = 0; i < this->m_RecentFileActions.max_size(); i++ )
    {
        auto pAction = std::make_shared<QAction>( this );
        pAction->setVisible( false );

        this->connect( pAction.get(), &QAction::triggered, this,
                       &CMainWindow::OnRecentFileOpen );

        this->m_RecentFileActions[i] = pAction;
        this->menu_Open_Recent->addAction( pAction.get() );
        this->menu_Open_Recent->insertAction( this->actionClear_list,
                                              pAction.get() );
    }

    this->menu_Open_Recent->insertSeparator( this->actionClear_list );
}

void CMainWindow::UpdateRecentFileActions()
{
    const int numRecentFiles =
        std::min( this->m_RecentFileNames.size(), MAINWIN_RECENT_ITEMS_NUM );

    if ( numRecentFiles == 0 )
    {
        this->menu_Open_Recent->setEnabled( false );
        return;
    }

    this->menu_Open_Recent->setEnabled( true );

    for ( int i = 0; i < numRecentFiles; i++ )
    {
        auto fileInfo = QFileInfo( this->m_RecentFileNames[i] );
        QString actionLabelText =
            tr( "%1 [%2]" ).arg( fileInfo.fileName(), fileInfo.filePath() );
        this->m_RecentFileActions[i]->setText( actionLabelText );
        this->m_RecentFileActions[i]->setData( this->m_RecentFileNames[i] );
        this->m_RecentFileActions[i]->setVisible( true );
    }

    for ( int i = numRecentFiles; i < MAINWIN_RECENT_ITEMS_NUM; i++ )
    {
        this->m_RecentFileActions[i]->setVisible( false );
    }
}

void CMainWindow::AddToRecentFiles( const fs::path& loadedFile )
{
    auto filePathStr = QString::fromStdString( loadedFile.generic_string() );

    this->m_RecentFileNames.removeAll( filePathStr );
    this->m_RecentFileNames.prepend( filePathStr );

    while ( this->m_RecentFileNames.size() > MAINWIN_RECENT_ITEMS_NUM )
        this->m_RecentFileNames.removeLast();

    this->UpdateRecentFileActions();
}

void CMainWindow::ClearRecentFiles()
{
    this->m_RecentFileNames.clear();
    this->UpdateRecentFileActions();
    this->SaveSettings();
}

void CMainWindow::SetWidgetsEnabled( bool bEnabled )
{
    this->treeView->setEnabled( bEnabled );

    auto menuList = this->menubar->findChildren<QMenu*>();

    for ( auto&& menu : menuList )
    {
        auto actionList = menu->actions();

        for ( auto&& action : actionList )
        {
            action->setEnabled( bEnabled );
        }
    }
}

void CMainWindow::SetArchiveOptionsEnabled( bool bEnabled )
{
    this->action_Extract_All->setEnabled( bEnabled );
    this->action_Properties->setEnabled( bEnabled );

    this->action_Extract->setEnabled( bEnabled );
}

void CMainWindow::SetLoadedFilename( std::string_view filename /*= {}*/ )
{
    QString windowTitle = QStringLiteral( "UnCSO2" );
    QString formattedTitle = windowTitle;

    if ( filename.empty() == false )
    {
        formattedTitle.prepend( QStringLiteral( " - " ) );
        formattedTitle.prepend( filename.data() );
    }

    this->setWindowTitle( formattedTitle );
}

void CMainWindow::LoadPackage(
    const fs::path& pkgPath, GameProvider provider /*= GameProvider::Unknown*/ )
{
    if ( this->m_Model.IsGenerated() == true )
    {
        this->m_Model.ResetModel();
    }

    bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w( this, tr( "Loading package" ) );
        return this->DoLoadPackageJob( pkgPath, provider );
    }();

    if ( bLoaded == false )
    {
        QString convertedPath =
            QString::fromStdString( pkgPath.generic_string() );
        this->ShowError(
            tr( "Failed to load package <code>%1</code>" ).arg( convertedPath ),
            this->m_Model.GetError() );
        return;
    }

    this->SetArchiveOptionsEnabled( true );

    this->AddToRecentFiles( pkgPath );
    this->SetLoadedFilename( pkgPath.filename().generic_string() );
}

bool CMainWindow::DoLoadPackageJob( const fs::path& pkgPath,
                                    GameProvider provider )
{
    std::future<bool> loadPkgFuture = std::async(
        std::launch::async,
        [&, this] { return this->m_Model.LoadPackage( pkgPath, provider ); } );

    if ( loadPkgFuture.valid() == false )
    {
        return false;
    }

    std::future_status status;

    do
    {
        status = loadPkgFuture.wait_for( 5ms );
        QCoreApplication::processEvents();
    } while ( status != std::future_status::ready );

    bool bLoaded = loadPkgFuture.get();
    return bLoaded;
}

bool CMainWindow::DoLoadIndexJob( const fs::path& indexPath,
                                  GameProvider provider )
{
    int iCurProgress = 0;
    std::size_t pkgNum = 0;

    std::future<bool> loadIndexFuture =
        std::async( std::launch::async, [&, this] {
            return this->m_Model.LoadIndex( indexPath, provider, iCurProgress,
                                            pkgNum );
        } );

    if ( loadIndexFuture.valid() == false )
    {
        return false;
    }

    std::future_status status;

    bool bShouldSetMargins = true;

    do
    {
        if ( bShouldSetMargins == true && pkgNum != 0 )
        {
            this->m_StatusWidget.SetMargins( 0,
                                             gsl::narrow_cast<int>( pkgNum ) );
            bShouldSetMargins = false;
        }

        status = loadIndexFuture.wait_for( 5ms );
        this->m_StatusWidget.SetProgressNum( iCurProgress );
        QCoreApplication::processEvents();
    } while ( status != std::future_status::ready );

    bool bLoaded = loadIndexFuture.get();
    return bLoaded;
}

bool CMainWindow::DoExtractionJob( const fs::path& outPath )
{
    int iCurrentEntry = 0;

    std::future<bool> extractFuture =
        std::async( std::launch::async, [&, this] {
            NodeExtractionMgr extractMgr(
                this->m_Model.GetLoadedPkgFiles(), outPath, iCurrentEntry,
                this->m_bShouldDecrypt, this->m_bShouldDecompress );

            auto vSelectedNodes = this->m_Model.GetCopyOfSelectedNodes();
            auto pkgParentPath = this->m_Model.GetCurrentParentPath();

            return extractMgr.ExtractNodes( vSelectedNodes, pkgParentPath );
        } );

    if ( extractFuture.valid() == false )
    {
        return false;
    }

    std::future_status status;

    do
    {
        status = extractFuture.wait_for( 5ms );
        this->m_StatusWidget.SetProgressNum( iCurrentEntry );
        QCoreApplication::processEvents();
    } while ( status != std::future_status::ready );

    bool bExtracted = extractFuture.get();
    return bExtracted;
}

bool CMainWindow::DoPreviewExtractionJob( ArchiveFileNode* pFileNode,
                                          const fs::path& outDirPath,
                                          fs::path& outResultPath )
{
    std::future<bool> previewFuture =
        std::async( std::launch::async, [&, this] {
            int unused;
            NodeExtractionMgr extractMgr(
                this->m_Model.GetLoadedPkgFiles(), outDirPath, unused,
                this->m_bShouldDecrypt, this->m_bShouldDecompress );

            auto pkgParentPath = this->m_Model.GetCurrentParentPath();

            return extractMgr.ExtractSingleFileNode( pFileNode, pkgParentPath,
                                                     outResultPath );
        } );

    if ( previewFuture.valid() == false )
    {
        return false;
    }

    std::future_status status;

    do
    {
        status = previewFuture.wait_for( 5ms );
        QCoreApplication::processEvents();
    } while ( status != std::future_status::ready );

    bool bExtracted = previewFuture.get();
    return bExtracted;
}

bool CMainWindow::DoExtractionAllJob( const fs::path& outPath )
{
    int iCurrentEntry = 0;

    std::future<bool> extractAllFuture =
        std::async( std::launch::async, [&, this] {
            const auto& loadedPkgFiles = this->m_Model.GetLoadedPkgFiles();

            std::vector<uc2::PkgFile*> vPkgFiles;
            std::transform( loadedPkgFiles.begin(), loadedPkgFiles.end(),
                            back_inserter( vPkgFiles ), []( const auto& val ) {
                                return val.second.get();
                            } );

            NodeExtractionMgr extractMgr( loadedPkgFiles, outPath,
                                          iCurrentEntry, this->m_bShouldDecrypt,
                                          this->m_bShouldDecompress );

            auto pkgParentPath = this->m_Model.GetCurrentParentPath();

            return extractMgr.ExtractPackages( vPkgFiles, pkgParentPath );
        } );

    if ( extractAllFuture.valid() == false )
    {
        return false;
    }

    std::future_status status;

    do
    {
        status = extractAllFuture.wait_for( 5ms );
        this->m_StatusWidget.SetProgressNum( iCurrentEntry );
        QCoreApplication::processEvents();
    } while ( status != std::future_status::ready );

    bool bExtracted = extractAllFuture.get();
    return bExtracted;
}

void CMainWindow::HandleExit()
{
    this->SaveSettings();
}

void CMainWindow::ValidateLastDirs()
{
    if ( fs::is_directory( this->m_LastOpenDir.toStdString() ) == false )
    {
        this->m_LastOpenDir = QDir::homePath();
    }

    if ( fs::is_directory( this->m_LastExtractDir.toStdString() ) == false )
    {
        this->m_LastExtractDir = QDir::homePath();
    }
}

void CMainWindow::LoadSettings()
{
    QSettings settings;

    settings.beginGroup( QStringLiteral( "mainwindow" ) );
    this->setGeometry(
        settings.value( QStringLiteral( "geometry" ), this->geometry() )
            .value<QRect>() );
    this->m_RecentFileNames =
        settings.value( QStringLiteral( "recentfiles" ) ).toStringList();
    settings.endGroup();

    settings.beginGroup( QStringLiteral( "settings" ) );
    this->actionDecrypt_e_files->setChecked(
        settings.value( QStringLiteral( "decryptedfiles" ), true ).toBool() );
    this->actionDecompress_textures->setChecked(
        settings.value( QStringLiteral( "decompressvtf" ), true ).toBool() );
    settings.endGroup();

    settings.beginGroup( QStringLiteral( "filedialogs" ) );
    this->m_LastOpenDir =
        settings.value( QStringLiteral( "lastopendir" ) ).toString();
    this->m_LastExtractDir =
        settings.value( QStringLiteral( "lastextractdir" ) ).toString();
    settings.endGroup();

    this->PostLoadSettings();
}

void CMainWindow::PostLoadSettings()
{
    this->UpdateRecentFileActions();
    this->ValidateLastDirs();
}

void CMainWindow::SaveSettings()
{
    QSettings settings;

    settings.beginGroup( QStringLiteral( "mainwindow" ) );
    settings.setValue( QStringLiteral( "geometry" ), this->geometry() );
    settings.setValue( QStringLiteral( "recentfiles" ),
                       this->m_RecentFileNames );
    settings.endGroup();

    settings.beginGroup( QStringLiteral( "settings" ) );
    settings.setValue( QStringLiteral( "decryptedfiles" ),
                       this->actionDecrypt_e_files->isChecked() );
    settings.setValue( QStringLiteral( "decompressvtf" ),
                       this->actionDecompress_textures->isChecked() );
    settings.endGroup();

    settings.beginGroup( QStringLiteral( "filedialogs" ) );
    settings.setValue( QStringLiteral( "lastopendir" ), this->m_LastOpenDir );
    settings.setValue( QStringLiteral( "lastextractdir" ),
                       this->m_LastExtractDir );
    settings.endGroup();
}

void CMainWindow::ShowError( const QString& message, const QString& details )
{
    this->m_ErrorBoxWidget.SetMessage( message, details );
    this->m_ErrorBoxWidget.SetVisible( true );
}

void CMainWindow::OnFileOpen()
{
    const fs::path filePath =
        QFileDialog::getOpenFileName( this, tr( "Open the PKG file" ),
                                      this->m_LastOpenDir,
                                      tr( "PKG files (*.pkg)" ) )
            .toStdString();

    if ( filePath.empty() == true )
    {
        return;
    }

    this->m_LastOpenDir =
        QString::fromStdString( filePath.parent_path().generic_string() );

    this->LoadPackage( filePath );
}

void CMainWindow::OnIndexFileOpen()
{
    CLoadIndexDialog( this ).exec();
}

void CMainWindow::OnRecentFileOpen()
{
    auto pAction = static_cast<QAction*>( this->sender() );

    if ( pAction == nullptr )
    {
        return;
    }

    fs::path filePath = pAction->data().toString().toStdString();
    this->LoadPackage( filePath );
}

void CMainWindow::OnExtractAll()
{
    const fs::path outPath =
        QFileDialog::getExistingDirectory(
            this, tr( "Select the output directory" ), this->m_LastExtractDir )
            .toStdString();

    if ( outPath.empty() == true )
    {
        return;
    }

    this->m_LastExtractDir = QString::fromStdString( outPath.generic_string() );

    bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w( this, tr( "Extracting files" ) );
        return this->DoExtractionAllJob( outPath );
    }();

    if ( bLoaded == false )
    {
        this->ShowError( tr( "Could not extract all files" ),
                         this->m_Model.GetError() );
        return;
    }
}

void CMainWindow::OnQuitButton()
{
    this->HandleExit();
    std::exit( 0 );
}

void CMainWindow::OnRecentFileClear()
{
    this->ClearRecentFiles();
}

void CMainWindow::OnDecryptToggle( bool bChecked )
{
    this->m_bShouldDecrypt = bChecked;
}

void CMainWindow::OnDecompressToggle( bool bChecked )
{
    this->m_bShouldDecompress = bChecked;
}

void CMainWindow::OnPreviewClick()
{
    std::size_t iSelectedNodesNum = this->m_Model.GetSelectedNodesCount();

    if ( iSelectedNodesNum != 1 )
    {
        return;
    }

    fs::path tempDirPath = this->m_TempDir.path().toStdString();
    fs::path outFilePath;

    const bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w( this, tr( "Previewing file" ) );

        auto vSelectedNodes = this->m_Model.GetCopyOfSelectedNodes();
        Q_ASSERT( vSelectedNodes.size() == 1 );

        auto pFileNode = BaseToFileNode( *vSelectedNodes.begin() );

        if ( pFileNode == nullptr )
        {
            return false;
        }

        return this->DoPreviewExtractionJob( pFileNode, tempDirPath,
                                             outFilePath );
    }();

    if ( bLoaded == false )
    {
        this->ShowError( tr( "Could not extract file for preview." ),
                         this->m_Model.GetError() );
        return;
    }

    QString convertedFilePath =
        QString::fromStdString( outFilePath.generic_string() );

    const bool bUrlOpened =
        QDesktopServices::openUrl( QUrl( convertedFilePath ) );

    if ( bUrlOpened == false )
    {
        this->ShowError( tr( "Could not open <code>%1</code> for view." )
                             .arg( convertedFilePath ),
                         this->m_Model.GetError() );
        return;
    }
}

void CMainWindow::OnExtractClick()
{
    std::size_t iSelectedNodesNum = this->m_Model.GetSelectedNodesCount();

    if ( iSelectedNodesNum == 0 )
    {
        return;
    }

    const fs::path outPath =
        QFileDialog::getExistingDirectory(
            this, tr( "Select the output directory" ), this->m_LastExtractDir )
            .toStdString();

    if ( outPath.empty() == true )
    {
        return;
    }

    this->m_LastExtractDir = QString::fromStdString( outPath.generic_string() );

    bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w(
            this, tr( "Extracting %1 files" ).arg( iSelectedNodesNum ) );
        return this->DoExtractionJob( outPath );
    }();

    if ( bLoaded == false )
    {
        this->ShowError( tr( "Could not extract file." ),
                         this->m_Model.GetError() );
        return;
    }
}

void CMainWindow::OnProperties()
{
    auto props = this->m_Model.GetCurrentFileProperties();

    if ( this->m_Model.IsIndexLoaded() == true )
    {
        CIndexPropertiesDialog( props, INDEX_FILENAME, this ).exec();
    }
    else
    {
        const fs::path& curModelPath = this->m_Model.GetCurrentParentPath();
        CPkgPropertiesDialog( props, curModelPath, this ).exec();
    }
}

void CMainWindow::OnAbout()
{
    CAboutDialog( this ).exec();
}

void CMainWindow::OnItemDoubleClicked( const QModelIndex& index )
{
    if ( this->m_Model.IsIndexFileNode( index ) == false )
    {
        return;
    }

    fs::path tempDirPath = this->m_TempDir.path().toStdString();
    fs::path outFilePath;

    const bool bLoaded = [&, this]() -> bool {
        CBusyWinWrapper w( this, tr( "Previewing file" ) );

        auto pFileNode = IndexToFileNode( index );

        if ( pFileNode == nullptr )
        {
            // the node should have its type verified before, since you can
            // only extract file nodes
            Q_ASSERT( false );
            return false;
        }

        return this->DoPreviewExtractionJob( pFileNode, tempDirPath,
                                             outFilePath );
    }();

    if ( bLoaded == false )
    {
        this->ShowError( tr( "Could not extract file for preview." ),
                         this->m_Model.GetError() );
        return;
    }

    QString convertedFilePath =
        QString::fromStdString( outFilePath.generic_string() );

    const bool bUrlOpened =
        QDesktopServices::openUrl( QUrl( convertedFilePath ) );

    if ( bUrlOpened == false )
    {
        this->ShowError( tr( "Could not open <code>%1</code> for view." )
                             .arg( convertedFilePath ),
                         this->m_Model.GetError() );
        return;
    }
}
