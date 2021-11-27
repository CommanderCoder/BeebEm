//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

// BEEBEM
struct CColour{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

int beeb_main(int argc, char *argv[]);
void beeb_MainCpuLoop();
int beeb_end();
void beeb_video(long height, long width, struct CColour buffer[]);
void beeb_handlekeys(long eventkind, unsigned long keycode, char charCode);
void beeb_handlemouse(long eventkind);

int beeb_HandleCommand(unsigned int cmdID);
void beeb_setFilePath(const char* _path);

//void beeb_setAMXPosition(unsigned int x,unsigned int y);

long beeb_TCHandleCommand(unsigned int cmdID);
void beeb_TapeControlOpenDialog();
void beeb_TapeControlCloseDialog();
long beeb_getTableRowsCount(const char* tablename);
const char* beeb_getTableCellData(unsigned int property, long itemID);

int beeb_autorun(char *path);
