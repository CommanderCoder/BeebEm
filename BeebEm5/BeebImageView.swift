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
    
    var trackingArea: NSTrackingArea?
    var mouseLocation: NSPoint { NSEvent.mouseLocation }

    // MARK: - Tracking area management

    /// Will install tracking area on the view if a window is set
    override func viewDidMoveToSuperview() {
        super.viewDidMoveToSuperview()
        installTrackingArea()
    }
    
    /// Install tracking area if window is set, remove previous one if needed.
    func installTrackingArea() {
        guard let window = window else { return }
        window.acceptsMouseMovedEvents = true
        if trackingArea != nil { removeTrackingArea(trackingArea!) }
//        let trackingOptions = [.activeAlways, .mouseEnteredAndExited, .mouseMoved]
        trackingArea = NSTrackingArea(rect: frame,
                                      options: [.activeAlways, .mouseEnteredAndExited, .mouseMoved],
                                      owner: self, userInfo: nil)
        self.addTrackingArea(trackingArea!)
    }
    
    /// Called when layout is modified
    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        installTrackingArea()
    }
    
    override func mouseEntered(with event: NSEvent) {
        updateTrackingAreas()
    }
    
    override func mouseExited(with event: NSEvent) {
        window!.acceptsMouseMovedEvents = false
        removeTrackingArea(trackingArea!)

    }
    
    override func mouseMoved(with event: NSEvent) {
        // Mouse Stuff Here
        // TODO: Fix this
        var x: UInt32 = 0;
        var y: UInt32 = 0;
        if ((self.mouseLocation.x) >= 0) {
            x = UInt32(self.mouseLocation.x)
        }
        if ((self.mouseLocation.y) >= 0) {
            y = UInt32(self.mouseLocation.y)
        }

        beeb_SetAMXPosition(x,y);
        // dont need debug stuff for the moment
        //print(String(format: "%.0f, %.0f", self.mouseLocation.x, self.mouseLocation.y))
    }
    
    override func mouseUp(with event: NSEvent) {
        beeb_handlemouse(kEventMouseUp);
    }
    
    override func mouseDown(with event: NSEvent) {
        beeb_handlemouse(kEventMouseDown);
    }
    
    override func rightMouseUp(with event: NSEvent) {
        beeb_handlemouse(99);
    }
    
    override func rightMouseDown(with event: NSEvent) {
        beeb_handlemouse(100);
    }
    
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

func sendAMXPosition( x: UInt, y: UInt)
{
}


/* NSEvent keycode values
 
 
 Zero            29
 One             18
 Two             19
 Three           20
 Four            21
 Five            23
 Six             22
 Seven           26
 Eight           28
 Nine            25
 A               0
 B               11
 C               8
 D               2
 E               14
 F               3
 G               5
 H               4
 I               34
 J               38
 K               40
 L               37
 M               46
 N               45
 O               31
 P               35
 Q               12
 R               15
 S               1
 T               17
 U               32
 V               9
 W               13
 X               7
 Y               16
 Z               6
 SectionSign     10
 Grave           50
 Minus           27
 Equal           24
 LeftBracket     33
 RightBracket    30
 Semicolon       41
 Quote           39
 Comma           43
 Period          47
 Slash           44
 Backslash       42
 Keypad0 0       82
 Keypad1 1       83
 Keypad2 2       84
 Keypad3 3       85
 Keypad4 4       86
 Keypad5 5       87
 Keypad6 6       88
 Keypad7 7       89
 Keypad8 8       91
 Keypad9 9       92
 KeypadDecimal   65
 KeypadMultiply  67
 KeypadPlus      69
 KeypadDivide    75
 KeypadMinus     78
 KeypadEquals    81
 KeypadClear     71
 KeypadEnter     76
 Space           49
 Return          36
 Tab             48
 Delete          51
 ForwardDelete   117
 Linefeed        52
 Escape          53
 Command         55
 Shift           56
 CapsLock        57
 Option          58
 Control         59
 RightShift      60
 RightOption     61
 RightControl    62
 Function        63
 F1              122
 F2              120
 F3              99
 F4              118
 F5              96
 F6              97
 F7              98
 F8              100
 F9              101
 F10             109
 F11             103
 F12             111
 F13             105
 F14             107
 F15             113
 F16             106
 F17             64
 F18             79
 F19             80
 F20             90
 VolumeUp        72
 VolumeDown      73
 Mute            74
 Help/Insert     114
 Home            115
 End             119
 PageUp          116
 PageDown        121
 LeftArrow       123
 RightArrow      124
 DownArrow       125
 UpArrow         126
 
 
 */
