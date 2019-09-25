#pragma once

#include <memory>

#include <QApplication>
#include "mainwindow.hpp"

class CUnCSO2App : public QApplication
{
    Q_OBJECT

public:
    CUnCSO2App( int& argc, char** argv );
    ~CUnCSO2App() = default;

private:
    void SetupSettingsInfo();

    static bool ShouldLoadOwnIcons();
    static void LoadOwnIcons();

private:
    std::unique_ptr<CMainWindow> m_pMainWindow;
};
