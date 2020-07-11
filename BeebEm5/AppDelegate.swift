//
//  AppDelegate.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate
{
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Insert code here to initialize your application
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true;
    }
    
    @IBAction func HandleCommand (_ sender: NSMenuItem) {
        
        let cmd: String = sender.identifier?.rawValue ?? "none"
//        print(cmd)
        beeb_HandleCommand(conv(cmd))
    }
    
    @IBOutlet weak var tapeControlMenuItem: NSMenuItem!
}
