BeebEm for the Apple Mac
========================

BeebEm is a BBC Micro and Master 128 emulator.  It enables you to run BBC
Micro software on your Apple Mac.

The software is a port of the Microsoft Windows version of BeebEm, currently
being developed by Mike Wyatt.

The program is still work in progress and is not complete yet so some options
available in the Windows version are not yet in the Mac version. These will be
added in due course.

This document will only describe the differences between the Mac version
and the Windows version. Please see the README.TXT file for further 
instructions in the use of the BeebEm emulator.

Installation
------------
As you are reading this, you have probably already installed the software onto
your computer. If not, just unzip the archive file to a suitable directory.

To run the program, double click the BeebEm3 application icon. This will launch
the program and (hopefully !) display a Master 128 window in the middle of your
screen.

The program has a menu bar where you can configure the program options. If you
change a menu option, remember to use Save Preferences off the Options menu
to store the program settings to disc.

See the README.TXT file for further information on the menu options.

Disc Images
-----------
There are some sample disc images in the diskimg folder. The images with an
'ssd' extension are DFS images. The four scsi?.dat files are ADFS hard disc
images. There are also some demo tunes in scsi0.dat which you can listen to with
the PLAY program. COLDTEA is a favourite of mine. The sasi0.dat file is a Torch
Z80 hard disc image.

Tape Images
-----------
There is a sample tape image in the Tapes folder. You load tape images from the
File -> Load Tape menu option. If you then *TAPE, you can *. or CH."" as normal
to catalog tapes or load programs. Further tape options are on the Comms Menu.
The Tape Control option opens a dialog box showing the blocks of data on the
tape. You can click on a block to re-position the tape, ie fast-forward or
rewind the virtual tape. You can also create a new tape by selecting the
Record button off the Tape Control dialog.

You can download disc and tape images off the internet from such sites as :

http://www.stairwaytohell.com
http://bbc.nvg.org
 

Keyboard Mapping
----------------
The keyboard is mapped as per the Windows version with the exception of :

Beeb		Mac

Caps		Left Alt
f0		F10, F11
Break		F12, F13

If you are using OS 10.4.X, you should find that the default hot key actions
for F9-F12 are disabled when BeebEm has focus. If not, ensure you have
"Enable access for assistive devices" ticked in the Universal Access preferences.

To get F9 to F12 to be recognised in OS 10.2.x or 10.3.x, you will have to 
temporarily disable these default hot keys by going into System Preferences, 
Keyboard & Mouse, Keyboard Shortcuts and unselect any shortcut using these keys.

Torch-Z80 Co Processor Support
------------------------------
The software now fully supports the Torch-Z80 co-processor board. If you do not
know what this is, you can safely ignore this next section :)

