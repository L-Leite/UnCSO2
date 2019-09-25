#pragma once

#include <array>

#include <QMainWindow>
#include <QTemporaryDir>

#include "ui_mainwindow.h"

#include "gamedatainfo.hpp"
#include "pkgfilemodel.hpp"
#include "widgets/errorbox.hpp"
#include "widgets/statuswidget.hpp"

class QFile;

constexpr const int MAINWIN_RECENT_ITEMS_NUM = 8;

class ArchiveFileNode;

class PkgFileModel;

//
// Call setupUi on construction time
//
// it's a hack to let us construct StatusWidget
// after setupUi is called
//
class MainWindowInit : public QMainWindow, public Ui::MainWindow
{
public:
    MainWindowInit( QWidget* pParent = nullptr ) : QMainWindow( pParent )
    {
        this->setupUi( this );
    }
};

class CMainWindow : public MainWindowInit
{
    Q_OBJECT

public:
    CMainWindow( QWidget* pParent = nullptr );
    ~CMainWindow();

public:
    void OnIndexFileAccepted( GameDataInfo info );

protected:
    virtual void closeEvent( QCloseEvent* event ) override;

private:
    virtual void dragEnterEvent( QDragEnterEvent* event ) override;
    virtual void dropEvent( QDropEvent* event ) override;

private:
    void ConnectActions();

    void CreateRecentFilesMenu();
    void UpdateRecentFileActions();
    void AddToRecentFiles( const fs::path& loadedFile );
    void ClearRecentFiles();

    void SetWidgetsEnabled( bool bEnabled );
    void SetArchiveOptionsEnabled( bool bEnabled );
    void SetLoadedFilename( std::string_view filename = {} );

    void LoadPackage( const fs::path& pkgPath,
                      GameProvider provider = GameProvider::Unknown );

    bool DoLoadPackageJob( const fs::path& pkgPath, GameProvider provider );
    bool DoLoadIndexJob( const fs::path& indexPath, GameProvider provider );

    bool DoExtractionJob( const fs::path& outPath );
    bool DoPreviewExtractionJob( ArchiveFileNode* pFileNode,
                                 const fs::path& outDirPath,
                                 fs::path& outResultPath );
    bool DoExtractionAllJob( const fs::path& outPath );

    void HandleExit();

    void ValidateLastDirs();

    void LoadSettings();
    void PostLoadSettings();
    void SaveSettings();

    void ShowError( const QString& message, const QString& details );

private slots:
    void OnFileOpen();
    void OnIndexFileOpen();
    void OnRecentFileOpen();
    void OnExtractAll();
    void OnQuitButton();

    void OnRecentFileClear();

    void OnDecryptToggle( bool bChecked );
    void OnDecompressToggle( bool bChecked );

    void OnPreviewClick();
    void OnExtractClick();

    void OnProperties();
    void OnAbout();

    void OnItemDoubleClicked( const QModelIndex& index );

private:
    PkgFileModel m_Model;

    ErrorBoxWidget m_ErrorBoxWidget;
    StatusWidget m_StatusWidget;

    std::array<std::shared_ptr<QAction>, MAINWIN_RECENT_ITEMS_NUM>
        m_RecentFileActions;
    QStringList m_RecentFileNames;

    QTemporaryDir m_TempDir;

    QString m_LastOpenDir;
    QString m_LastExtractDir;

    bool m_bShouldDecrypt;
    bool m_bShouldDecompress;

    friend class CBusyWinWrapper;
};
