//
//  BeebEm5-Bridging-Code.swift
//  BeebEm5
//
//  Created by Commander Coder on 06/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Foundation
import Cocoa
import AVFoundation


@objc enum FileFilter :Int {
    case DISC
    case UEF
    case IFD
    case KEYBOARD
}


@objc public enum KB_LEDs : UInt16 {
    case CASS = 0
    case CAPS
    case SHIFT
    case HD0
    case HD1
    case HD2
    case HD3
    case FD0
    case FD1
}

//option set (bit flags)
struct LEDFlags: OptionSet
{
    let rawValue: UInt16

    static let CassLED  = LEDFlags(rawValue: 1 << KB_LEDs.CASS.rawValue)
    static let CapsLED  = LEDFlags(rawValue: 1 << KB_LEDs.CAPS.rawValue)
    static let ShiftLED = LEDFlags(rawValue: 1 << KB_LEDs.SHIFT.rawValue)
    static let HD0LED  = LEDFlags(rawValue: 1 << KB_LEDs.HD0.rawValue)
    static let HD1LED  = LEDFlags(rawValue: 1 << KB_LEDs.HD1.rawValue)
    static let HD2LED  = LEDFlags(rawValue: 1 << KB_LEDs.HD2.rawValue)
    static let HD3LED  = LEDFlags(rawValue: 1 << KB_LEDs.HD3.rawValue)
    static let FD0LED  = LEDFlags(rawValue: 1 << KB_LEDs.FD0.rawValue)
    static let FD1LED  = LEDFlags(rawValue: 1 << KB_LEDs.FD1.rawValue)
}

enum CBridge {
    static var windowTitle = "-"
    static var nextCPU = 0
    static var leds: LEDFlags = []
    static var machineType = 0
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
func swift_SaveFile(filepath : UnsafeMutablePointer<CChar>, bytes: Int, fileexts: FileFilter) -> Bool
{
    let dialog = NSSavePanel()
    
    dialog.title                   = "Choose a file | BeebEm5"
    dialog.showsResizeIndicator    = true
    dialog.showsHiddenFiles        = false
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
        guard let result = dialog.url else { return false} // Pathname of the file

        let path: String = result.path
        
        // path contains the file path e.g
        // /Users/ourcodeworld/Desktop/file.txt
        
        // set the filepath back in the C code.. fill with zeros first
        filepath.assign(repeating: 0, count: bytes)
        filepath.assign(from: path, count: path.count)
            
        print("Picked \(String(cString:filepath))")
        return true
    
    }
    return false
}

// allow access to this in C
@_cdecl("swift_MoveFile")
public func swift_MoveFile(srcpath : UnsafeMutablePointer<CChar>, destpath : UnsafeMutablePointer<CChar>) -> Bool
{
    let src = String(cString: srcpath)
    let dest = String(cString: destpath)
    do
    {
        try FileManager.default.moveItem(atPath: src, toPath: dest)
    }
    catch  {
        print("Unexpected error: \(error).")
        return false
    }
    return true
}




@_cdecl("swift_SetWindowTitleWithCString")
public func swift_SetWindowTitleWithCString( title: UnsafePointer<CChar> )
{
    CBridge.windowTitle = String(cString: title)
}

@_cdecl("swift_sleepCPU")
public func swift_sleepCPU( microseconds: Int)
{
    // increment the nextCPU time
    CBridge.nextCPU += microseconds
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

extension NSView {

    func subviewsRecursive() -> [NSView] {
        return subviews + subviews.flatMap { $0.subviewsRecursive() }
    }
}



func recurse(find id: String, menu: NSMenu) -> NSMenuItem? {
  for item in menu.items {
      if item.identifier?.rawValue == id //&& item.isEnabled
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
        print("smc",cmdSTR,check)
        n.state = check ? .on : .off
    }
    else
    {
        print("smc not found: ",cmdSTR)
    }
}


// set the text on the menu with a 4 character identifier
@_cdecl("swift_ModifyMenu")
public func swift_ModifyMenu(_ cmd: UInt32, _ newitem: UInt32,  _ itemtext: UnsafePointer<CChar>)
{
    assert(cmd == newitem)

    let cmdSTR =  conv(cmd)
    var text = String(cString: itemtext)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        if (text.first == "&" )
        {
            text.removeFirst()
            n.keyEquivalent = String(text.first!)
        }
        n.title = text
    }
}