The Torch-Z80 board was an add-on card to the BBC computers which allowed you
to run CPN (a clone of CP/M) software. It was Z80 based, came with 64K of RAM
and connected to the BBC via the Tube interface. It had no hardware I/O of it's
own but used the display, keyboard, disc drives etc of the host processor.
Although it used the host disc drives, the disc formats were different to DFS
or ADFS. It still used 80 track, double sided discs (400K) but these discs
cannot be read on the BBC without special disc converting software. If you 
wish to create your own disc images of original Torch Z80 software, I use 
XFER 5.1 (http://www.g7jjf.com).

The emulator comes as default with Torch-Z80 support disabled (to prevent
confusion for new users). To configure support, you need to load a sideways
ROM image for the Host Tube support. A configuration file has already been
created for this so the simplest thing to do is replace the Roms.cfg file
with the Roms_Torch.cfg file (backing up Roms.cfg first of course !) This
file simply loads the required MCP122.ABM ROM into ROM Slot 0.

With this new configuration, running the emulator will default to a blue
Mode 0 screen. You need to the select Torch Z80 Second Processor off the 
Hardware menu (the emulator will reboot) and type *MCP to enter CPN. If you
try and run *MCP without enabling the Torch Z80 Second Processor off the
Hardware menu, you will get a 'No Z80!' error message on screen.

By default, the system will try and autoboot off Drive C and if you don't
have a suitable disc image mounted, it will bring back an error. To prevent
this, when typing *MCP, hold down the Alt key (Caps Lock) and this will
prevent CPN from autobooting.

I cannot give you a full overview of CPN but typing HELP is a good start
as well as exploring the disc images found at :

http://www.g7jjf.com/disc_images.htm

The original Torch Standard Utilities Ver 2.0 disc is included on
drive C and the Torch Hard Disc Utilities Ver 4.0 are on Drive D and
both contain several useful programs to get you going.

The Torch Z80 fully supports Econet emulation which provides TORCHNET
facilities for sharing drives amongst multiple computers or instances of
BeebEm.

If you have any specific questions about the Torch-Z80 support, please
drop me an e-mail and I will try to answer any queries you might have.

Master 512 Co Processor Support
-------------------------------
The software now supports the Master 512 Co-Processor board.

The Master 512 board was an add-on card to the BBC computers which allowed you
to run DOS Plus (based on CP/M but largely MS-DOS compatible) software. 
It was 80186 based, ran at 10 MHz came with 512K of RAM (or 1MB with third 
party upgrades) and connected to the BBC via the Tube interface.

You start the Master 512 by selecting 80186 Second Processor off the hardware
menu. This will boot off a 10MB hard disc image (ADFS Drive 0) which comes
pre-installed with the original DOS Plus system and the GEM application software.
The emulator will also recognise Drive A and B if a suitable DOS Plus formatted
disc is loaded. The emulator should recognise various disc formats (look at
the DISK program for more info) but I would only recommend using 640K ADFS .adl
format and 800K DOS Plus .img format discs.

GEM can be started by typing 'GEM' from the command prompt. GEM uses the AMX
mouse support which much be enabled for the mouse to work. I would recommend you
start GEM first then enable AMX Mouse support afterwards. Please note that mouse
support works but due to the acceleration factors built into the GEM driver, 
mouse movement can be a bit erratic. I hope to improve this in future versions 
of the emulator.

The Master 512 should run most MS-DOS 2.1 compatible software but no guarantees
are given for what will and what won't work.

I will start a library of known good disc images at :

http://www.g7jjf.com/512_disc_images.htm

Please feel free to send me any more you know work with the Master 512.

Acorn Z80 Co Processor Support
------------------------------
The software now supports the Acorn Z80 Co-Processor board.

The Acorn Z80 was a CP/M based system running with 64KB RAM on a Z80
processor.

You start the Acorn Z80 by selecting Acorn Z80 Co-Processor off the hardware
menu. If you load a CP/M system disc in drive 0, the system will then boot
to an A: prompt.

I have included a CP/M Utilities disc in the installation which will boot
the system. The Acorn Z80 originally came with 7 discs which can be download
from my web site at :

http://www.g7jjf.com/acornz80_disc_images.htm

These discs contain the Acorn Z80 application software including :

CPM Utilities
BBC Basic
Mallard Basic
CIS Cobol
MemoPlan
GraphPlan
FilePlan
Accountant
Nucleus

You will also find some manuals for this software on my site as well.

The Acorn Z80 should also run most CP/M compatible software but no guarantees
are given for what will and what won't work.

I will start a library of known good disc images at :

http://www.g7jjf.com/acornz80_disc_images.htm

Please feel free to send me any more you know work with the Acorn Z80.

Acorn ARM Second Processor Support
----------------------------------
The software now supports the Acorn ARM Second Processor board, also
known as the ARM Evaluation System.

The ARM interpreter and disassembler code is based on software written
by David Sharp to whom I am grateful for allowing me to use it with BeebEm.

The Acorn ARM Second Processor was an 8MHz system running with 4MB RAM on an
ARM 1 RISC processor.

You start the Acorn ARM by selecting ARM Second Processor off the hardware
menu. The ARM Second Processor has no operating system but relies on a sideways
ROM to provide a command line prompt to the user. Once you see the A* prompt, type
*HELP SUPERVISOR to see what inbuilt commands are available.

The ARM Evaluation System came with 6 discs full of software. I have included disc
3 in the installation which contains ARM Basic. Load the armdisc3.adl disc image
(ensuring you are in ADFS mode), type *MOUNT 4 and *AB to load ARM Basic.
The remaining disc images plus a hard disc image containing all 6 disc images can be
downloaded from my web site at :

http://www.g7jjf.com/acornArm_disc_images.htm

These discs contain the Acorn ARM application software including :

TWIN - Editor
ARM Assembler/Linker/Debugger
ARM Basic
Lisp
Prolog
Fortran 77
General File Utilities

You will also find some manuals for the ARM Second Processor on my site as well but
if you have any manuals or other disc images I don't have, please feel free to send me 
copies to add to the library.


Teletext Support
----------------
The software now supports the Acorn Teletext Adapter. Unfortunately, I
haven't got my teletext server software finished yet so the emulation
can only work from teletext data captured to disc files. You can download
some sample teletext data from :

http://www.g7jjf.com/teletext.htm

Speech Generator
----------------
The BBC B and B+ had the facility for installing a TMS5220 Speech Generator
chip and associated TMS6100 PHROM containing the digitised voice of the BBC
newsreader Kenneth Kendall. With these two chips installed, you could easily
make the computer speak using the BASIC SOUND command. BeebEm now supports
the TMS5220 and upto 16 different PHROM's. A new configuration file has been
created to specify which PHROM's are available. The configuration file is
phroms.cfg and contains a list of 16 PHROM filenames or EMPTY if a PHROM
is not available. Two PHROM's are currently included with the emulator and
are stored in the Phroms directory. They are PHROMA which was the original
Kenneth Kendall voice and PHROMUS which is an American voice speaking a
different dictionary.

To say a word, you use the SOUND command, eg :

SOUND -1, 179, 0, 0 would speak COMPUTER using PHROMA.

The -1 specifies the PHROM number to use, from -1 to -16 which corresponds
to each entry in the phroms.cfg file.

A list of words available in PHROMSUS is included in the Phroms directory for
reference. A list for PHROMA can be found in Appendix A and B of the manual
detailed below.

The Torch Z80 Co Processor also supports the speech system. Try the SNAKE game
off the system utilities disc. There is also BACKCHAT.

Please remember that the Master 128 doesn't support speech so you need to be
in Model B/B+ mode for speech to work.

For more information on using the Speech System you are referred to the Acorn 
Speech System User Guide which you can download from :

http://www.g7jjf.com/docs/acorn_speech_user_guide.pdf  (1202KB)

If anyone has any more PHROM images, can they please email them to me so
I can include them with the distribution.

Source Code
-----------
The full source code for the program is in the src sub-directory. You will
need the XCode 3.0 development tools to be able to modify and re-compile
the program.

Performance
-----------
In the default configuration and a 640x512 screen, the program runs at 
approx 38fps on my Mac Mini 1.4GHz. Running full screen (ie, click 
green + button or cmd-Z) the speed drops to about 20 fps but is still fully playable
with no sound break up.

Missing Functions
-----------------
The following functions have yet to be ported across :

Some items off Comms menu
Some items off Options menu

I can live without these functions but if you have a preference for items
you think ought to go back in, please let me know.

Known Problems
--------------
There are currently no known problems with the emulator.

If you do find any problems, please let me know.

This is my first attempt at writing Mac software and I've yet to fully
come to terms with the hugh library of Carbon API's. If you have any
suggestions for doing things a better way, please get in touch.
I am especially interested in how I can improve the frame rate of the
program. The CopyBits function seems awfully slow :)

