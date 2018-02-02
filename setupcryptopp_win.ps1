cd '../libs/cryptopp'
$result = (Get-ChildItem -Recurse -Include 'cryptlib.vcxproj' | select-string 'MultiThreadedDLL')
if ($result -ne $null)
{
	Break Script
}
copy 'cryptlib.vcxproj' 'cryptlib.vcxproj.old'
(Get-Content 'cryptlib.vcxproj.old') | Foreach-Object {
    $_ -replace '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' `
   -replace '<RuntimeLibrary>MultiThreaded</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>' `
    } | Set-Content 'cryptlib.vcxproj'
rm 'cryptlib.vcxproj.old'