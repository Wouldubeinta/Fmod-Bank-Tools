# Fmod Bank Tools

![Download Count](https://img.shields.io/github/downloads/Wouldubeinta/Fmod-Bank-Tools/total.svg)

![Title Image](https://staticdelivery.nexusmods.com/mods/1891/images/2/2-1757393867-2013881810.jpg)

**How To Use:**  
  

-   First place bank files into the bank folder you want to extract (**Don't include Master.bank or** **Master.strings.bank, they don't have audio in them**). Once extracted, you will find the wav files in the wav folder with another folder with the same name as the bank file with the .txt wav list.  
    

  

-   To rebuild the bank files, you will need to replace the wav files with your own that has the same file type, bitrate etc. THE DURATION MUST BE SAME OR LESS THAN ORIGINAL AUDIO. If you want, you can edit the .txt file to your new wav file names. After that click Rebuild and you will find the new rebuilt .bank files in the build folder.  
    

  

-   For encrypted bank files, you will need to create a .txt file with the password in it and name it the same as the bank file and place it in the bank folder with the bank file. Example: Weapon.bank, Weapon.txt.  
    

  
**How To Find Password If The Bank Is Encrypted:**  
  

-   First download [https://aluigi.altervista.org/papers/fsbext.zip](https://aluigi.altervista.org/papers/fsbext.zip) and extract it to a folder, we will use this later.﻿  
    

  

-   Now copy the encrypted bank file into the bank folder in the Fmod Bank Tools and click Extract. It will come up saying that the bank file is encrypted and it needs a password, just ignore that for now. Go to the fsb folder and copy that encrypted fsb file to the fsbext folder that you just downloaded. Know create a batch file and add the following command -

![Image 1](https://i.imgur.com/zqRUCk2.jpeg)

Where it's got Weapons.fsb, rename it to your .fsb file.  
  
-   Ok load up the batch file, it should show something like this -

![Image 2](https://i.imgur.com/3jgpyGM.jpeg)

Type ? and hit Enter and you should get this -

![Image 3](https://i.imgur.com/vl1GSZD.jpeg)

I have the password highlighted under the - encryption type 2. You will see under the highlight password it's the same password, that means you have the right one.  
  

-   Now copy that password and add it to a txt file and name it the same name as the bank file and place it into the bank folder.  
 
Now your ready to extract and rebuild the encrypted bank file.
