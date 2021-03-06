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

    
    override init() {
        super.init()
        // Add your subclass-specific initialization here.
    }

    override class var autosavesInPlace: Bool {
        return true
    }

    override func makeWindowControllers() {
        let parent = NSDocumentController.shared
        let documentCount = parent.documents.count
        Swift.print(documentCount)
        // only add one window controller
        if documentCount > 1
        {
            // close old document before starting the new one
            parent.documents[0].close()
        }
        
        // Returns the Storyboard that contains your Document window.
        let storyboard = NSStoryboard(name: NSStoryboard.Name("Main"), bundle: nil)
        let windowController = storyboard.instantiateController(withIdentifier: NSStoryboard.SceneIdentifier("Document Window Controller")) as! NSWindowController
        self.addWindowController(windowController)
        
//        windowController.contentViewController?.representedObject = content
    }

    override func data(ofType typeName: String) throws -> Data {
        // Insert code here to write your document to data of the specified type, throwing an error in case of failure.
        // Alternatively, you could remove this method and override fileWrapper(ofType:), write(to:ofType:), or write(to:ofType:for:originalContentsURL:) instead.
  
        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }

    override func read(from url: URL, ofType typeName: String) throws {
        // SEE AEodoc()
        // autorun with the filename given

        Swift.print("Autoran with "+url.path)

        let path = UnsafeMutablePointer<Int8>(mutating: (url.path as NSString).utf8String)

        beeb_autorun(path)
    }

    



}

