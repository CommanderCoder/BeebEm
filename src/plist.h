CFDictionaryRef CreateMyDictionary( void );
CFPropertyListRef CreateMyPropertyListFromFile( CFURLRef fileURL );
void WriteMyPropertyListToFile( CFPropertyListRef propertyList, CFURLRef fileURL );
void AddDictNum(CFMutableDictionaryRef dict, CFStringRef key, int value);
void AddDictString(CFMutableDictionaryRef dict, CFStringRef key, char *value);
int GetDictNum(CFMutableDictionaryRef dict, CFStringRef key, int Default);
void GetDictString(CFMutableDictionaryRef dict, CFStringRef key, char *value, char *Default);
