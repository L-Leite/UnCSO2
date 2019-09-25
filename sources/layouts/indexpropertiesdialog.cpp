#include "indexpropertiesdialog.hpp"

#include "fileproperties.hpp"

CIndexPropertiesDialog::CIndexPropertiesDialog( const FileProperties& props,
                                                std::string_view indexFilename,
                                                QWidget* pParent /*= nullptr*/ )
    : QDialog( pParent )
{
    this->setupUi( this );

    const GameDataInfo& gameInfo = props.GetGameDataInfo();

    this->lblPkgName->setText( indexFilename.data() );
    this->lblGame->setText( gameInfo.GetPrintableGameName().data() );
    this->lblProvider->setText( gameInfo.GetPrintableProviderName().data() );
    this->lblPkgNum->setText(
        tr( "%1 packages" ).arg( props.GetPkgFilesNum() ) );
    this->lblEntries->setText( tr( "%1 files" ).arg( props.GetFileEntries() ) );
    this->lblEntriesEnc->setText(
        tr( "%1 files" ).arg( props.GetEncryptedFiles() ) );
    this->lblEntriesPlain->setText(
        tr( "%1 files" ).arg( props.GetPlainFiles() ) );

    this->connect( this->buttonBox, &QDialogButtonBox::accepted, this,
                   &QDialog::accept );
}
