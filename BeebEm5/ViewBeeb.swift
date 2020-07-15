//
//  ViewBeeb.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa
import Carbon

@IBDesignable
class ViewBeeb: NSView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
        drawScreen(dirtyRect)

        guard let mainwindow = NSApplication.shared.mainWindow else {return}
        mainwindow.title = windowTitle;
    }
    
    // keyboard

    
    // allow key detection
    override var acceptsFirstResponder: Bool { return true }
        
    override func keyDown(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
        beeb_handlekeys(kEventRawKeyDown, UInt(event.keyCode), cs)
    }
    
    override func keyUp(with event: NSEvent) {
        let cs = event.characters?.cString(using: .ascii)?[0] ?? 64
        beeb_handlekeys(kEventRawKeyUp, UInt(event.keyCode), cs)
    }
    
    override func flagsChanged(with event: NSEvent) {
//       print("\(String(event.modifierFlags.rawValue, radix: 16))")
        beeb_handlekeys(kEventRawKeyModifiersChanged, event.modifierFlags.rawValue, 0)
    }

    
    
    
    
    
    
    
    // audio
    var beebAudio = BeebAudio()

    // video
    private let width : Int
    private let height : Int
    private var videoRefresh: Timer!
    private let fpsSpeed = 50 //Hz
    private var pixelData : [PixelData]

    required init?(coder: NSCoder) {
        width = 640
        height = 512
        pixelData = [PixelData](repeating: PixelData(a:0,r:0,g:0,b:0), count: width * height)
        
        super.init(coder: coder)

        // update the video 50 times a second
        videoRefresh = Timer.scheduledTimer(timeInterval: 1.0/Double(fpsSpeed),
                        target: self,
                        selector: #selector(videorefresh),
                        userInfo: nil,
                        repeats: true)

        init_audio()
        init_cpu()
    }
    
    deinit {
        end_cpu()
    }

    // to use as a selector need @objc
    @objc
    func videorefresh()
    {
        // every 50th of a second
        update_video()
        setNeedsDisplay(self.visibleRect)
    }
    
 
     func drawScreen(_ dirtyRect: NSRect) {
        
        // Drawing code here.
        let context = NSGraphicsContext.current?.cgContext

        // Update pixels in image with those from pixeldata
        let image = imageFromARGB32Bitmap(pixels: pixelData, width: width, height: height)
        // image is a CGimage
        
        context?.draw(image, in: dirtyRect)
        
        beeb_MainCpuLoop()

        tcViewControllerInstance?.update()
    }
        
    private let rgbColorSpace = CGColorSpaceCreateDeviceRGB()
    private let bitmapInfo:CGBitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedFirst.rawValue)

    public func imageFromARGB32Bitmap(pixels:[PixelData], width:Int, height:Int)->CGImage {
        let bitsPerComponent:Int = 8
        let bitsPerPixel:Int = 32
        
        assert(pixels.count == width * height)
        
        var data = pixels // Copy to mutable []
        let providerRef = CGDataProvider(
            data: NSData(bytes: &data, length: data.count * MemoryLayout<PixelData>.size)
            )

        let cgim = CGImage(
            width: width,
            height: height,
            bitsPerComponent: bitsPerComponent,
            bitsPerPixel: bitsPerPixel,
            bytesPerRow: width * MemoryLayout<PixelData>.size,
            space: rgbColorSpace, bitmapInfo: bitmapInfo,
            provider: providerRef!,
            decode: nil, shouldInterpolate: true, intent: .defaultIntent)
            
        return cgim!
    }
    
    
}






extension ViewBeeb {

  // MARK: - Sound

    func init_audio()
    {
        if #available(OSX 10.15, *) {
            beebAudio.init_audio()
        } else {
            // Fallback on earlier versions
        }
    }
    
    func start_audio()
    {
        beebAudio.start_audio()
    }

    func stop_audio()
    {
        beebAudio.stop_audio()
    }
}


extension ViewBeeb{
    
    func init_cpu()
    {
        var commandline : [UnsafeMutablePointer<Int8>?] = []
        
        // convert the Strings to UnsafeMutablePointer<Int8>? suitable for passing to C
        for a in 0 ..< Int(CommandLine.argc)
        {
            commandline.append(strdup(CommandLine.arguments[a]))
        }
        
        // pass the 'arr' into the closure as the variable p as unsafe mutable bytes - this is
        // so that beeb_main can modify them if necessary
        commandline.withUnsafeMutableBytes { (p) -> () in
            // find the base address of the UnsafeMutableRawBufferPointer (it is a UnsafeMutableRawPointer)
            // get the typed pointer to the base address assuming it is already bound to a type
            // in this case it will make pp : UnsafeMutablePointer<Int8>
            // self is the arr instance
            let pp = p.baseAddress?.assumingMemoryBound(to: UnsafeMutablePointer<Int8>?.self)
            beeb_main(CommandLine.argc, pp)
        }
    }
    
    func end_cpu()
    {
        beeb_end()
    }

    func update_video()
    {
        let ac = Int32(pixelData.count)
        
        // call beeb_video() with a pointer to the raw pixel data
        pixelData.withUnsafeMutableBytes{(p)->() in
            let pp = p.baseAddress?.assumingMemoryBound(to: PixelData.self)
            beeb_video(ac,pp)
        }
    }

           
}
