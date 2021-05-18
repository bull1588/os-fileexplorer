# os-fileexplorer
Graphic File Explorer.

Jonas Bull and Jack Dooley. 5/17/2021. Assignment for CS 310 Operating Systems (UST), Spring 2021-02, Dr. Marrinan. 
## Overview
On launch, a window appears that diplays the file explorer based on the user's home directory. Navigation is done via clicking with the mouse and scrolling using the up-down keys on the keyboard. Traversing the file hierarchy is easy - just click on afolder to expand into it or click on a file to launch it.

## Code Features
The program is run through 'src/main.cpp'. Here, graphical compnents are initialized and continuously rendered. Features include:
- scrolling option using arrow keys to find results if they cannot be displayed in full in one window.
- recursive display option to shows contents of directories/folders. Option is a toggle.
- navigation of the file system through clicking folders to expand them.
- launching of files, if there is a default application for launch already set up on the device.

## Development Obstacles



