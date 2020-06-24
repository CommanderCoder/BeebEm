//
//  BeebAudio.swift
//  EggTimer
//
//  Created by Commander Coder on 20/06/2020.
//  Copyright © 2020 Commander Coder. All rights reserved.
//

import Cocoa
import AVFoundation

class BeebAudio: NSObject {
    let engine = AVAudioEngine()
    
    func init_audio()
    {
        let square = { (phase: Float) -> Float in
            if phase <= Float.pi {
                return 1.0
            } else {
                return -1.0
            }
        }
        let signal = square
        
        let frequency : Float = 440
        let amplitude : Float = 0.5

        let twoPi = 2 * Float.pi

        let mainMixer = engine.mainMixerNode
        let output = engine.outputNode
        let outputFormat = output.inputFormat(forBus: 0)
        let sampleRate = Float(outputFormat.sampleRate)
        // Use output format for input but reduce channel count to 1
        let inputFormat = AVAudioFormat(commonFormat: outputFormat.commonFormat,
                                        sampleRate: outputFormat.sampleRate,
                                        channels: 1,
                                        interleaved: outputFormat.isInterleaved)


        var currentPhase: Float = 0
        // The interval by which we advance the phase each frame.
        let phaseIncrement = (twoPi / sampleRate) * frequency

        let srcNode = AVAudioSourceNode { _, _, frameCount, audioBufferList -> OSStatus in
            let ablPointer = UnsafeMutableAudioBufferListPointer(audioBufferList)

            for frame in 0..<Int(frameCount) {
                // Get signal value for this frame at time.
                let value = signal(currentPhase) * amplitude
                // Advance the phase for the next frame.
                currentPhase += phaseIncrement
                if currentPhase >= twoPi {
                    currentPhase -= twoPi
                }
                if currentPhase < 0.0 {
                    currentPhase += twoPi
                }
                // Set the same value on all channels (due to the inputFormat we have only 1 channel though).
                for buffer in ablPointer {
                    let buf: UnsafeMutableBufferPointer<Float> = UnsafeMutableBufferPointer(buffer)
                    buf[frame] = value
                }
            }
            return noErr
        }

        engine.attach(srcNode)

        engine.connect(srcNode, to: mainMixer, format: inputFormat)
        engine.connect(mainMixer, to: output, format: outputFormat)
        mainMixer.outputVolume = 0.5

    }
    
    func start_audio()
    {
        do
        {
            try engine.start()
        }
        catch{
            print("Could not start engine: \(error)")
        }
    }
    
    func audio_update()
    {
        if !engine.isRunning
        {
            return
        }
        
    }
    
    func stop_audio()
    {
        engine.stop()
    }
    
}
