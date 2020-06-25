/* !770 FDC Support for Beebem */
/* Written by Richard Gellman */

#ifndef DISC1770_HEADER
#define DISC1770_HEADER
extern unsigned char DWriteable[2]; // Write Protect
extern unsigned char Disc1770Enabled;
unsigned char Read1770Register(unsigned char Register1770);
void Write1770Register(unsigned char Register1770, unsigned char Value);
void Load1770DiscImage(char *DscFileName,int DscDrive,unsigned char DscType);
void WriteFDCControlReg(unsigned char Value);
unsigned char ReadFDCControlReg(void);
void Reset1770(void);
void Poll1770(int NCycles);
void CreateADFSImage(char *ImageName,unsigned char Drive,unsigned char Tracks);
void Close1770Disc(char Drive);
void Save1770UEF(FILE *SUEF);
void Load1770UEF(FILE *SUEF, int Version);
void Get1770DiscInfo(int DscDrive, int *Type, char *pFileName);
extern bool InvertTR00;
#endif
