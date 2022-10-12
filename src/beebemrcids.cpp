//
//  beebemrcids.cpp
//  BeebEm5
//
//  Created by Commander Coder on 05/10/2022.
//  Copyright © 2022 Andrew Hague. All rights reserved.
//

#include <stdio.h>

#include "beebemrcids.h"

std::map<int,int> ID2RC =
{
{'sfps', IDM_SPEEDANDFPS},
{'rund', IDM_RUNDISC},
{'opn0', IDM_LOADDISC0},
{'opn1', IDM_LOADDISC1},
{'new0', IDM_NEWDISC0},
{'new1', IDM_NEWDISC1},
{'rest', ID_FILE_RESET},
{'bbcb', ID_MODELB},
{'bbci', ID_MODELBINT},
{'bbcp', ID_MODELBPLUS},
{'bbcm', ID_MASTER128},
{'tnil', IDM_TUBE_NONE},
{'tube', IDM_TUBE_ACORN65C02},
//    {'t186', whatever happend to tube 186? },
{'t512', IDM_TUBE_MASTER512  }, //new
{'az80', IDM_TUBE_ACORNZ80  },
{'tz80', IDM_TUBE_TORCHZ80  },
{'tarm', IDM_TUBE_ACORNARM  },
{'tspr', IDM_TUBE_SPROWARM  }, //new
{'sRT ', IDM_REALTIME },
{'s50 ', IDM_50FPS},
{'s25 ', IDM_25FPS},
{'s10 ', IDM_10FPS},
{'s5  ', IDM_5FPS },
{'s1  ', IDM_1FPS },
{'f100', IDM_FIXEDSPEED100 },
{'f5  ', IDM_FIXEDSPEED5   },
{'f2  ', IDM_FIXEDSPEED2   },
{'f1.5', IDM_FIXEDSPEED1_5 },
{'f1.2', IDM_FIXEDSPEED1_25},
{'f1.1', IDM_FIXEDSPEED1_1 },
{'f0.9', IDM_FIXEDSPEED0_9 },
{'f0.7', IDM_FIXEDSPEED0_75},
{'f50 ', IDM_FIXEDSPEED50  },
{'f0.2', IDM_FIXEDSPEED0_25},
{'f10 ', IDM_FIXEDSPEED10  },
{'f0.5', IDM_FIXEDSPEED0_5 },
{'f0.1', IDM_FIXEDSPEED0_1 },
{'monr', ID_MONITOR_RGB  },
{'monb', ID_MONITOR_BW   },
{'mona', ID_MONITOR_AMBER},
{'mong', ID_MONITOR_GREEN},
{'wrp0', IDM_WPDISC0},
{'wrp1', IDM_WPDISC1},
{'wpol', IDM_WPONLOAD},
{'siz1', ID}, //160x128 selected
{'siz2', ID},
{'siz3', ID},
{'siz4', ID},
{'siz5', ID},
{'siz6', ID},
{'siz7', ID},
{'siz8', ID},
{'siz9', ID},
{'siza', ID},
{'sizb', ID_VIEW_DD_1600X1200},//1600x1200 selected
{'vfsc', ID},//View full screen selected
{'vmar', IDM_MAINTAINASPECTRATIO},//Maintain Aspect Ratio selected
{'volh', IDM_HIGHVOLUME  },
{'volm', IDM_MEDIUMVOLUME},
{'voll', IDM_LOWVOLUME   },
{'volf', IDM_FULLVOLUME},
{'sndc', IDM_SOUNDCHIP},
{'sond', IDM_SOUNDONOFF},
{'sped', IDM_SPEECH}, //--gone
{'enet', ID_ECONET},
{'igil', ID},//Ignore Illegal Instructions - see OPCODES
{'hard', ID_BASIC_HARDWARE_ONLY},//Basic Hardware
{'docu', ID},//Documented Only
{'extr', ID},//Common Extras
{'full', ID},//Full Set
{'ifd0', IDM_DISC_IMPORT_0},
{'ifd1', IDM_DISC_IMPORT_1},
{'ifd2', IDM_DISC_IMPORT_2},
{'ifd3', IDM_DISC_IMPORT_3},
{'efd0', IDM_DISC_EXPORT_0},
{'efd1', IDM_DISC_EXPORT_1},
{'efd2', IDM_DISC_EXPORT_2},
{'efd3', IDM_DISC_EXPORT_3},
{'roma', IDM_ALLOWWRITES_ROM0},
{'romb', IDM_ALLOWWRITES_ROM1},
{'romc', IDM_ALLOWWRITES_ROM2},
{'romd', IDM_ALLOWWRITES_ROM3},
{'rome', IDM_ALLOWWRITES_ROM4},
{'romf', IDM_ALLOWWRITES_ROM5},
{'romg', IDM_ALLOWWRITES_ROM6},
{'romh', IDM_ALLOWWRITES_ROM7},
{'romi', IDM_ALLOWWRITES_ROM8},
{'romj', IDM_ALLOWWRITES_ROM9},
{'romk', IDM_ALLOWWRITES_ROMA},
{'roml', IDM_ALLOWWRITES_ROMB},
{'romm', IDM_ALLOWWRITES_ROMC},
{'romn', IDM_ALLOWWRITES_ROMD},
{'romo', IDM_ALLOWWRITES_ROME},
{'romp', IDM_ALLOWWRITES_ROMF},
{'ledr', ID_RED_LEDS     },
{'ledg', ID_GREEN_LEDS   },
{'ledk', ID_SHOW_KBLEDS  },
{'ledd', ID_SHOW_DISCLEDS},
{'savp', IDM_SAVE_PREFS},
{'prop', IDM_AUTOSAVE_PREFS_ALL},
{'qukl', IDM_QUICKLOAD},
{'quks', IDM_QUICKSAVE},
{'rsts', IDM_LOADSTATE},
{'savs', IDM_SAVESTATE},
{'prnt', IDM_PRINTERONOFF},
{'pfle', IDM_PRINTER_FILE},
{'pclp', IDM_PRINTER_LPT1},
{'pcl2', IDM_PRINTER_LPT2}, //new vv
{'pcl3', IDM_PRINTER_LPT3},
{'pcl4', IDM_PRINTER_LPT4},
{'pcc1', IDM_PRINTER_COM1},
{'pcc2', IDM_PRINTER_COM2},
{'pcc3', IDM_PRINTER_COM3},
{'pcc4', IDM_PRINTER_COM4}, //new ^^
{'kmas', IDM_MAPAS},
{'copy', IDM_EDIT_COPY},
{'past', IDM_EDIT_PASTE},
{'trac', ID}, // trace186
{'amxo', IDM_AMXONOFF       },
{'amx3', IDM_AMX_320X256    },
{'amx6', IDM_AMX_640X256    },
{'amx1', IDM_AMX_160X256    },
{'axp5', IDM_AMX_ADJUSTP50  },
{'axp3', IDM_AMX_ADJUSTP30  },
{'axp1', IDM_AMX_ADJUSTP10  },
{'axm1', IDM_AMX_ADJUSTM10  },
{'axm3', IDM_AMX_ADJUSTM30  },
{'axm5', IDM_AMX_ADJUSTM50  },
{'amxl', IDM_AMX_LRFORMIDDLE},
{'ofwm', IDM_FREEZEINACTIVE},
{'msea', IDM_ANALOGUE_MOUSESTICK},
{'msed', IDM_DIGITAL_MOUSESTICK},
{'opnt', ID_LOADTAPE},
{'tpfa', ID_TAPE_FAST  },
{'tpmf', ID_TAPE_MFAST },
{'tpms', ID_TAPE_MSLOW },
{'tpno', ID_TAPE_NORMAL},
{'tpso', ID_TAPESOUND},
{'tpcr', ID_SFX_RELAY},
{'ddso', ID_SFX_DISCDRIVES},
{'tpre', ID_REWINDTAPE},
{'tpul', ID_UNLOCKTAPE},
{'tpco', IDD_TAPECONTROL},
{'dbgr', IDM_SHOWDEBUGGER},
{'upbo', ID},
{'uprm', ID},
{'abou', IDM_ABOUT},
{'kusr', IDM_DEFINEKEYMAP},
{'lukm', IDM_LOADKEYMAP},
{'sukm', IDM_SAVEKEYMAP},
{'udkm', IDM_USERKYBDMAPPING},
{'dkm ', IDM_DEFAULTKYBDMAPPING},
{'lkm ', IDM_LOGICALKYBDMAPPING},
{'vidc', IDM_CAPTUREVIDEO},
{'vide', IDM_ENDVIDEO},
{'skp0', IDM_VIDEOSKIP0},
{'skp1', IDM_VIDEOSKIP1},
{'skp2', IDM_VIDEOSKIP2},
{'skp3', IDM_VIDEOSKIP3},
{'skp4', IDM_VIDEOSKIP4},
{'skp5', IDM_VIDEOSKIP5},
{'rec1', IDM_VIDEORES1},
{'rec2', IDM_VIDEORES2},
{'rec3', IDM_VIDEORES3},
{'rec4', ID}, //--unused
{'mbof', ID},
{'mb2f', ID},
{'mb4f', ID},
{'mb8f', ID},
{'ejd0', IDM_EJECTDISC0},
{'ejd1', IDM_EJECTDISC1},
{'snev', ID},
{'txte', ID_TELETEXT},
{'txtd', ID_TELETEXTFILES},
{'txts', ID_TELETEXTLOCALHOST},
{'txtc', ID_TELETEXTCUSTOM},
{'flpe',ID_FLOPPYDRIVE},
{'hdsc', ID_HARDDRIVE},
{'hdde', ID_IDEDRIVE},
{'hdre', IDM_SELECT_HARD_DRIVE_FOLDER},
{'rs42', ID},
{'sdts', ID},
{'sdep', ID},
{'sdsp', ID},
{'page', ID}, // page setup
{'prns', ID}, // print window
{'cpyc', ID}, // copy window to clipboard
{'invb', ID}, // invertbackground
{'swtd', IDM_CAPTURESCREEN},
{'mbcn', ID_8271},
{'mbca', ID_FDC_ACORN}, //Acorn 1770 Controller
{'mbco', ID_FDC_OPUS}, //OPUS 1770 Controller - on DLL on windows
{'mbcw', ID_FDC_WATFORD}, //Watford 1770 - on DLL on windows
    {'mrtc', ID_UPRM},
    {'mrty', ID_RTCY2KADJUST},
    {'jstk', IDM_JOYSTICK},
    {'hcur', IDM_HIDECURSOR},
    {'kf10', IDM_MAPFUNCS},
    {'kdis', IDM_DISABLEKEYSALL},
    {'sram', IDM_SWRAMBOARD},
    {'erom', IDM_ROMCONFIG},
};

std::map<int,int>
invertMap(std::map<int,int> const& myMap)
{
    std::map<int,int> other;
  
    // Traverse the map and
    // pushback the values -> keys
    for (auto const& pair : myMap) {
        other[pair.second] = pair.first;
    }
    return other;
}

std::map<int,int> RC2ID = invertMap(ID2RC);

