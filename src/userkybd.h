/****************************************************************************/
/*                               Beebem                                     */
/*                               ------                                     */
/* This program may be distributed freely within the following restrictions:*/
/*                                                                          */
/* 1) You may not charge for this program or for any part of it.            */
/* 2) This copyright message must be distributed with all copies.           */
/* 3) This program must be distributed complete with source code.  Binary   */
/*    only distribution is not permitted.                                   */
/* 4) The author offers no warrenties, or guarentees etc. - you use it at   */
/*    your own risk.  If it messes something up or destroys your computer   */
/*    thats YOUR problem.                                                   */
/* 5) You may use small sections of code from this program in your own      */
/*    applications - but you must acknowledge its use.  If you plan to use  */
/*    large sections then please ask the author.                            */
/*                                                                          */
/* If you do not agree with any of the above then please do not use this    */
/* program.                                                                 */
/****************************************************************************/


#ifndef USERKYBD_HEADER
#define USERKYBD_HEADER

// Public declarations.

extern int UserKeymap[256][2];
extern WindowRef mUKWindow; 

void UserKeyboardOpenDialog();
void UserKeyboardCloseDialog();

bool LoadUserKeyboard( const char *path );
void SaveUserKeyboard( char *path );

void SetRowCol( int ctrlID );
const char *KeyName( int Key );
void GetKeysUsed( char *Keys );
void SetBBCKeyForVKEY( int Key );

#endif