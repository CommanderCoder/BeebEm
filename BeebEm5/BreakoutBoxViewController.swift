//
//  BreakoutBoxViewController.swift
//  BeebEm5
//
//  Created by Commander Coder on 20/10/2022.
//  Copyright © 2022 Andrew Hague. All rights reserved.
//

import Cocoa
import Carbon

class BreakoutBoxViewController: NSViewController {

    
    // allow key detection
    override var acceptsFirstResponder: Bool { return true }

    override func keyDown(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
        
        print("keydown \(cs)")
        beeb_bbhandlekeys(kEventRawKeyDown, UInt32(event.keyCode), cs)
    }
    
    override func keyUp(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
        
        print("keyup \(cs)")
        beeb_bbhandlekeys(kEventRawKeyUp, UInt32(event.keyCode), cs)
    }

    override func viewDidAppear() {
        beeb_BreakoutBoxOpenDialog()
    }
    
    override func viewDidDisappear() {
        beeb_BreakoutBoxCloseDialog()
    }
    
}
