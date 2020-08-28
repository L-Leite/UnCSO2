function CreateDirectory {
    param( [string]$newDirectory)
    New-Item -ItemType Directory -Path $newDirectory
}

function SetupVsToolsPath {
    # from https://allen-mack.blogspot.com/2008/03/replace-visual-studio-command-prompt.html

    # split location to shorten the command
    Push-Location 'C:\Program Files (x86)\Microsoft Visual Studio\2017'
    Push-Location '.\Community\VC\Auxiliary\Build'

    cmd /c "vcvars64.bat&set" |
    ForEach-Object {
        if ($_ -match "=") {
            $v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
        }
    }

    Pop-Location
    Pop-Location
}

$curBuildCombo = $env:BUILD_COMBO
$curConfig = $env:CONFIGURATION

$isMsvcBuild = $curBuildCombo -eq 'windows-msvc'

$clangFsOption = 'DUNCSO2_USE_CLANG_FSAPI=OFF';

Write-Host "Running build script..."
Write-Host "Current script build combo is: $curBuildCombo $curConfig"

$targetCompilerCC = '';
$targetCompilerCXX = '';

switch ($curBuildCombo) {
    "linux-gcc" {
        $targetCompilerCC = 'gcc-9'
        $targetCompilerCXX = 'g++-9'
        break
    }
    "linux-clang" {
        $targetCompilerCC = 'clang-10'
        $targetCompilerCXX = 'clang++-10'
        $clangFsOption = 'DUNCSO2_USE_CLANG_FSAPI=ON';
        break
    }
    "windows-mingw" {
        $targetCompilerCC = 'gcc'
        $targetCompilerCXX = 'g++'
        break
    }
    "windows-msvc" {
        $targetCompilerCC = 'cl'
        $targetCompilerCXX = 'cl'
        # add msvc 17 tools to path
        SetupVsToolsPath
        break
    }
    Default {
        Write-Error 'Unknown build combo used, could not find appropriate compiler.'
        exit 1
    }
}

Write-Host "Selected C compiler: $targetCompilerCC"
Write-Host "Selected C++ compiler: $targetCompilerCXX"

# create build dir
CreateDirectory ./build

# go to build dir
Push-Location ./build

if ($isMsvcBuild) {
    cmake -G "Visual Studio 16 2019" -A "x64" ../ 
}
else {
    cmake -G "Ninja" `
        -DCMAKE_CXX_COMPILER="$targetCompilerCXX" `
        -DCMAKE_C_COMPILER="$targetCompilerCC" `
        -DCMAKE_BUILD_TYPE="$curConfig" `
        $clangFsOption `
        ../
}

if ($LASTEXITCODE -ne 0) {
    Write-Error 'Failed to generate CMake configuration files.'
    exit 1
}

if ($isMsvcBuild) {
    msbuild uc2.sln /p:Configuration=$curConfig
}
else {
    ninja all
}

if ($LASTEXITCODE -ne 0) {
    Write-Error 'Failed to build project.'
    exit 1
}

# go back to the project's dir
Pop-Location
