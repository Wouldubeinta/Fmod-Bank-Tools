# Fmod Bank Tools

![Download Count](https://img.shields.io/github/downloads/Wouldubeinta/Fmod-Bank-Tools/total.svg)

![Title Image](https://staticdelivery.nexusmods.com/mods/1891/images/2/2-1757393867-2013881810.jpg)

**How To Use:**  
  

-   First place bank files into the bank folder you want to extract (**Don't include Master.bank or** **Master.strings.bank, they don't have audio in them**). Once extracted, you will find the wav files in the wav folder with another folder with the same name as the bank file with the .txt wav list.  
    

  

-   To rebuild the bank files, you will need to replace the wav files with your own that has the same file type, bitrate etc. THE DURATION MUST BE SAME OR LESS THAN ORIGINAL AUDIO. If you want, you can edit the .txt file to your new wav file names. After that click Rebuild and you will find the new rebuilt .bank files in the build folder.  
    

  

-   For encrypted bank files, you will need to create a .txt file with the password in it and name it the same as the bank file and place it in the bank folder with the bank file. Example: Weapon.bank, Weapon.txt.  
    

  
## How To Find Password If The Bank Is Encrypted:
  

-   First download [https://aluigi.altervista.org/papers/fsbext.zip](https://aluigi.altervista.org/papers/fsbext.zip) and extract it to a folder, we will use this later.﻿  
    

  

-   Now copy the encrypted bank file into `Fmod_Bank_Tools\bank` and click Extract. It will say that the bank file is encrypted and it needs a password, ignore that for now. Go to the `Fmod_Bank_Tools\fsb` folder and copy that encrypted fsb file to the fsbext folder that you just downloaded.

- Open a command prompt window inside of the fsbext folder. This can be done by clicking the blank space after the file path, typing "`cmd.exe`," and pressing enter.

![opencmd](https://i.imgur.com/wCQqNwp.png)
  
- Once you are in the command prompt, type "`fsbext <YourFSB>.fsb`"
  
![typecommand](https://i.imgur.com/WROAq5Y.png)
  
- Press enter. This should result in the following:

![probably_encryption](https://i.imgur.com/3jgpyGM.jpeg)

Type ? and hit Enter and you should get this -

![encryption_types](https://i.imgur.com/vl1GSZD.jpeg)

* The password will likely be about 32 characters long. In this case, type 2 has the full password.

* If two files use the same encryption password, bits of the password will match between FSB files, while the surrounding does not.

* Most of the time, the first few characters in type 2 will be the start of your password, while the password itself appears a bit later.

* If your password is NOT fully seen here, Proceed to "Extracting the remaining password with Cheat Engine."

* If you have the entire password, proceed to "Adding password file."

* If you get the error "The version number of this file format is not supported." 
  * It is likely that you have part of the password, but it is incompletely or slightly incorrect.
  * Proceed to "Extracting the remaining password with Cheat Engine"
  
* Example of incomplete password (filtered to characters in common):

![incomplete_passwd](https://i.imgur.com/vEGr5bl.png)

## Extracting the remaining password with Cheat Engine

**Cheat engine download:**
* Download the correct Cheat Engine for your system from [the cheat engine website](https://www.cheatengine.org/downloads.php)

* **IMPORTANT**: When installing Cheat Engine, make SURE you press decline on the other software it offers you.

* Install the program you've downloaded.

* Open the game that the bank files originate from.

* With the game still open, open Cheat Engine and select the process.
![open_process_menu](https://i.imgur.com/5oZWDcw.png)
![select_process](https://i.imgur.com/njBXccn.png)

* Open the `Memory View` window.
![memview](https://i.imgur.com/7YWVo3Q.png)


* Search for the piece of the password you have in memory.
![findmem](https://i.imgur.com/oHjv4PQ.png)
![findmenu](https://i.imgur.com/eflN1UR.png)

* With any luck, the full password should be shown in the bottom right pane.
![foundpass](https://i.imgur.com/qPsVF6x.png)


## Adding password file

* Type the password into a file with the same name as your .BANK file, with a .txt extension instead.
Example:
```
Weapons.bank <-- Original bank file, to name your password file after
Weapons.txt <-- Contains the password and nothing else
```
