#include "busywinwrapper.hpp"

#include "mainwindow.hpp"

CBusyWinWrapper::CBusyWinWrapper( CMainWindow* window,
                                  const QString& newLabel /*= {}*/ )
    : m_pWindow( window )
{
    this->m_pWindow->SetWidgetsEnabled( false );
    this->m_pWindow->m_StatusWidget.SetVisible( true );
    this->m_pWindow->m_StatusWidget.SetLabel( newLabel );
    this->m_pWindow->m_StatusWidget.SetMargins( 0, 0 );

    QGuiApplication::setOverrideCursor( Qt::WaitCursor );
}

CBusyWinWrapper::~CBusyWinWrapper()
{
    QGuiApplication::restoreOverrideCursor();

    this->m_pWindow->m_StatusWidget.SetVisible( false );
    this->m_pWindow->SetWidgetsEnabled( true );
}
