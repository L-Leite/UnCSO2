cd 'libs/cryptopp'
copy 'cryptlib.vcxproj' 'cryptlib.vcxproj.old'
(Get-Content 'cryptlib.vcxproj.old') | Foreach-Object {
    $_ -replace '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' `
   -replace '<RuntimeLibrary>MultiThreaded</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>' `
    } | Set-Content 'cryptlib.vcxproj'
rm 'cryptlib.vcxproj.old'