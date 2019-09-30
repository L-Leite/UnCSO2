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
    Copy-Item ./build/uc2 ./build/package/
}
elseif ($isWindows) {
    if ($isMingwBuild) {
        # copy libcryptopp to the same directory of uc2
        cp ./build/libuncso2/libuncso2.dll ./build/package/

        # copy libuncso2 to the same directory of uc2
        cp ./build/libuncso2/external/cryptopp/libcryptopp.dll ./build/package/
    }
    elseif ($isMsvcBuild) {
        # copy libcryptopp to the same directory of uc2
        cp ./build/libuncso2/uncso2.dll ./build/package/
    }
    else {        
        Write-Error 'Unknown build combo detected.'
        exit 1
    }
    
    # copy uncso2 itself to the package dir
    Copy-Item ./build/uc2.exe ./build/package/
}
else {
    Write-Error 'An unknown OS is running this script, implement me.'
    exit 1
}

# copy breeze icons
Copy-Item ./build/icons-breeze.rcc -Destination ./build/package/

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
    ./linuxdeployqt-continuous-x86_64.AppImage ./uc2 -bundle-non-qt-libs -appimage
}
elseif ($isWindows) {
    $windeployBin = ''

    if ($isMingwBuild) {
        $windeployBin = 'C:\Qt\5.13\mingw73_64\bin\windeployqt.exe'
    }
    elseif ($isMsvcBuild) {        
        $windeployBin = 'C:\Qt\5.13\msvc2017_64\bin\windeployqt.exe'
    }
    else {        
        Write-Error 'Unknown build combo detected.'
        exit 1
    }

    if ($curConfig -eq 'Release') {       
        & $windeployBin --release ./uc2.exe  
    }
    else {       
        & $windeployBin ./uc2.exe  
    }  
}

Pop-Location

if ($isWindows) {
    7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -ms=on "UnCSO2-$versionStr-win64.7z" ./package/*
}

Pop-Location
