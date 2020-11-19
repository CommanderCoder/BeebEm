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

enum CBridge {
    static var windowTitle = "-"
    static var nextCPU = 0
}

// allow access to this in C
@_cdecl("swift_GetOneFileWithPreview")
func swift_GetOneFileWithPreview(filepath : UnsafeMutablePointer<CChar>, bytes: Int, fileexts : FileFilter) -> Int
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
            
            // set the filepath back in the C code - fill with zeros first
            filepath.assign(repeating: 0, count: bytes)
            filepath.assign(from: path, count: path.count)

            return 0 // no issues
        }
    }
//      User clicked on "Cancel"
// or   the result URL was nil
    return 1 // error
}

@_cdecl("swift_SaveFile")
public func swift_SaveFile(filepath : UnsafeMutablePointer<CChar>, bytes: Int)
{
    let dialog = NSSavePanel()

    dialog.title                   = "Choose a file | BeebEm5"
    dialog.showsResizeIndicator    = true
    dialog.showsHiddenFiles        = false

    if (dialog.runModal() ==  NSApplication.ModalResponse.OK) {
        guard let result = dialog.url else { return } // Pathname of the file

        let path: String = result.path
        
        // path contains the file path e.g
        // /Users/ourcodeworld/Desktop/file.txt
        
        // set the filepath back in the C code.. fill with zeros first
        filepath.assign(repeating: 0, count: bytes)
        filepath.assign(from: path, count: path.count)
            
        print("Picked \(String(cString:filepath))")
    
    }
}

@_cdecl("swift_SetWindowTitleWithCString")
public func swift_SetWindowTitleWithCString( title: UnsafePointer<CChar> )
{
    CBridge.windowTitle = String(cString: title)
}

@_cdecl("swift_sleepCPU")
public func swift_sleepCPU( microseconds: Int)
{
    // increment the nextCPU time - but if this gets larger than a slow frame
    // clamp it to that
    CBridge.nextCPU = min(CBridge.nextCPU + microseconds, 1000000/25)
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

func recurse(find id: String, menu: NSMenu) -> NSMenuItem? {
  for item in menu.items {
      if item.identifier?.rawValue == id && item.isEnabled
      {
      return item
    } else if let submenu = item.submenu {
        if let item = recurse(find: id, menu: submenu) {
        return item
      }
    }
  }
  return nil
}

func textFieldByIdentifier(id: String) -> NSTextField? {
    return nil
}


func menuItemByIdentifier(id: String) -> NSMenuItem? {
    guard let mainmenu = NSApplication.shared.mainMenu else { return nil }

    return recurse(find: id, menu: mainmenu)
}

// set the tick on the menu with a 4 character identifier
@_cdecl("swift_SetMenuCheck")
public func swift_SetMenuCheck(_ cmd: UInt32, _ check: Bool)
{
    let cmdSTR =  conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
//        print("smc",cmdSTR)
        n.state = check ? .on : .off
    }
}


@_cdecl("swift_SetMenuItemTextWithCString")
public func swift_SetMenuItemTextWithCString(_ cmd: UInt32, _ text: UnsafePointer<CChar>) -> Int
{
    let cmdSTR =  conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        n.title = String(cString: text)
        return 0
    }
    return 1
}



@_cdecl("swift_UpdateItem")
public func swift_UpdateItem(_ text: UnsafePointer<CChar>, _ b:Int)
{
    print("\(#function) \(String(cString:text)) \(b)")
    tcViewControllerInstance?.reloadFileList()
}


@_cdecl("swift_SelectItem")
public func swift_SelectItem(_ text: UnsafePointer<CChar>, _ b:Int, _ c: UnsafePointer<UInt>)
{
    print("\(#function) \(String(cString:text)) \(b) ")
    tcViewControllerInstance?.selectRowInTable(UInt(b))
}

@_cdecl("swift_UEFNewFile")
public func swift_UEFNewFile(_ text: UnsafePointer<CChar>)
{
    print("\(#function) \(String(cString:text))")
    tcViewControllerInstance?.reloadFileList()
}



@_cdecl("swift_Alert")
public func swift_Alert(_ text: UnsafePointer<CChar>, _ text2: UnsafePointer<CChar>,_ hasCancel:Bool) -> Int
{
    
    let a = NSAlert()
    a.messageText = String(cString: text)
   a.informativeText = String(cString: text2)
   //   .alertFirstButtonReturn
   a.addButton(withTitle: "OK")


   //   .alertSecondButtonReturn
    if hasCancel
    {
        a.addButton(withTitle: "Cancel")
    }
    
    a.alertStyle = .warning
    let res = a.runModal()
    let val = res == .alertFirstButtonReturn ? 1:2
    print(String(cString:text)+" "+String(cString:text2)+" "+String(hasCancel))
    tcViewControllerInstance?.reloadFileList()
    return val
}


@_cdecl("swift_SetControlValue")
public func swift_SetControlValue(_ cmd: UInt32, _ state: Int)
{
    let cmdSTR = conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        print("scv",cmdSTR)
        n.state = (state==1) ? .on : .off
    }
}

@_cdecl("swift_GetControlValue")
public func swift_GetControlValue(_ cmd: UInt32) -> Int
{
    var val: Int = 0
    let cmdSTR =  conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        print("gcv",cmdSTR)
        val = (n.state == .on) ? 1 : 0
    }
    return val
}

@_cdecl("swift_SetControlEditText")
public func swift_SetControlEditText(_ cmd: UInt32, _ text: NSString)
{
    let cmdSTR =  conv(cmd)
    if let n = textFieldByIdentifier(id:cmdSTR)
    {
        n.stringValue = text as String
    }
}

@_cdecl("swift_GetControlEditText")
public func swift_GetControlEditText(_ cmd: UInt32, _ text: NSMutableString, _ length:Int)
{
    let cmdSTR =  conv(cmd)
    if let n = textFieldByIdentifier(id:cmdSTR)
    {
        // set the text back in the C code - fill with zeros first
//        text.assign(repeating: 0, count: length)
//        text.assign(from: n.stringValue, count: length)
        text.setString(n.stringValue)
    }
    
}
