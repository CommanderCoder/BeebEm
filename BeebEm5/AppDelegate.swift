//
//  AppDelegate.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {



    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Insert code here to initialize your application
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true;
    }

    func conv(_ str: String) -> UInt32
    {
        var  v: UInt32 = 0
        let  i = str.count-1
        for (index, element) in str.unicodeScalars.enumerated()
        {
            v += element.value << ((i-index)*8)
        }
        return v
    }

    @IBAction func HandleCommand (_ sender: NSMenuItem) {
        
        let cmd: String = sender.identifier?.rawValue ?? "none" //"opn0"
        beeb_HandleCommand(conv(cmd))
    }
    
}


// allow access to this in C
@_cdecl("swift_GetOneFileWithPreview")
public func swift_GetOneFileWithPreview( ) -> Int
{
    let dialog = NSOpenPanel()

    dialog.title                   = "Choose a file | Our Code World"
    dialog.showsResizeIndicator    = true
    dialog.showsHiddenFiles        = false
    dialog.allowsMultipleSelection = false
    dialog.canChooseDirectories    = false
    dialog.allowedFileTypes        = ["ssd", "dsd", "wdd", "dos", "adl", "adf", "img"];

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


// allow access to this in C
@_cdecl("swift_SaveFile")
public func swift_SaveFile()
{
    let path = "a selected file."
    print("Save File to \(path)")
}
