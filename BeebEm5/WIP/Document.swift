//
//  Document.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa
import Foundation

// NOTE: to get this working again remove the 'Is Initial Controller' from the view controller
// Also, add this to the Info.plist
//<key>NSDocumentClass</key>
//<string>$(PRODUCT_MODULE_NAME).Document</string>


class Document: NSDocument {

    // true : when the document read some data
    var didReadData = false

    
    override init() {
        super.init()
        // Add your subclass-specific initialization here.
    }

    override class var autosavesInPlace: Bool {
        return true
    }

    override func makeWindowControllers() {
        // Returns the Storyboard that contains your Document window.
        let storyboard = NSStoryboard(name: NSStoryboard.Name("Main"), bundle: nil)
        let windowController = storyboard.instantiateController(withIdentifier: NSStoryboard.SceneIdentifier("Document Window Controller")) as! NSWindowController
        self.addWindowController(windowController)
        
        
        
//        windowController.contentViewController?.representedObject = content
    }

    override func data(ofType typeName: String) throws -> Data {
        // Insert code here to write your document to data of the specified type, throwing an error in case of failure.
        // Alternatively, you could remove this method and override fileWrapper(ofType:), write(to:ofType:), or write(to:ofType:for:originalContentsURL:) instead.
        
        let alert = NSAlert()
               alert.messageText = "Missing image"
               alert.informativeText = "There is no image to apply the filter to."
        
        var w: NSWindow?
             if let window = NSApplication.shared.windows.first{
                 w = window
             }
             if let window = w{
                alert.beginSheetModal(for: window){ (modalResponse) in
                     if modalResponse == .alertFirstButtonReturn {
                        Swift.print("Document deleted")
                     }
                 }
             }

        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }


    override func read(from data: Data, ofType typeName: String) throws {
        // Insert code here to read your document from the given data of the specified type, throwing an error in case of failure.
        // Alternatively, you could remove this method and override read(from:ofType:) instead.
        // If you do, you should also override isEntireFileLoaded to return false if the contents are lazily loaded.

        //        try createDisk(from: data)
        
        let alert = NSAlert()
               alert.messageText = "Missing image"
               alert.informativeText = "There is no image to apply the filter to."
        
        var w: NSWindow?
             if let window = NSApplication.shared.windows.first{
                 w = window
             }
             if let window = w{
                alert.beginSheetModal(for: window){ (modalResponse) in
                     if modalResponse == .alertFirstButtonReturn {
                        Swift.print("Document deleted")
                     }
                 }
             }

        
//        if content.use(data: data) {
//            // Make didReadData flag true to indicate that data has been read from a file!
//            didReadData = true
//                return
//            }
//
        
        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }

    
    /// Creates an attachment from a URL
    func createDisk(from url: URL) throws {
    
//        track("Creating attachment from URL \(url.lastPathComponent).")

        // Try to create the attachment
        let fileWrapper = try FileWrapper.init(url: url)
        let pathExtension = url.pathExtension.uppercased()
        try createAttachment(from: fileWrapper, ofType: pathExtension)

        // Put URL in recently used URL lists
//        myAppDelegate.noteNewRecentlyUsedURL(url)
    }
    
    /// Creates an attachment from a file wrapper
    fileprivate func createAttachment(from fileWrapper: FileWrapper,
                                      ofType typeName: String) throws {
        
        guard let filename = fileWrapper.filename else {
            throw NSError(domain: "VirtualC64", code: 0, userInfo: nil)
        }
        guard let data = fileWrapper.regularFileContents else {
            throw NSError(domain: "VirtualC64", code: 0, userInfo: nil)
        }
        
        let buffer = (data as NSData).bytes
        let length = data.count
        var openAsUntitled = true
        
//        track("Read \(length) bytes from file \(filename).")
        
        switch typeName {
            
//        case "VC64":
//            // Check for outdated snapshot formats
//            if SnapshotProxy.isUnsupportedSnapshot(buffer, length: length) {
//                throw NSError.snapshotVersionError(filename: filename)
//            }
//            attachment = SnapshotProxy.make(withBuffer: buffer, length: length)
//            openAsUntitled = false
//
//        case "CRT":
//            // Check for unsupported cartridge types
//            if CRTFileProxy.isUnsupportedCRTBuffer(buffer, length: length) {
//                let type = CRTFileProxy.typeName(ofCRTBuffer: buffer, length: length)!
//                throw NSError.unsupportedCartridgeError(filename: filename, type: type)
//            }
//            attachment = CRTFileProxy.make(withBuffer: buffer, length: length)
//
//        case "TAP":
//            attachment = TAPFileProxy.make(withBuffer: buffer, length: length)
//
//        case "T64":
//            attachment = T64FileProxy.make(withBuffer: buffer, length: length)
//
//        case "PRG":
//            attachment = PRGFileProxy.make(withBuffer: buffer, length: length)
//
//        case "D64":
//            attachment = D64FileProxy.make(withBuffer: buffer, length: length)
//
//        case "P00":
//            attachment = P00FileProxy.make(withBuffer: buffer, length: length)
//
//        case "G64":
//            attachment = G64FileProxy.make(withBuffer: buffer, length: length)
//
        default:
            throw NSError(domain: "VirtualC64", code: 0, userInfo:
                            [NSLocalizedDescriptionKey: "The document \"\(filename)\" could not be opened.",
                                NSLocalizedRecoverySuggestionErrorKey: "The snapshot was created with a different version of VirtualC64."])
        }
        
//        if attachment == nil {
//            throw NSError.corruptedFileError(filename: filename)
//        }
//        if openAsUntitled {
//            fileURL = nil
//        }
//        attachment!.setPath(filename)
    }


}

