function CreateDirectory {
    param( [string]$newDirectory)
    New-Item -ItemType Directory -Path $newDirectory
}

$curBuildCombo = $env:BUILD_COMBO
$curConfig = $env:CONFIGURATION

# only package on Release builds, but don't error out
if ($curConfig -ne 'Release') {
    Write-Host 'Non release build detected, exiting packaging script...'
    exit 0
}

$isGccBuild = $curBuildCombo -eq 'linux-gcc'
$isLinuxClangBuild = $curBuildCombo -eq 'linux-clang'
$isMingwBuild = $curBuildCombo -eq 'windows-mingw'
$isMsvcBuild = $curBuildCombo -eq 'windows-msvc'

Write-Host "Running packaging script..."
Write-Host "Current setup build combo is: $curBuildCombo"

# create dir to store package files
CreateDirectory ./build/package

if ($isLinux) {
    # copy libuncso2 to the package dir
    Copy-Item ./build/libuncso2/libuncso2.so* -Destination ./build/package/

    # copy libcryptopp to the package dir
    Copy-Item ./build/libuncso2/external/cryptopp/libcryptopp.so* -Destination ./build/package/

    # copy AppImage prebuilt files
    Copy-Item ./appimage/* -Destination ./build/package/
    
    # copy uncso2 itself to the package dir
    Copy-Item ./build/uc2 -Destination ./build/package/

}
elseif ($isWindows) {
    if ($isMingwBuild) {
        # copy crypto++ to the package dir
        Copy-Item ./build/libuncso2/external/cryptopp/libcryptopp.dll -Destination ./build/package/
    }
    
    # copy libuncso2 to the package dir
    Copy-Item ./build/libuncso2/*uncso2.dll -Destination ./build/package/
    
    # copy uncso2 itself to the package dir
    Copy-Item ./build/uc2.exe -Destination ./build/package/
}
else {
    Write-Error 'An unknown OS is running this script, implement me.'
    exit 1
}

# copy breeze icons
Copy-Item ./build/icons-breeze.rcc -Destination ./build/package/

# copy license
Copy-Item ./COPYING -Destination ./build/package/

# copy README
Copy-Item ./README.md -Destination ./build/package/

# get app version
$versionStr = Get-Content -Path ./version.txt -TotalCount 1
Write-Host "UnCSO2 version: $versionStr"

Push-Location ./build
Push-Location ./package

if ($isLinux) {
    # retrieve deployment tool
    wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt-continuous-x86_64.AppImage

    $env:VERSION = $versionStr;
    ./linuxdeployqt-continuous-x86_64.AppImage ./uc2 -bundle-non-qt-libs "-extra-plugins=iconengines,platformthemes,styles" -appimage

    if ($isGccBuild) {
        Move-Item *.AppImage -Destination "UnCSO2-$versionStr-linux64_gcc.AppImage"
    }
    elseif ($isLinuxClangBuild) {
        Move-Item *.AppImage -Destination "UnCSO2-$versionStr-linux64_clang.AppImage"
    }

    Pop-Location
}
elseif ($isWindows) {
    $windeployBin = ''

    if ($isMingwBuild) {
        $windeployBin = 'C:\Qt\5.14\mingw73_64\bin\windeployqt.exe'
    }
    elseif ($isMsvcBuild) {        
        $windeployBin = 'C:\Qt\5.14\msvc2017_64\bin\windeployqt.exe'
    }
    else {        
        Write-Error 'Unknown build combo detected.'
        exit 1
    }

    if ($curConfig -eq 'Release') {
        if ($isMingwBuild) {
            & $windeployBin ./uc2.exe  
        }
        else {
            & $windeployBin --release ./uc2.exe  
        }
    }
    else {       
        & $windeployBin ./uc2.exe  
    }

    Pop-Location

    if ($isMingwBuild) {       
        7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -ms=on "UnCSO2-$versionStr-win64_mingw.7z" ./package/*
    }
    elseif ($isMsvcBuild) {       
        7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -ms=on "UnCSO2-$versionStr-win64_msvc.7z" ./package/*
    }
}

Pop-Location
