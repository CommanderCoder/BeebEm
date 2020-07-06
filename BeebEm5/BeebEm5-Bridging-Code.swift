//
//  BeebEm5-Bridging-Code.swift
//  BeebEm5
//
//  Created by Commander Coder on 06/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Foundation
import Cocoa



@objc enum FileFilter :Int {
    case DISC
    case UEF
    case IFD
    case KEYBOARD
}


var windowTitle = "-"


// allow access to this in C
@_cdecl("swift_GetOneFileWithPreview")
func swift_GetOneFileWithPreview( _ fileexts : FileFilter) -> Int
{
    let dialog = NSOpenPanel()

    dialog.title                   = "Choose a file | BeebEm5"
    dialog.showsResizeIndicator    = true
    dialog.showsHiddenFiles        = false
    dialog.allowsMultipleSelection = false
    dialog.canChooseDirectories    = false
    switch fileexts {
    case .DISC:
        dialog.allowedFileTypes        = ["ssd", "dsd", "wdd", "dos", "adl", "adf", "img"];
    case .UEF:
        dialog.allowedFileTypes        = ["uef", "csw"];
    case .IFD:
        dialog.allowedFileTypes        = ["ssd", "dsd", "inf"];
    case .KEYBOARD:
        dialog.allowedFileTypes        = ["kmap"];
    }

    if (dialog.runModal() ==  NSApplication.ModalResponse.OK) {
        let result = dialog.url // Pathname of the file

        if (result != nil) {
            let path: String = result!.path
            
            // path contains the file path e.g
            // /Users/ourcodeworld/Desktop/file.txt
            
            // set the filepath back in the C code
            beeb_setFilePath(path)
            return 1
        }
    }
//      User clicked on "Cancel"
// or   the result URL was nil
    return 0
}

@_cdecl("swift_SaveFile")
public func swift_SaveFile(filepath : UnsafeMutablePointer<CChar>)
{
    filepath.assign(from: "Andrew", count: 6)
        
    print("Save File to \(filepath)")
}

@_cdecl("swift_SetWindowTitleWithCString")
public func swift_SetWindowTitleWithCString( title: UnsafePointer<CChar> )
{
    windowTitle = String(cString: title)
}


// convert a string of 4 characters to a UInt32
func conv(_ str: String) -> UInt32
{
    var  v: UInt32 = 0
    let  i = str.count-1
    for (index, element) in str.unicodeScalars.enumerated()
    {
        if index == 4
        {
            break
        }
        v += element.value << ((i-index)*8)
    }
    return v
}

// convert a UInt32 to a string of 4 characters
func conv(_ value: UInt32) -> String
{
    var  s: String = ""
    let  i = 4
    for index in 1...i
    {
        // if unicode not recognised - use '@'
        s.append( Character(UnicodeScalar(value >> ((i-index)*8)&0xff) ?? UnicodeScalar(64)))
    }
    return s
}

// set the tick on the menu with a 4 character identifier
@_cdecl("swift_SetMenuCheck")
public func swift_SetMenuCheck(_ cmd: UInt32, _ check: Bool)
{
    
    func itemByIdentifier(id: String) -> NSMenuItem? {

      func recurse(menu: NSMenu) -> NSMenuItem? {
        for item in menu.items {
            if item.identifier?.rawValue == id {
            return item
          } else if let submenu = item.submenu {
            if let item = recurse(menu: submenu) {
              return item
            }
          }
        }
        return nil
      }
        guard let mainmenu = NSApplication.shared.mainMenu else { return nil }

        return recurse(menu: mainmenu)

    }
    
    let cmdSTR =  conv(cmd)
    if let n = itemByIdentifier(id:cmdSTR)
    {
        n.state = check ? .on : .off
    }
}
