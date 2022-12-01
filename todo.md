#  Todo

* breakout board - test 
* check User Def keyboard
* joystick/mousestick
* debugger
* preference options
* Sound volume
* rs423 - using DCB, SetCommState and WaitCommEvent stuff and hSerialPort as a serial port file handle : mac to use 'unix' 
* import/export files disc- import done : export requires a dialogue box
* s/w ram board - "edit rom" dialogue required

* IDE HDD - not tested? https://acorn.huininga.nl/pub/software/BeebEm/BeebEm-4.14.68000-20160619/Help/harddisks.html
* music 5000 http://8bs.com/emum5000.htm#m500 - working but needs massive buffers.  need to figure out why the timing is off.

* textview - part of beebspeech
* text to speech : use AVSpeechSynthesizer 


* motion blur - done
* printer - done: sets up a file and writes bytes to that file
* copy/paste - done
* fixed tape

* video capture - just use quicktime
* image capture - just use quicktime
* RTC module - done
* Econet - done (not tested)
* Teletext - done (I don't think this has been tested in a while)
* cassette/HD leds - DONE
* disable keys - DONE for BREAK and ESC
* SPROW second processor -  added MODET, MODE32 and BEEBEM to the preprocessor; don't build armmem.cpp & armdis.cpp; because MENU handler and CPU update are on different threads, need to make sure to block CPU update until MENU has finished.
