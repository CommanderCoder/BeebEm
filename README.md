# BeebEm
Port of BeebEm4 by Jon Welch (https://www.g7jjf.com/download.htm) to make it will work on Mac Catalina and beyond.  Source code is also available from Stardot.org on github (https://github.com/stardot/beebem-mac) 

To futureproof the emulator I took the liberty of wrapping it in a Swift harness (not painful at all!) so it will appear a little different to normal and some features are still unavailable.  Create an issue here if you want a specific feature to be implemented.

Apple DMG (installer) of the emulator 
https://github.com/CommanderCoder/BeebEm/blob/master/Install/BeebEm5.dmg

Zip file of the DMG contents
https://github.com/CommanderCoder/BeebEm/raw/master/Install/BeebEm5.zip


#### Building
Some people have reported that it doesn't work out of the 'zip' because of attributes associated with the files. This doesn't (shouldn't) happen with the installation DMG.  If you double click on the BeebEm.app file and nothing seems to happen, open a terminal window to run this command on the ```BeebEm5-files``` folder after unzipping.

```zsh
$ xattr -crv BeebEm5-files
```

## Building

This version has been built using XCode 13.2.1. You need to install this version (or later) to build, debug and otherwise work on the project. At the time of writing
the compiler emits A LOT of warnings, most of which are purely semantic and can be ignored. I;I'll get around to removing these in a future release.

If the build fails, something other than the code will be wrong.  Most likely this will be in your build settings / targets.  Although, I may have missed something.

The app will not run in XCode unless you set the working directory to a folder containing the ```.ini``` files and ROM images.  These can be found in this repository.  The working directory setting (for debug) is found in the XCode Scheme.  Select the menu ```Product->Scheme->Edit Scheme...```   In the 'Run' settings, select the 'Options' tab, and then locate the Working Directory tick box.  Select it and give the working directory where 'beebem.ini' can be found.

## Creating The Installer

Creating a release is a two-step process. Firstly you must create and export an archive of the application
bundle (the .app folder), then you need to run the distribution script to create both:
- a zip file to copy and
- the DMG for distribution.


### Creating and exporting the archive (for copying)

Take
the following steps:

- If you do not yet have an archive to export go to 'Archive' from the 'Project' menu. This will build your project and open the Organiser at the Archives tab
- If you already had an archive then it can be reached using the Organizer option on the Window menu
- In the organiser select the archive you wish to distribute and click the Distribute App button
- Select "Copy App" as the method of distribution. Click Next
- Select a location to save the app.  It should automatically offer a suitable location but put this into the "Install" directory
- Xcode will create the app bundle and a couple of supporting files in this folder.
- Open a terminal
- Change directory (cd) to the ```Install``` directory inside BeebEm5
- Execute ./create-distribution.zsh with the name of the BeebEm archive folder (the one just created inside Install) as the 1st parameter.
e.g 
```zsh
    ./create_distribution.zsh "BeebEm5 2020-07-14 15-21-10"
```

Now you have a zip file called ```BeebEm5.zip``` which can be copied and unzipped to other Macs.

You also have a DMG file called ```BeebEm5.dmg``` which doesn't need to be unzipped. It will mount a drive with the app.

Note that the application cannot be submitted
to the App Store in its current state for multiple reasons but all being well the end user should
not receive any scary warnings when installing the application. Sadly the "This application was
downloaded from the internet" and the "This application needs to be updated" cannot be avoided at this
time but the user should be fairly familier with those messages.

[GO TO TOP](#building)
