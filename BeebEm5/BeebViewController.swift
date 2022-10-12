//
//  BeebViewController.swift
//  BeebEm5
//
//  Created by Commander Coder on 25/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa
import Carbon
import SpriteKit

//10 T% = TIME
//20 REPEAT
//30 REPEAT UNTIL TIME > T%
//45 T%=T%+100
//40 PRINT TIME
//50 UNTIL FALSE

// Alternative method in VirtualC64 - a thread is created that calls the cpu_update very quickly
// and in the cpu_update it WAITS
//


// MODIFIED FOR MacOS FROM
// https://github.com/nicklockwood/RetroRampage
// AND
// https://stackoverflow.com/questions/25981553/cvdisplaylink-with-swift

weak var beebViewControllerInstance : BeebViewController?
class BeebViewController: NSViewController {

    @IBOutlet weak var spriteView: SKView!
    var skimage: SKSpriteNode = SKSpriteNode()
    
    var displayLink : CVDisplayLink?
    
    var screenFilename : String?
//    var timer: Timer = Timer()

    var BeebReady : Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
                
        beebViewControllerInstance = self
        
        // Do view setup here.
        
        // two options NSTimer or CVDisplayLink

        // create a callback for the DisplayLink to the update() method of this class
        let displayLinkOutputCallback: CVDisplayLinkOutputCallback = {
            (displayLink: CVDisplayLink,
             inNow: UnsafePointer<CVTimeStamp>,
             inOutputTime: UnsafePointer<CVTimeStamp>,
             flagsIn: CVOptionFlags,
             flagsOut: UnsafeMutablePointer<CVOptionFlags>,
             displayLinkContext: UnsafeMutableRawPointer?) -> CVReturn in

            // need to typecast the context from pointer to self.
            let view = unsafeBitCast(displayLinkContext, to: BeebViewController.self)

            // Make an asynchronous call to the cpu update
            DispatchQueue.main.async {              view.update()            }

            return kCVReturnSuccess
        }

        CVDisplayLinkCreateWithActiveCGDisplays(&displayLink)
        // need to convert 'self' to unsafe pointer
        CVDisplayLinkSetOutputCallback(displayLink!,
                                       displayLinkOutputCallback,
                                       UnsafeMutableRawPointer(
                                        Unmanaged.passUnretained(self).toOpaque()))
        CVDisplayLinkStart(displayLink!)

        
        
        
        if let sview = spriteView {
            // scene is not optional
            let scene = SKScene(size: sview.bounds.size)
            scene.anchorPoint = CGPoint(x: 0.5, y: 0.5)

            // load an image to new node
            skimage = SKSpriteNode(texture: SKTexture(), size: sview.bounds.size)
            scene.addChild(skimage)

            // Present the scene
            sview.presentScene(scene)
        }
        
        BeebReady = setupBeeb()
        
        // two options NSTimer or CVDisplayLink
//        timer = Timer.scheduledTimer(withTimeInterval: 1.0/100000, repeats: true) { timer in
//            // update the CPU
//            self.update_cpu()
//        }

        // avoid problems with heavy UIs
//        RunLoop.current.add(timer, forMode: .common)

