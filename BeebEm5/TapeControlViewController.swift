//
//  TapeControlViewController.swift
//  BeebEm5
//
//  Created by Commander Coder on 07/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa


public struct Metadata: CustomDebugStringConvertible, Equatable {

  let name: String
  let block: Int
  let length: Int

    init(name: String, block: Int, length: Int ) {
    self.name = name
    self.block = block
    self.length = length
  }

  public var debugDescription: String {
    return name + " " + "block: \(block)" + " length: \(length)"
  }

}


class TapeControlViewController: NSViewController {

    @IBOutlet weak var tableView: NSTableView!
    
    var directoryItems : [Metadata]?
    let sizeFormatter = ByteCountFormatter()

    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do view setup here.
        
        tableView.delegate = self
        tableView.dataSource = self

    }
    
    override var representedObject: Any? {
      didSet {
        reloadFileList()

      }
    }
    
    func reloadFileList() {
//      directoryItems = directory?.contentsOrderedBy(sortOrder, ascending: sortAscending)
      tableView.reloadData()
    }

}

extension TapeControlViewController: NSTableViewDataSource {
  
  func numberOfRows(in tableView: NSTableView) -> Int {
    return 5
  }

}

extension TapeControlViewController: NSTableViewDelegate {

  fileprivate enum CellIdentifiers {
    static let FilenameCell = "FilenameCellID"
    static let BlockCell = "BlockCellID"
    static let LengthCell = "LengthCellID"
  }

  func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {

    var text: String = ""
    var cellIdentifier: String = ""
    
    // 1
    let item = Metadata (name:String(row), block:row*3, length:row*5)

    // 2
    if tableColumn == tableView.tableColumns[0] {
      text = item.name
      cellIdentifier = CellIdentifiers.FilenameCell
    } else if tableColumn == tableView.tableColumns[1] {
      text =  String(item.block)
      cellIdentifier = CellIdentifiers.BlockCell
    } else if tableColumn == tableView.tableColumns[2] {
      text =  String(item.length)
      cellIdentifier = CellIdentifiers.LengthCell
    }

    // 3
    if let cell = tableView.makeView(withIdentifier:  NSUserInterfaceItemIdentifier(rawValue: cellIdentifier), owner: nil) as? NSTableCellView {
      cell.textField?.stringValue = text
      return cell
    }
    return nil
  }

}

