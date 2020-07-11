//
//  TapeControlViewController.swift
//  BeebEm5
//
//  Created by Commander Coder on 07/07/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

import Cocoa

weak var tcViewControllerInstance : TapeControlViewController?

class TapeControlViewController: NSViewController {

    @IBOutlet weak var tableView: NSTableView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do view setup here.
        
        tableView.delegate = self
        tableView.dataSource = self
        
        tcViewControllerInstance = self
    }
    
    func selectRowInTable(_ row: UInt){
        tableView.selectRowIndexes(.init(integer:Int(row-1)), byExtendingSelection: false)
    }
    
    func reloadFileList() {
        tableView.reloadData()
    }

    override func viewDidAppear() {
        beeb_TapeControlOpenDialog()
        // setAutoenablesItems must be disabled on the NSMenu containing this item in the storyboard
        
        if let appDel = NSApplication.shared.delegate as? AppDelegate {
            appDel.tapeControlMenuItem.isEnabled = false;
        }
    }

    override func viewDidDisappear() {
        beeb_TapeControlCloseDialog()
        // setAutoenablesItems must be disabled on the NSMenu containing this item in the storyboard
        if let appDel = NSApplication.shared.delegate as? AppDelegate {
            appDel.tapeControlMenuItem.isEnabled = true;
        }
    }

    @IBAction func TCHandleCommand(_ sender: NSButton) {
        let cmd: String = sender.identifier?.rawValue ?? "none"
        beeb_TCHandleCommand(conv(cmd));
    }
    
    
}

extension TapeControlViewController: NSTableViewDataSource {
  
  func numberOfRows(in tableView: NSTableView) -> Int {
    return beeb_getTableRowsCount("TapeControl")
  }

}

extension TapeControlViewController: NSTableViewDelegate {

  fileprivate enum CellIdentifiers {
    static let FilenameCell = NSUserInterfaceItemIdentifier(rawValue: "NAME")
    static let BlockCell = NSUserInterfaceItemIdentifier(rawValue: "BLCK")
    static let LengthCell = NSUserInterfaceItemIdentifier(rawValue: "LENG")
  }

// this function will create a cell - by the column (as a tablecolumn) and the row (as an integer))
  func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {

    // GET DATA FOR THE CELL AS SOME TEXT INTO
    var text: String = ""
    let cellIdentifier: NSUserInterfaceItemIdentifier = tableColumn!.identifier
    
    text = String(cString: beeb_getTableCellData(conv(cellIdentifier.rawValue), row+1))

    // 3
    if let cell = tableView.makeView(withIdentifier: cellIdentifier , owner: nil) as? NSTableCellView {
      cell.textField?.stringValue = text
      return cell
    }
    return nil
  }
    
    func tableViewSelectionDidChange(_ notification: Notification) {
        print("\(#function) \(tableView.selectedRow) \(notification.name)")
    }
    
    func tableViewSelectionIsChanging(_ notification: Notification) {
        
        print("\(#function) \(tableView.selectedRow) \(notification.name)")
    }
}

