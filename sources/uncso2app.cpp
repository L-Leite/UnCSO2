#include "uncso2app.hpp"

#include <QDebug>
#include <QResource>

#include "uc2_version.hpp"

CUnCSO2App::CUnCSO2App( int& argc, char** argv ) : QApplication( argc, argv )
{
    this->SetupSettingsInfo();

    if ( CUnCSO2App::ShouldLoadOwnIcons() == true )
    {
        CUnCSO2App::LoadOwnIcons();
    }

    this->m_pMainWindow = std::make_unique<CMainWindow>();
    this->m_pMainWindow->show();
}

void CUnCSO2App::SetupSettingsInfo()
{
    this->setOrganizationName( "Luís Leite" );
    this->setOrganizationDomain( "leite.xyz" );
    this->setApplicationName( "UnCSO2" );
    this->setApplicationVersion( QStringLiteral( UNCSO2_VERSION ) );
}

bool CUnCSO2App::ShouldLoadOwnIcons()
{
    QString sampleIconName = QStringLiteral( "document-open" );
    return QIcon::hasThemeIcon( sampleIconName ) == false;
}

void CUnCSO2App::LoadOwnIcons()
{
    qDebug() << "Theme doesn't have the 'document-open' icon, loading our "
                "own icons";

    auto iconsResourceFilename = QStringLiteral( "/icons-breeze.rcc" );

    bool bIconRccLoaded = QResource::registerResource(
        QGuiApplication::applicationDirPath() + iconsResourceFilename );

    if ( bIconRccLoaded == false )
    {
        qDebug() << "Could not load fallback Breeze icons";
    }

    QIcon::setThemeName( QStringLiteral( "Breeze" ) );
}
