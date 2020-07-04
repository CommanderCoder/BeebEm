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

int beeb_main(int argc, char *argv[]);
int beeb_end();
void Exec6502Instruction();
void beeb_video(int argc, struct PixelData buffer[]);
void beeb_handlekeys(long eventkind, unsigned short keycode, char charCode);

int beeb_HandleCommand(unsigned int cmdID);
void beeb_setFilePath(const char* _path);