        // run the CPU update as fast as possible
        // it will be put to sleep internally.
        update_cpu()
    }

    
    func update() {
        if (!BeebReady)
        {
            return
        }
        // draw to the renderer bitmap and then put that image
        // onto the imageview
        var renderer = Renderer(width: 640, height: 512)
        renderer.draw()
        
        guard let bmImage = NSImage(bitmap: renderer.bitmap) else { return  }
        // save screen if filename is set
        if (screenFilename != nil)
        {
            savePNG(image: bmImage, filepath: screenFilename ?? "beebem.png");
            screenFilename = nil;
        }
        
        skimage.texture = SKTexture(image: bmImage)

        // update the tape controller
        tcViewControllerInstance?.update()

        update_hardware_bridge()

        // update the window label
        NSApplication.shared.mainWindow?.title = CBridge.windowTitle
    }

    func savePNG(image: NSImage, filepath:String) {
        let patharray = FileManager.default.urls(for: .picturesDirectory,
                                            in: .userDomainMask)
        let currentTimeStamp = String(Int(NSDate().timeIntervalSince1970))
        let path = patharray[0].appendingPathComponent(currentTimeStamp+filepath)

        let imageRep = NSBitmapImageRep(data: image.tiffRepresentation!)
        let pngData = imageRep?.representation(using: .png, properties: [:])
        do {
            try pngData!.write(to: path)
        } catch {
            print("Error info: \(error)")
        }
        
    }


    deinit {
        //  Stop the display link.  A better place to stop the link is in
        //  the viewController or windowController within functions such as
        //  windowWillClose(_:)

        CVDisplayLinkStop(displayLink!)

//        timer.invalidate()
        
        end_cpu()
    }


    private var width : Int = 0
    private var height : Int = 0
    
    @IBOutlet weak var WIPlabel: NSTextField!

    
    @IBOutlet weak var CassMotorLabel: NSTextField!
    @IBOutlet weak var MasterPowerOnLabel: NSTextField!
    
    @IBOutlet weak var CassMotorLED: NSImageView!
    @IBOutlet weak var CapsLED: NSImageView!
    @IBOutlet weak var ShiftLED: NSImageView!

    @IBOutlet weak var HD0LED: NSImageView!
    @IBOutlet weak var HD1LED: NSImageView!
    @IBOutlet weak var HD2LED: NSImageView!
    @IBOutlet weak var HD3LED: NSImageView!
    @IBOutlet weak var FD0LED: NSImageView!
    @IBOutlet weak var FD1LED: NSImageView!

    
 
    func setupBeeb() -> Bool
    {
        width = 640
        height = 512
        
        //init_audio()
        return init_cpu()
    }

}



// - CPU part of the controller
extension BeebViewController{
    
    func init_cpu() -> Bool
    {

        
//    https://stackoverflow.com/questions/63257460/swift-c-interop-passing-swift-argv-to-c-argv
        
        let ret = CommandLine.unsafeArgv.withMemoryRebound(
            to: UnsafeMutablePointer<Int8>?.self,
            capacity: Int(CommandLine.argc) )
                { ptr in beeb_main( Int(CommandLine.argc), ptr) }
        return (ret == 0)
        
    }
    
    func update_cpu()  // game update
    {
        let d = DispatchQueue(label:"CPU")
        // GAME UPDATE is called from the DisplayLinkOutputCallback
        // and it is called on the main thread
        d.async {
            while true {
                // update the CPU - not now - but after the delay requested by the previous cpu cycle
                Thread.sleep(forTimeInterval: Double(CBridge.nextCPU)/1000.0);
                CBridge.nextCPU = 0
                beeb_MainCpuLoop()

//                print("> "+String(Date.timeIntervalSinceReferenceDate))
            }
        }
    }

    func update_hardware_bridge()
    {
        WIPlabel?.stringValue = CBridge.windowTitle

        if #available(OSX 10.14, *) {
            CapsLED?.contentTintColor = CBridge.leds.contains(.CapsLED) ? NSColor.red: NSColor.darkGray
            ShiftLED?.contentTintColor = CBridge.leds.contains(.ShiftLED) ? NSColor.red: NSColor.darkGray
            CassMotorLED?.contentTintColor = CBridge.leds.contains(.CassLED) ? NSColor.red: NSColor.darkGray

            HD0LED?.contentTintColor = CBridge.leds.contains(.HD0LED) ? NSColor.yellow: NSColor.darkGray
            HD1LED?.contentTintColor = CBridge.leds.contains(.HD1LED) ? NSColor.yellow: NSColor.darkGray
            HD2LED?.contentTintColor = CBridge.leds.contains(.HD2LED) ? NSColor.yellow: NSColor.darkGray
            HD3LED?.contentTintColor = CBridge.leds.contains(.HD3LED) ? NSColor.yellow: NSColor.darkGray
            FD0LED?.contentTintColor = CBridge.leds.contains(.FD0LED) ? NSColor.yellow: NSColor.darkGray
            FD1LED?.contentTintColor = CBridge.leds.contains(.FD1LED) ? NSColor.yellow: NSColor.darkGray

            CassMotorLabel.isHidden = (CBridge.machineType == 3) // BBC Master
            MasterPowerOnLabel.isHidden = !CassMotorLabel.isHidden // opposite of CassMotorLabel
            
        } else {
            // cannot COLOUR TINT on earlier versions
            // Fallback on earlier versions
        }
    }
    
    func end_cpu()
    {
        beeb_end()
    }
 
    
}
