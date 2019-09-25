#pragma once

#include <QDialog>
#include "ui_pkgpropertiesdialog.h"

#include <filesystem>
namespace fs = std::filesystem;

class FileProperties;

class CPkgPropertiesDialog : public QDialog, public Ui::PkgPropertiesDialog
{
    Q_OBJECT

public:
    CPkgPropertiesDialog( const FileProperties& props, const fs::path& pkgPath,
                          QWidget* pParent = nullptr );
};
