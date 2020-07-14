# BeebEm
Port of BeebEm4 for Mac Catalina and beyond

Zip File of work in progress
https://github.com/CommanderCoder/BeebEm/raw/distribution/beebem5-distribution.zip


## Building

This version has been built using XCode 11.4.1.   You need to install this version (or later) to build, debug and otherwise work on the project. At the time of writing
the compiler emits A LOT of warnings, most of which are purely semantic and can be ignored. These
may be "fixed" in a future release.

At this point if the build fails something is wrong, most likely in your build settings / targets or
perhaps I have missed something in this document.

The app will not run in XCode unless you set the working directory to a folder containing the .ini files and ROM images.  These can be found in this repository.  The working directory setting (for debug) is found in the XCode Scheme.  Select the menu Product->Scheme->Edit Scheme...   In the 'Run' settings, select the 'Options' tab, and then locate the Working Directory tickbox.  Select it and give the working directory where 'beebem.ini' can be found.

## Creating The Installer

Creating a release is a two-step process. Firstly you must create and export an archive of the application
bundle (the .app folder), then you need to either:
- create a zip file to copy or
- create the DMG for distribution.


### Creating and exporting the archive (for copying)

There is more than one way to go about this and in the future it may be automated but for now take
the following steps:

- If you do not yet have an archive to export go to 'Archive' from the 'Project' menu. This will build your project and open the Organiser at the Archives tab
- If you already had an archive then it can be reached using the Organizer option on the Window menu
- In the organiser select the archive you wish to distribute and click the Distribute App button
- Select "Copy App" as the method of distribution. Click Next
- Select a location to save the app.  It should automatically offer a suitable location but put this into the "Install" directory
- Xcode will create the app bundle and a couple of supporting files in this folder.
- Open a terminal
- Change directory (cd) to the Install directory inside BeebEm5
- Execute ./make_distribution.sh with the name of the BeebEm archive folder (the one just created inside Install) as the 1st parameter.
e.g 
```zsh
    ./make_distribution.sh "BeebEm5 2020-07-14 15-21-10"
```

You will have a zip file call beebem5-distribution which can be copied and unzipped to other Macs.







### Creating and exporting the archive (for distributing using a DMG)

**(IGNORE THIS SECTION ..  it ends with..)**

Code signing for the **Application** is now complete - we will sign the entire DMG in the next step.

### Building the DMG installer

**(IGNORE THIS SECTION)**

For this final step there is a somewhat crude bash script supplied with the project. Open a terminal
and navigate to the Install folder. From there simply execute:

```zsh
./create_dmg.sh
```

The script will take the template disk image from the install folder, decompress it, mount it, add
the new application to it along with the sources and other user data files, unmount the image, re-compress
it and finally sign it for distribution. The resulting DMG file will be located in the folder

```zsh
'~/tmp/BeebEmMac_BUILD'
```

Before running the script again you must ensure the image is not mounted and that it has been deleted
from this folder or the script will abort and / or fail.

The resulting DMG can be zipped and used for distribution. Note that the application cannot be submitted
to the App Store in it's current state for multiple reasons but all being well the end user should
not receive any scary warnings when installing the application. Sadly the "This application was
downloaded from the internet" and the "This application needs to be updated" cannot be avoided at this
time but the user should be fairly familier with those messages.

[GO TO TOP](#building-beebem-for-mac)