// grey the menu with a 4 character identifier
@_cdecl("swift_SetMenuEnable")
public func swift_SetMenuEnable(_ cmd: UInt32, _ enable: Bool)
{
    // There is a checkbox in the Menu's Inspector called "Auto Enables Items" that was overriding my code.
    let cmdSTR =  conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        print("sme",cmdSTR,enable)
        n.isEnabled = enable
    }
    else
    {
        print("sme not found: ",cmdSTR)
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


@_cdecl("swift_menuSetControlValue")
public func swift_menuSetControlValue(_ cmd: UInt32, _ state: Int)
{
    let cmdSTR = conv(cmd)
    if let n = menuItemByIdentifier(id:cmdSTR)
    {
        print("scv",cmdSTR)
        n.state = (state==1) ? .on : .off
    }
}

@_cdecl("swift_menuGetControlValue")
public func swift_menuGetControlValue(_ cmd: UInt32) -> Int
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

@_cdecl("swift_menuSetControlEditText")
public func swift_menuSetControlEditText(_ cmd: UInt32, _ text: NSString)
{
    let cmdSTR =  conv(cmd)
    if let n = textFieldByIdentifier(id:cmdSTR)
    {
        n.stringValue = text as String
    }
}

@_cdecl("swift_menuGetControlEditText")
public func swift_menuGetControlEditText(_ cmd: UInt32, _ text: NSMutableString, _ length:Int)
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


@_cdecl("swift_SetLED")
public func swift_SetLED(_ led: KB_LEDs, _ value: Bool)
{
    // CASS,CAPS,SHIFT from KB_LEDs
    let ledf: LEDFlags = LEDFlags(rawValue: 1 << led.rawValue)

    if (value) {
        CBridge.leds.insert(ledf)
    } else {
        CBridge.leds.remove(ledf)
    }
}


@_cdecl("swift_SetMachineType")
public func swift_SetMachineType(_ type: Int)
{
    CBridge.machineType = type;
}

@_cdecl("swift_uksetasstitle")
public func swift_uksetasstitle( _ text: UnsafePointer<CChar>)
{
    print("\(#function) \(text)")
    kbViewControllerInstance?.setasstitle(String(cString: text));
}



@_cdecl("swift_buttonSetControlValue")
public func swift_buttonSetControlValue(_ cmd: UInt32, _ state: Int)
{
    let cmdSTR = conv(cmd)
    
    kbViewControllerInstance?.buttonSetControlValue(cmdSTR, state);
}

@_cdecl("swift_saveScreen")
public func swift_saveScreen(_ text: UnsafePointer<CChar>)
{
    print("\(#function) \(text)")
    beebViewControllerInstance?.screenFilename = String(cString: text);
}


// allow access to this in C
@_cdecl("swift_GetResourcePath")
public func swift_GetResourcePath( _ resourcePath: UnsafeMutablePointer<CChar>, _ length:Int, _ filename: UnsafePointer<CChar>)
{
    let filenameString = String(cString: filename)
    
    if let fileurl = Bundle.main.url(forResource:filenameString,withExtension: nil)
    {
        let path: String = fileurl.path
        print("\(#function) \(path)")

        // set the filepath back in the C code - fill with zeros first
        resourcePath.assign(repeating: 0, count: length)
        resourcePath.assign(from: path, count: path.count)
    }
    
}


// allow access to this in C
@_cdecl("swift_GetBundleDirectory")
public func swift_GetBundleDirectory( _ bundlePath: UnsafeMutablePointer<CChar>, _ length:Int)
{
    let dpath : String = Bundle.main.resourcePath!+"/"
    print("\(#function) \(dpath)")
    // set the filepath back in the C code - fill with zeros first
    bundlePath.assign(repeating: 0, count: length)
    bundlePath.assign(from: dpath, count: dpath.count)
}

// allow access to this in C
@_cdecl("swift_GetApplicationSupportDirectory")
public func swift_GetApplicationSupportDirectory( _ resourcePath: UnsafeMutablePointer<CChar>, _ length:Int)
{
    
    /* discover Application Support and Resources folder
     Opening files will always start at Documents or folder
     https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/MacOSXDirectories/MacOSXDirectories.html#//apple_ref/doc/uid/TP40010672-CH10-SW1
     
     https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/AccessingFilesandDirectories/AccessingFilesandDirectories.html#//apple_ref/doc/uid/TP40010672-CH3-SW7
     */
    
    
//        let patharray = FileManager.default.urls(for: .applicationSupportDirectory,
//                                                 in: .userDomainMask)
//        let userDataDirectory = patharray[0].appendingPathComponent("UserData")
//
//        print (userDataDirectory.path)
//
//        let teletextpath = Bundle.main.url(forResource: "teletext", withExtension: "fnt")!
//        let z80path = Bundle.main.url(forResource: "Z80", withExtension: "ROM",subdirectory: "UserData/BeebFile")!
//        let rompath = Bundle.main.url(forResource: "Roms", withExtension: "cfg", subdirectory: "UserData")!
//        let allpath = Bundle.main.urls( forResourcesWithExtension: nil, subdirectory: "UserData")!
//
//        print (teletextpath)
//        print (z80path)
//        print (rompath)
//        print (allpath)

//        if let fileContents = try? String(contentsOf: rompath) {
//            // we loaded the file into a string!
//            print(fileContents)
//        }
//
//        if let fileContents = try? Data(contentsOf: z80path) {
//            // we loaded the file into a binary datablock!
//            print(fileContents.description)
//            print(fileContents.base64EncodedString(options: .lineLength64Characters))
//            print(String(decoding: fileContents, as: UTF8.self))
//        }
    
    
    
    
    do
    {
        //  Find Application Support directory
        let fileManager = FileManager.default
        let appSupportURL = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        //  Create subdirectory
        let directoryURL = appSupportURL.appendingPathComponent("BeebEm5")
        try fileManager.createDirectory (at: directoryURL, withIntermediateDirectories: true, attributes: nil)
        
        let dpath = directoryURL.path+"/"
        // set the filepath back in the C code - fill with zeros first
        resourcePath.assign(repeating: 0, count: length)
        resourcePath.assign(from: dpath, count: dpath.count)

    }
    catch
    {
        print("Couldn't find the ApplicationSupportDirectory")
    }
}


// allow access to this in C
@_cdecl("swift_CopyDirectoryRecursively")
public func swift_CopyDirectoryRecursively( _ sourcePath: UnsafePointer<CChar>, _ targetPath:UnsafePointer<CChar>) -> Bool
{
    let sourcePathStr = String(cString: sourcePath)
    let targetPathStr = String(cString: targetPath)

    print( "copy \(sourcePathStr) to \(targetPathStr)");
    
    do {
        if FileManager.default.fileExists(atPath: targetPathStr) {
            try FileManager.default.removeItem(atPath: targetPathStr)
        }
        try FileManager.default.copyItem(atPath: sourcePathStr, toPath:targetPathStr)
    } catch (let error) {
        print("Cannot copy item at \(sourcePathStr) to \(targetPathStr): \(error)")
        return false
    }
    return true
            
}

// https://gist.github.com/dimagimburg/54f24b24d08643b177804955483ac878

// the audio engine - MUST NOT DEALLOCATE SO OUTSIDE FUNC
var audioEngine = AVAudioEngine()
// the player used for scheduling from bufffer
var audioBufferPlayer = AVAudioPlayerNode()

//var beebAudioFormat = AVAudioFormat(commonFormat: AVAudioCommonFormat.pcmFormatInt32, sampleRate: 44100.0, channels: 2, interleaved: false)!
var beebAudioFormat = AVAudioFormat(commonFormat: AVAudioCommonFormat.pcmFormatFloat32, sampleRate: 44100.0, channels: 2, interleaved: false)!

//create the buffer with the correct format
var buffer = AVAudioPCMBuffer(pcmFormat: beebAudioFormat, frameCapacity: 100)!
//var buffer = AVAudioPCMBuffer(pcmFormat: audioBufferPlayer.outputFormat(forBus: 0), frameCapacity: 100)!
// mixer - to configure the output
// mixer has multiple inputs but one output - for mainmixer this goes to the audio output
var mainMixer = audioEngine.mainMixerNode
let mainMixerOutputFormat = mainMixer.outputFormat(forBus: 0)

// allow access to this in C
@_cdecl("swift_SoundInit")
public func swift_SoundInit()
{
    // audio input is from mic
    // audio player is from buffer/segments
    // audio output is to loudspeaker

    print (beebAudioFormat)
    print (audioBufferPlayer.outputFormat(forBus: 0))
    
    do {

        audioEngine.attach(audioBufferPlayer) // attach the player - which can play PCM or from files.
        audioEngine.connect(audioBufferPlayer, to: mainMixer, format: nil)  // connect player to the mixer using the player format

        // start the engine
        try audioEngine.start()
        
//        // play the buffer immediately
        audioBufferPlayer.play()

        buffer.frameLength = 100 //number of VALID samples - must be no greater than frameCapacity

//        print (int32format)
//        // schedule playing from the buffer, now, and looping, so doesn't complete
        audioBufferPlayer.scheduleBuffer(buffer, at: nil, options: .loops, completionHandler: nil)
//

     // generate sine wave - Mixer format will be created from the node it is connected to which is a player, which is playing a buffer
        // mixer is automatically connected to the output
//        let sr = Float(mainMixerOutputFormat.sampleRate)
//        for i in stride(from:0, to: Int((buffer.frameLength)), by: Int.Stride(mainMixerOutputFormat.channelCount)) {
        let sr = Float(beebAudioFormat.sampleRate)
        for i in stride(from:0, to: Int((buffer.frameLength)), by: Int.Stride(beebAudioFormat.channelCount)) {
            
            var val = sinf(441.0*Float(i)*2*Float(Double.pi)/sr)
            if beebAudioFormat.commonFormat == .pcmFormatInt32
            {
                // range is from 0 to INT32.max
                let val1 = Int32(Double(val)*Double(1<<30))
                let val2 = Float(val1)/Float(1<<30)
                buffer.int32ChannelData!.pointee[i] = val1.littleEndian
//                buffer.int32ChannelData!.pointee[i+1] = val1>>1
//                print(val,String(format:"%04X", val1),val2)
            }
            else{
                buffer.floatChannelData!.pointee[i] = (val * 0.5)
            }
         }
    }
    catch {
        print ("")
        print (error)
        print ("")
    }
    
}


// allow access to this in C
@_cdecl("swift_SoundStream")
public func swift_SoundStream( _ soundbuffer: UnsafeMutablePointer<UInt8>)
{
//    soundbuffer.
//    print(String(bytes: seq, encoding: .ascii ))
    
    // NOTES:
    
    // THIS EMULATOR IS RUNING QUICKLY
    // SOUNDS WILL BE SHORTER AND MAYBE HIGH PITCHED
    
    // BBC BASIC TEST
    // SOUND 1,-15,400,20
    
    // this comes through as a square wave on 8 bits mono

    var j = 0
    for i in stride(from:0, to: Int(buffer.frameLength), by: UInt32.Stride(beebAudioFormat.channelCount)) {
        // samples are 8 bit
        let val = soundbuffer[j]

        if beebAudioFormat.commonFormat == .pcmFormatInt32
        {
            let val1 = (Int32(val)-128)<<24
//            print(String(format:"%02X", val),val1)
            buffer.int32ChannelData!.pointee[i] = val1
        }
        else
        {
            let val1 = (Float(val)/Float(1<<9))-0.5
//            print(String(format:"%02X", val),val1)
            buffer.floatChannelData!.pointee[i] = val1
        }
        j+=1
     }

}



// allow access to this in C
@_cdecl("swift_CloseAudio")
public func swift_CloseAudio()
{
    audioBufferPlayer.stop()
    audioEngine.stop()
}



