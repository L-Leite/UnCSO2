# UnCSO2
Extracts the encrypted content of Counter Strike Online 2's pkg files. At the moment only Tiancity's (chinese) version is compatible.

It can extract data from the following providers:

- **Nexon** (South Korean)

- **Tiancity** (Chinese)

## Download 
You may download the latest build of UnCSO2 [here](https://github.com/Ochii/UnCSO2/releases/latest).

## How to use
Open the program, then go to *File* > *Open Folder* and choose Counter Strike Online 2's data directory.

After loading the game's data, you may choose which files you wish to unpack by ticking their respective check box.

When you're done selecting the files, press *Unpack selected items* and choose the directory where you wish to extract the files.

### Options
In *Options* you may pick:

- **Decrypt encrypted files** - it decrypts files when needed (such as .etxt files);

- **Rename encrypted files** - renames decrypted files extension (.etxt would become .txt);

- **Decompress VTF files** - decompresses texture files in order to make them usable by games such as Counter-Strike: Source;

- **Replace toolsshadowblock.vmt** - replaces *toolsshadowblock.vmt* content with an invisible material. *toolsshadowblock.vmt* is used in most of CSO2 maps and will display a black and yellow texture if used by other Source Engine game;

- **Decompress BSP files** - decompresses map files;

- **Convert BSP lumps** - tries to convert the maps to a more Counter-Strike: Source friendly version. Note: due to stock Source Engine limitations, some maps cannot be converted;

## Bug reporting
Feel free [to open an issue](https://github.com/Ochii/UnCSO2/issues) if you find a bug in the tool.

The tool is also open to improvements, [open a pull request](https://github.com/Ochii/UnCSO2/pulls) if you have any.

## How to build

### Requirements
- [Visual Studio 2017](https://www.visualstudio.com/downloads/)
- [Qt 5.10](https://www.qt.io/download)
- [Qt VS Tools](http://doc.qt.io/qtvstools/qtvstools-getting-started.html)
- [Windows PowerShell](https://docs.microsoft.com/en-us/powershell/scripting/setup/installing-windows-powershell)

Currently only Windows is supported.

### Before building
On the project directory, run ```powershell ./setupcryptopp_win.ps1```. This Powershell script will replace a string in Crypto++'s *cryptolib.vcxproj* file to make it build with the ```MultiThreadedDLL``` instead of ```MultiThreaded``` so they can be compatible with prebuilt Qt binaries.

An alternative would be building Qt with ```MultiThreaded```.

### Building
Open the solution *UnCSO2.sln* and build it in your preferred configuration.

If built successfully, you will find your binary inside the *bin* directory.

## Credits

- Ekey for [reversing the unpacking algorithm](http://forum.xentax.com/viewtopic.php?f=21&t=11117)
- weidai11 for [Crypto++](https://www.cryptopp.com/)
- The Qt Company for [Qt](https://www.qt.io/)
- George Anescu for his [CRijndael](https://www.codeproject.com/Articles/1380/A-C-Implementation-of-the-Rijndael-Encryption-Decr) class
- [RPCS3](https://rpcs3.net/)'s team for some UI code and the Git version header generator script
- [Valve Software](https://github.com/ValveSoftware/source-sdk-2013) and [Igor Parlov](http://www.7-zip.org/) for the lzmaDecoder

For more information check the license specific file of each project
