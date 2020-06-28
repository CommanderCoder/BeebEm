//
//  ViewBeeb.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa

@IBDesignable
class ViewBeeb: NSView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
        drawScreen(dirtyRect)
    }
    
    
    
    
    // allow key detection
    override var acceptsFirstResponder: Bool { return true }
        
    override func keyDown(with event: NSEvent) {
     print("Keydown "+(event.characters ?? "nil"))
        start_audio()
    }
    
    override func keyUp(with event: NSEvent) {
     print("Keyup "+(event.characters ?? "nil"))
        stop_audio()
    }
    
    override func flagsChanged(with event: NSEvent) {
        print("flagsChanged  \(event.keyCode)" )
    }

    
    
    
    
    
    
    
    //--- test code
    var beebAudio = BeebAudio()
    // pixeldata
    var pixelData : [PixelData]

    
    required init?(coder: NSCoder) {
        length = 200
        range = length
        pixelData = [PixelData](repeating: redPixel, count: length * length)

        super.init(coder: coder)
        print("started 1")
        
        timer = Timer.scheduledTimer(timeInterval: 1.0/Double(fpsSpeed), target: self, selector: #selector(tick), userInfo: nil, repeats: true)

        init_audio()
        init_cpu()
        
    }

     func drawScreen(_ dirtyRect: NSRect) {
        
        // Drawing code here.
        let context = NSGraphicsContext.current?.cgContext

        // Update pixels
        let image = imageFromARGB32Bitmap(pixels: pixelData, width: length, height: length)
        // image is a CGimage
        
        context?.draw(image, in: dirtyRect)
    }
    
    var lasttime = 0.0
    
    // to use as a selector need @objc
    @objc func tick() {
        setNeedsDisplay(self.visibleRect)

//        let ct = CACurrentMediaTime()//.truncatingRemainder(dividingBy: 1)
//        print(ct-lasttime)
//        lasttime=ct
        
        exec_cpu()
        
        update_video()


    }
        
    private var timer: Timer!
    private let fpsSpeed=120

    private var x = 0
    private let range : Int
    
    let length : Int
    let redPixel = PixelData(a: 255, r: 192, g: 0, b: 0)
    let purplePixel = PixelData(a: 255, r: 192, g: 0, b: 255)

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
        beebAudio.init_audio()
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
        
        let s0 = strdup("alpha")
        let s1 = strdup("beta")
        let s2 = strdup("gama")
        let s3 = strdup("delta")


        var arr = [s0,s1,s2,s3]
        let ac = Int32(arr.count)


        // pass the 'arr' into the closure as the variable p as unsafe mutable bytes - this is
        // so that beeb_main can modify them if necessary
        arr.withUnsafeMutableBytes { (p) -> () in
            // find the base address of the UnsafeMutableRawBufferPointer (it is a UnsafeMutableRawPointer)
            // get the typed pointer to the base address assuming it is already bound to a type
            // in this case it will make pp : UnsafeMutablePointer<Int8>
            // self is the arr instance
            let pp = p.baseAddress?.assumingMemoryBound(to: UnsafeMutablePointer<Int8>?.self)
            beeb_main(ac,pp)
        }

    }
    func exec_cpu()
    {
        
        Exec6502Instruction()

    }
    
    func update_video()
    {
        let ac = Int32(pixelData.count)

        pixelData.withUnsafeMutableBytes{(p)->() in
            let pp = p.baseAddress?.assumingMemoryBound(to: PixelData.self)
            beeb_video(ac,pp)
        }
    }
}
