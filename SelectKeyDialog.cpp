/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 2020  Chris Needham

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with this program; if not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA  02110-1301, USA.
****************************************************************/

// User defined keyboard functionality.

#ifdef BEEBWIN
#include <windows.h>
#else
#include <Carbon/Carbon.h>
#endif
#include <string>

#include "main.h"
#include "beebemrc.h"
#include "SelectKeyDialog.h"
#include "Messages.h"

#ifdef BEEBWIN
/****************************************************************************/

static bool IsDlgItemChecked(HWND hDlg, int nIDDlgItem);

SelectKeyDialog* selectKeyDialog;

/****************************************************************************/

SelectKeyDialog::SelectKeyDialog(
	HINSTANCE hInstance,
	HWND hwndParent,
	const std::string& Title,
	const std::string& SelectedKey,
	bool EnableShift) :
	m_hInstance(hInstance),
	m_hwnd(nullptr),
	m_hwndParent(hwndParent),
	m_Title(Title),
	m_SelectedKey(SelectedKey),
	m_EnableShift(EnableShift),
	m_Key(-1),
	m_Shift(false)
{
}

/****************************************************************************/

bool SelectKeyDialog::Open()
{
	// Create modeless dialog box
	m_hwnd = CreateDialogParam(
		m_hInstance,
		MAKEINTRESOURCE(IDD_SELECT_KEY),
		m_hwndParent,
		sDlgProc,
		reinterpret_cast<LPARAM>(this)
	);

	if (m_hwnd != nullptr)
	{
		EnableWindow(m_hwndParent, FALSE);

		return true;
	}

	return false;
}

/****************************************************************************/

void SelectKeyDialog::Close(UINT nResultID)
{
	EnableWindow(m_hwndParent, TRUE);
	DestroyWindow(m_hwnd);
	m_hwnd = nullptr;

	PostMessage(m_hwndParent, WM_SELECT_KEY_DIALOG_CLOSED, nResultID, 0);
}

/****************************************************************************/

INT_PTR SelectKeyDialog::DlgProc(
	HWND   hwnd,
	UINT   nMessage,
	WPARAM wParam,
	LPARAM /* lParam */)
{
	switch (nMessage)
	{
	case WM_INITDIALOG:
		m_hwnd = hwnd;

		SetWindowText(m_hwnd, m_Title.c_str());

		SetDlgItemText(m_hwnd, IDC_ASSIGNED_KEYS, m_SelectedKey.c_str());

		if (!m_EnableShift)
		{
			ShowWindow(GetDlgItem(m_hwnd, IDC_SHIFT), SW_HIDE);
		}
		return TRUE;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			hCurrentDialog = nullptr;
		}
		else
		{
			hCurrentDialog = m_hwnd;
			hCurrentAccelTable = nullptr;
		}
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			Close(IDCANCEL);
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			Close(IDCONTINUE);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

/****************************************************************************/

INT_PTR CALLBACK SelectKeyDialog::sDlgProc(
	HWND   hwnd,
	UINT   nMessage,
	WPARAM wParam,
	LPARAM lParam)
{
	SelectKeyDialog* dialog;

	if (nMessage == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwnd, DWLP_USER, lParam);
		dialog = reinterpret_cast<SelectKeyDialog*>(lParam);
	}
	else
	{
		dialog = reinterpret_cast<SelectKeyDialog*>(
			GetWindowLongPtr(hwnd, DWLP_USER)
		);
	}

	return dialog->DlgProc(hwnd, nMessage, wParam, lParam);
}

/****************************************************************************/

bool SelectKeyDialog::HandleMessage(const MSG& msg)
{
	if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
	{
		m_Key = (int)msg.wParam;
		m_Shift = IsDlgItemChecked(m_hwnd, IDC_SHIFT);

		Close(IDOK);
		return true;
	}

	return false;
}

#endif

/****************************************************************************/

int SelectKeyDialog::Key() const
{
	return m_Key;
}

/****************************************************************************/


