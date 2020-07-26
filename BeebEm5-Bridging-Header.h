//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//


// BEEBEM
struct PixelData{
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
struct CColour{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

int beeb_main(int argc, char *argv[]);
int beeb_end();
void beeb_MainCpuLoop();
void beeb_video(int argc, struct PixelData buffer[]);
void beeb_video2(long height, long width, struct CColour buffer[]);
void beeb_handlekeys(long eventkind, unsigned long keycode, char charCode);

int beeb_HandleCommand(unsigned int cmdID);
void beeb_setFilePath(const char* _path);


long beeb_TCHandleCommand(unsigned int cmdID);
void beeb_TapeControlOpenDialog();
void beeb_TapeControlCloseDialog();
long beeb_getTableRowsCount(const char* tablename);
const char* beeb_getTableCellData(unsigned int property, long itemID);

