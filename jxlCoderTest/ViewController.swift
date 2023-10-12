//
//  ViewController.swift
//  jxlCoderTest
//
//  Created by YuanYuan Liu on 2023/10/12.
//

import Cocoa
import JxlCoder

class ViewController: NSViewController {
  
  @IBOutlet weak var btn: NSButton!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    btn.action = #selector(ViewController.handleBtnClick(_:))
  }
  
  override var representedObject: Any? {
    didSet {
      // Update the view, if already loaded.
    }
  }
  
  @objc func handleBtnClick(_ sender: NSButton) {
    let openPanel = NSOpenPanel()
    openPanel.canChooseFiles = true
    openPanel.allowsMultipleSelection = false
    openPanel.canChooseDirectories = false
    openPanel.begin { [weak self] (result) -> Void in
      if result == .OK {
        guard let url = openPanel.url, let img = NSImage(contentsOf: url) else {
          print("invalid file")
          return
        }
        
        guard let data: Data = try? JXLCoder.encode(image: img, compressionOption: .loseless, quality: 100) else {
          print("conversion failed")
          return
        }
        
        let directory = NSTemporaryDirectory()
        let fileName = NSUUID().uuidString + ".jxl"
        let destURL = NSURL.fileURL(withPathComponents: [directory, fileName])!
        try? data.write(to: destURL)
        NSWorkspace.shared.activateFileViewerSelecting([destURL])
      } else {
        openPanel.close()
      }
    }
  }
}

