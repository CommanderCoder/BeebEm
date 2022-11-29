#  Todo

* breakout board - 
* check User Def keyboard
* cassette/HD leds - DONE
* disable keys - DONE for BREAK and ESC
* SPROW second processor -  added MODET, MODE32 and BEEBEM to the preprocessor; don't build armmem.cpp & armdis.cpp; because MENU handler and CPU update are on different threads, need to make sure to block CPU update until MENU has finished.
* joystick/mousestick
* debugger
* preference options
* RTC module - done
* Econet - done (not tested)
* Teletext - done (I don't think this has been tested in a while)
* Sound volume
* text to speech : use AVSpeechSynthesizer 
* music 5000 http://8bs.com/emum5000.htm#m500 - working but needs massive buffers.  need to figure out why the timing is off.
* motion blur - done
* printer
* rs423
* IDE HDD - not tested? https://acorn.huininga.nl/pub/software/BeebEm/BeebEm-4.14.68000-20160619/Help/harddisks.html
* import/export files disc- import done : export requires a dialogue box
* video capture - just quicktime
* image capture - just quicktime
* s/w ram board - "edit rom" dialogue required
* copy/paste - done
* textview - part of beebspeech
