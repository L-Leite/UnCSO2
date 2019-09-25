#include "pkgpropertiesdialog.hpp"

#include "fileproperties.hpp"

CPkgPropertiesDialog::CPkgPropertiesDialog( const FileProperties& props,
                                            const fs::path& pkgPath,
                                            QWidget* pParent /*= nullptr*/ )
    : QDialog( pParent )
{
    this->setupUi( this );

    QString szFilename =
        QString::fromStdString( pkgPath.filename().generic_string() );
    const GameDataInfo& gameInfo = props.GetGameDataInfo();

    this->setWindowTitle( tr( "%1's properties" ).arg( szFilename ) );

    this->lblPkgName->setText( szFilename );
    this->lblGame->setText( gameInfo.GetPrintableGameName().data() );
    this->lblProvider->setText( gameInfo.GetPrintableProviderName().data() );
    this->lblEntries->setText( tr( "%1 files" ).arg( props.GetFileEntries() ) );
    this->lblEntriesEnc->setText(
        tr( "%1 files" ).arg( props.GetEncryptedFiles() ) );
    this->lblEntriesPlain->setText(
        tr( "%1 files" ).arg( props.GetPlainFiles() ) );
    this->lblMD5->setText( props.GetMd5Hash().data() );

    this->connect( this->buttonBox, &QDialogButtonBox::accepted, this,
                   &QDialog::accept );
}
