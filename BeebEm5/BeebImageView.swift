//
//  BeebImageView.swift
//  BeebEm5
//
//  Created by Commander Coder on 25/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa
import Carbon

class BeebImageView: NSImageView {
    
    override func draw(_ dirtyRect: NSRect) {
                NSGraphicsContext.current?.imageInterpolation = .none
        
        super.draw(dirtyRect)
        
        
        // Drawing code here.
    }
    
    // allow key detection
    override var acceptsFirstResponder: Bool { return true }
    
    override func keyDown(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
        
//        print("keydown \(cs)")
        beeb_handlekeys(kEventRawKeyDown, UInt(event.keyCode), cs)
    }
    
    override func keyUp(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
//        print("keyup \(cs)")
        beeb_handlekeys(kEventRawKeyUp, UInt(event.keyCode), cs)
    }
    
    override func flagsChanged(with event: NSEvent) {
        //       print("\(String(event.modifierFlags.rawValue, radix: 16))")
        beeb_handlekeys(kEventRawKeyModifiersChanged, event.modifierFlags.rawValue, 0)
    }
    
}
