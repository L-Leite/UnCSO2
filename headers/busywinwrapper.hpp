#pragma once

#include <QString>

class CMainWindow;

class CBusyWinWrapper
{
public:
    CBusyWinWrapper( CMainWindow* window, const QString& newLabel = {} );
    ~CBusyWinWrapper();

private:
    CMainWindow* m_pWindow;
};
