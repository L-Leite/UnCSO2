#include "aboutdialog.hpp"

#include <uc2/uc2version.hpp>
#include "uc2_version.hpp"

CAboutDialog::CAboutDialog( QWidget* pParent /*= nullptr*/ )
    : QDialog( pParent )
{
    this->setupUi( this );

    this->verLabel->setText( UNCSO2_VERSION );
    this->commitLabel->setText( GIT_COMMIT_HASH "-" GIT_BRANCH );
    this->libVerLabel->setText( uc2::Version::GetVersionString().data() );
}