Version History
---------------
3.3a - 09/02/08
Added disc LED lights in 8271 mode (code by Richard Drysdall)
Added 1770 controller options in Model B mode
Added support for Watford Double Density 720K disc images
(use file extension .wdd to recognise disc image)

3.3 - 01/01/08
Added remote client/server ethernet option for serial port emulation(so you could write software to telnet to a remote network, write a simple web browser or ftp client etc, running on the Beeb)Added print screen functionAdded copy screen to clipboard functionAdded three new higher resolution screen modes (code by Richard Drysdall)Added freeze when minimised optionAdded load/save user keyboard layouts (code by Richard Drysdall)
Added protect on load disc option
Bug fix - Missing scan line at top of screenBug fix - Missing scan lines at bottom of screenBug fix - Non-default window size not set on startupBug fix - Random crash when changing from mode 7 to any other screen modeBug fix - Cursor position wrong when in column 1 in editing modeSlight tweak to source code to get it to compile using Xcode 3 on LeopardSlight change to sound routine to remove some artefacts

3.2a - 15/09/07
Added clipboard as destination for printerAdded cmd-C to copy BASIC program to clipboardAdded cmd-V to paste BASIC program from clipboard(alternatives to using *SPOOL/*EXEC with disc files)Corrected emulation of PLY processor instructionAdded new program and disc icons
Set root directory to $ for new ADFS images
Fixed keyboard interrupt handling routine
Fixed disc write protect menu update after ejecting a disc
Increased default Econet flag fill timeout to 250000

3.2 - 14/05/07
Added ARM Second Processor support

3.1 - 01/01/07
Added serial port supportAdded preliminary 300 baud support for CSW, Swarm now loadsAdded hard disc activity LED'sAdded CSW support to Tape Control windowAdded support for Level 3 Econet User Port RTC ModuleAdded support for mixed mode ADFS/NETFS format discs

3.0x - 19/11/06
Added preliminary 300 baud support for CSW, Swarm now loadsAdded hard disc activity LED'sAdded CSW support to Tape Control windowAdded support for Level 3 Econet User Port RTC ModuleAdded support for mixed mode ADFS/NETFS format discs

3.0w - 07/10/06
Added cmd-P screen dump command
(To create bmp pictures for BBC Games List program)

3.0v - 04/10/06
Fixed problem of saving to ADFS hard disc under 65C02 emulation
Altered launching of BeebEm by clicking on image file to autorun disc
(for compatibility with new BBC Games List database frontend program)

3.0u - 13/09/06
Added preliminary support for loading CSW format tape imagesAdded emulation of Microvitec touch screenAdded user port breakout box
Fixed joystick emulation in Master 128 mode
Fixed problem of ESC key not always being detectedFixed problem of accessing files when ADFS and DFS discs loaded side by sideFixed a couple of tape related game loading problems
Fixed interrupt clearing issue in 8271 disk emulation
Minor VIA timing tweak to make Snapper work again

3.0t - 20/08/06
Rebuilt application as universal binary
Added UDP broadcast support for TeleText server software
Added changes done by Mike Wyatt to BeebEm Ver 3.3, namely :
Added menu options to enable/disable Teletext adapter and hard drive emulation.
Removed ADFS and ATS ROMs from Model-B configuration as they were causing
a few problems.
Improved VIA and interrupt timing and fixed instruction cycle count for 
branches.  The following programs now run:
  Nightshade (tape), Lancelot, The Empire Strikes Back, Dabs Fingerprint,
  Yie Ar Kung-Foo (tape)
Added "Eject Disc" options to the file menu.  The name of the currently 
loaded image file is shown next to the menu option.
Change sound volume to be exponential (suggested by Rich Talbot-Watkins).
Seems to be a definite improvement on the linear scale.  There is a menu
option to switch between the two.

3.0s - 01/04/06 (my 40th birthday if anyone is interested !)
Added support for Acorn Z80 Co-Processor

3.0r - 26/03/06
Added support for Master 512 Co-Processor
Added support for Acorn Teletext Adapter

3.0q - 30/12/05
Added SCSI ADFS Hard Disc support
Added SASI Torch Z80 Hard Disc support
Added Econet support (see README.TXT for more information)
(Therefore TORCHNET works using the Torch Z80 Co-Processor)
Removed IDE support as no longer needed

3.0p - 24/12/05
Added emulation of TMS5220/TMC6100 Speech Generator

3.0o - 20/12/05
Added 6502/Z80 Debugger
You may need to drop the frame rate when running the debugger
as it does slow down the emulation speed, eg View -> 25 FPS.
See the README.TXT file for further information on the debugger.

3.0n - 16/12/05
Added Motion Blur option
Added tape relay sounds
Fixed problem with case-sensitive filing systems
Fixed tape control window not scrolling properly
Fixed tape control window showing extra block data

3.0m - 22/11/05
Added Full Screen display mode (toggle with cmd-Z)
Merged in BeebEm for Windows bug fixes, namely :
Improved VIA timing emulation and fixed some instruction cycle counts
Fixed bug in horizontal displayed register emulation
Fixed bug in virtical sync position register emulation
Fixed bug in virtical displayed/total register emulation for mode 7

3.0l - 01/11/05
Added Skip Frame and Frame Size options to video capture

3.0k - 28/10/05
Added QuickTime recording of running program

3.0j - 25/09/05
Added user defined keyboard mapping

3.0i - 23/09/05
Added key mapping for '@' character (forgot to put it in !)
Added support for tape images

3.0h - 21/08/05
Added support for 16 bit colour displays

3.0g - 17/08/05
Fixed problem of emulator sometimes dropping to 0 fps
Fixed problem of red close button forcing emulator full screen

3.0f - 07/08/05
Added support for the Torch Z80 co-processor card
Changed compiler to use GCC 3.3 so works on 10.3.7 without missing library errors
Fix to remove _kCGColorSpaceGenericRGB error on pre 10.4.x OS's

3.0e - 16/07/05
Increased screen refresh rate by using custom CopyBits procedure
Added support for launching BeebEm by selecting disc image from Finder
Added support for disabling F9-F12 hot keys on Tiger OS
Changed compiler options to use weak linking so the program should 
load under OS 10.2.x and 10.3.x
Added program icons

3.0d - 14/07/05
Added support for analogue mouse stick
Added support for digital mouse stick
Added support for AMX mouse

3.0c - 12/07/05
Added support for F9-F12
Remapped Break key to F12
Remapped F0 key to F10
Added Map A,S to CAPS,CTRL option

3.0b - 11/07/05
Added Save Preferences menu option (stored in beebem.ini)
Added New Disc 0/1 menu options
Added Load/Save State
Added Quick Load/Save State
Added Printer options

3.0a - 04/07/05
Initial Release

Jon Welch
jon@g7jjf.com
http://www.g7jjf.com
9th February 2008
