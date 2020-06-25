/* Teletext Support for Beebem */

extern char TeleTextAdapterEnabled;
extern char TeleTextData;
extern char TeleTextServer;

void TeleTextWrite(int Address, int Value);
int TeleTextRead(int Address);
void TeleTextPoll(void);
void TeleTextLog(char *text, ...);
void TeleTextInit(void);
