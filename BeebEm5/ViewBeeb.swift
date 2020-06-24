//
//  ViewBeeb.swift
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa

import AppKit


@IBDesignable
class ViewBeeb: NSView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
        drawScreen(dirtyRect)
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    //--- test code
    
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        print("started 1")
        
        timer = Timer.scheduledTimer(timeInterval: 1.0/Double(fps), target: self, selector: #selector(tick), userInfo: nil, repeats: true)

    }
    

     func drawScreen(_ dirtyRect: NSRect) {
        
        // Drawing code here.
        let context = NSGraphicsContext.current?.cgContext


        var pixelData = [PixelData](repeating: redPixel, count: length * length)
        var cData : [UInt8] = [1,2,3]
        setPixelsInCPP(&cData)

        let X = x*length
        pixelData.replaceSubrange(X..<(X+range), with: repeatElement(purplePixel, count: range))
//        pixelData.replaceSubrange(400..<(400+range), with: repeatElement(purplePixel, count: range))
        // Update pixels
        
        let image = imageFromARGB32Bitmap(pixels: pixelData, width: length, height: length)
        // image is a CGimage
        
        context?.draw(image, in: dirtyRect)
    }
    
    var lasttime = 0.0
    
    // to use as a selector need @objc
    @objc func tick() {
        setNeedsDisplay(self.visibleRect)

        x += 1
        x %= range-2

//        let ct = CACurrentMediaTime()//.truncatingRemainder(dividingBy: 1)
//        print(ct-lasttime)
//        lasttime=ct
    }
        
    private var timer: Timer!
    private let fps=60

    private var x = 0
    private let range = 200
    
    private var a = getIntFromCPP()

    public struct PixelData {
        var a:UInt8 = 255
        var r:UInt8
        var g:UInt8
        var b:UInt8
    }
    
    let length:Int = 200
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
