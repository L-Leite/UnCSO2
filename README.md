# UnCSO2
Extracts the encrypted content of Counter Strike Online 2's pkg files.

## How to use
After running the executable, go to File > Open Folder and choose Counter Strike Online 2's data directory, the program will load the packed files entries afterwards.

Then, select the files you wish to extract and press the *Unpack selected items* button. Doing this will open a file browser dialog asking you to choose the directory where you wish to keep the files.

## How to build

### Requirements
- [Visual Studio 2017](https://www.visualstudio.com/downloads/)
- [Qt 5.10](https://www.qt.io/download)
- [Qt VS Tools](http://doc.qt.io/qtvstools/qtvstools-getting-started.html)
- [Windows PowerShell](https://docs.microsoft.com/en-us/powershell/scripting/setup/installing-windows-powershell)

Currently only Windows is supported.

### Building
In order to be compatible with Qt, Crypto++ project must be set to build with /MD and /MDd. 

Go to the root of the source code and run:

```
powershell .\setupcryptopp_win.ps1
```

After that, open the solution *UnCSO2.sln* and build it in your preferred configuration.

If built successfully, you will find your binary inside the *bin* directory.

## Credits

- Ekey for [reversing the unpacking algorithm](http://forum.xentax.com/viewtopic.php?f=21&t=11117)
- weidai11 for [Crypto++](https://www.cryptopp.com/)
- The Qt Company for [Qt](https://www.qt.io/)
- George Anescu for his [CRijndael](https://www.codeproject.com/Articles/1380/A-C-Implementation-of-the-Rijndael-Encryption-Decr) class
- [RPCS3](https://rpcs3.net/)'s team for some UI code

For more information check the license specific file of each project