#ifdef BEEBWIN
static bool IsDlgItemChecked(HWND hDlg, int nIDDlgItem)
{
	return SendDlgItemMessage(hDlg, nIDDlgItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
#endif

/****************************************************************************/

LPCSTR SelectKeyDialog::KeyName(int Key)
{
	static CHAR Character[2]; // Used to return single characters.

	switch (Key)
	{
#ifdef BEEBWIN
	case   8: return "Backspace";
	case   9: return "Tab";
	case  13: return "Enter";
	case  16: return "Shift";
	case  17: return "Ctrl";
	case  18: return "Alt";
	case  19: return "Break";
	case  20: return "Caps";
	case  27: return "Esc";
	case  32: return "Spacebar";
	case  33: return "PgUp";
	case  34: return "PgDn";
	case  35: return "End";
	case  36: return "Home";
	case  37: return "Left";
	case  38: return "Up";
	case  39: return "Right";
	case  40: return "Down";
	case  45: return "Insert";
	case  46: return "Del";
	case  93: return "Menu";
	case  96: return "Pad0";
	case  97: return "Pad1";
	case  98: return "Pad2";
	case  99: return "Pad3";
	case 100: return "Pad4";
	case 101: return "Pad5";
	case 102: return "Pad6";
	case 103: return "Pad7";
	case 104: return "Pad8";
	case 105: return "Pad9";
	case 106: return "Pad*";
	case 107: return "Pad+";
	case 109: return "Pad-";
	case 110: return "Pad.";
	case 111: return "Pad/";
	case 112: return "F1";
	case 113: return "F2";
	case 114: return "F3";
	case 115: return "F4";
	case 116: return "F5";
	case 117: return "F6";
	case 118: return "F7";
	case 119: return "F8";
	case 120: return "F9";
	case 121: return "F10";
	case 122: return "F11";
	case 123: return "F12";
	case 144: return "NumLock";
	case 145: return "SclLock";
	case 186: return ";";
	case 187: return "=";
	case 188: return ",";
	case 189: return "-";
	case 190: return ".";
	case 191: return "/";
	case 192: return "\'";
	case 219: return "[";
	case 220: return "\\";
	case 221: return "]";
	case 222: return "#";
	case 223: return "`";
    default:
        Character[0] = (CHAR)LOBYTE(Key);
        Character[1] = '\0';
        return Character;

#else
        case     0    :    return    "A";
        case     1    :    return    "S";
        case     2    :    return    "D";
        case     3    :    return    "F";
        case     4    :    return    "H";
        case     5    :    return    "G";
        case     6    :    return    "Z";
        case     7    :    return    "X";
        case     8    :    return    "C";
        case     9    :    return    "V";
        case     10    :    return    "SectionSign";
        case     11    :    return    "B";
        case     12    :    return    "Q";
        case     13    :    return    "W";
        case     14    :    return    "E";
        case     15    :    return    "R";
        case     16    :    return    "Y";
        case     17    :    return    "T";
        case     18    :    return    "One";
        case     19    :    return    "Two";
        case     20    :    return    "Three";
        case     21    :    return    "Four";
        case     22    :    return    "Six";
        case     23    :    return    "Five";
        case     24    :    return    "Equal";
        case     25    :    return    "Nine";
        case     26    :    return    "Seven";
        case     27    :    return    "Minus";
        case     28    :    return    "Eight";
        case     29    :    return    "Zero";
        case     30    :    return    "RightBracket";
        case     31    :    return    "O";
        case     32    :    return    "U";
        case     33    :    return    "LeftBracket";
        case     34    :    return    "I";
        case     35    :    return    "P";
        case     36    :    return    "Return";
        case     37    :    return    "L";
        case     38    :    return    "J";
        case     39    :    return    "Quote";
        case     40    :    return    "K";
        case     41    :    return    "Semicolon";
        case     42    :    return    "Backslash";
        case     43    :    return    "Comma";
        case     44    :    return    "Slash";
        case     45    :    return    "N";
        case     46    :    return    "M";
        case     47    :    return    "Period";
        case     48    :    return    "Tab";
        case     49    :    return    "Space";
        case     50    :    return    "Grave";
        case     51    :    return    "Delete";
        case     52    :    return    "Linefeed";
        case     53    :    return    "Escape";
        case     55    :    return    "Command";
        case     56    :    return    "Shift";
        case     57    :    return    "CapsLock";
        case     58    :    return    "Option";
        case     59    :    return    "Control";
        case     60    :    return    "RightShift";
        case     61    :    return    "RightOption";
        case     62    :    return    "RightControl";
        case     63    :    return    "Function";
        case     64    :    return    "F17";
        case     65    :    return    "KeypadDecimal";
        case     67    :    return    "KeypadMultiply";
        case     69    :    return    "KeypadPlus";
        case     71    :    return    "KeypadClear";
        case     72    :    return    "VolumeUp";
        case     73    :    return    "VolumeDown";
        case     74    :    return    "Mute";
        case     75    :    return    "KeypadDivide";
        case     76    :    return    "KeypadEnter";
        case     78    :    return    "KeypadMinus";
        case     79    :    return    "F18";
        case     80    :    return    "F19";
        case     81    :    return    "KeypadEquals";
        case     82    :    return    "Keypad0";
        case     83    :    return    "Keypad1";
        case     84    :    return    "Keypad2";
        case     85    :    return    "Keypad3";
        case     86    :    return    "Keypad4";
        case     87    :    return    "Keypad5";
        case     88    :    return    "Keypad6";
        case     89    :    return    "Keypad7";
        case     90    :    return    "F20";
        case     91    :    return    "Keypad8";
        case     92    :    return    "Keypad9";
        case     96    :    return    "F5";
        case     97    :    return    "F6";
        case     98    :    return    "F7";
        case     99    :    return    "F3";
        case     100    :    return    "F8";
        case     101    :    return    "F9";
        case     103    :    return    "F11";
        case     105    :    return    "F13";
        case     106    :    return    "F16";
        case     107    :    return    "F14";
        case     109    :    return    "F10";
        case     111    :    return    "F12";
        case     113    :    return    "F15";
        case     114    :    return    "Help/Insert";
        case     115    :    return    "Home";
        case     116    :    return    "PageUp";
        case     117    :    return    "ForwardDelete";
        case     118    :    return    "F4";
        case     119    :    return    "End";
        case     120    :    return    "F2";
        case     121    :    return    "PageDown";
        case     122    :    return    "F1";
        case     123    :    return    "LeftArrow";
        case     124    :    return    "RightArrow";
        case     125    :    return    "DownArrow";
        case     126    :    return    "UpArrow";
        default:
            return "UNFOUND";
#endif
	}
}
