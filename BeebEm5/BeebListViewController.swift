//
//  BeebListWindowController.swift
//  BeebEm5
//
//  Created by Commander Coder on 08/01/2023.
//  Copyright © 2023 Andrew Hague. All rights reserved.
//

import Foundation
import AppKit

//https://www.appcoda.com/macos-programming-tableview/

class BeebListData {
    
    var selectedFiles : [Int32] = []
    
    var rows: [ String ] = []
    
    func clear()
    {
        rows = []
    }
    
    func setrow(_ val : String)
    {
        rows.append(val)
    }
}

class BeebListViewController: NSViewController {

    static var beeblistdata = BeebListData()

    static var beeblist : BeebListViewController?
    
    var driveNumber : String?
    
    @IBOutlet weak var tableView: NSTableView!
 
    
    override func viewDidLoad() {
        
        super.viewDidLoad()
        
        BeebListViewController.beeblist = self
        tableView.allowsMultipleSelection = true

        tableView.delegate = self
        tableView.dataSource = self
        
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        
        // App Delegate will show this window
        // it will attempt to reload data
        // this will render the rows and columns (getting rows from beeblistdata rows
        // so need to fill beeblistdata with info
        // for the DRIVE selected.
        // disable to the menu item
        // for the DRIVE selected.
        
        tableView.reloadData()
    }
    
    @IBAction func exportSelected(_ sender : NSButton) {
        print("Exported \(sender)")
        
        BeebListViewController.beeblistdata.selectedFiles.removeAll()
        for r in tableView.selectedRowIndexes
        {
            BeebListViewController.beeblistdata.selectedFiles.append(Int32(r))
            print(">> \(r)")
        }
        
        
        beeb_ExportDiscFilesToFolder();
    }
}

extension BeebListViewController: NSTableViewDataSource {
    func numberOfRows(in tableView: NSTableView) -> Int {
        return BeebListViewController.beeblistdata.rows.count
    }
}

extension BeebListViewController: NSTableViewDelegate {
    
    
    fileprivate enum CellIdentifiers {
      static let ColumnCell = NSUserInterfaceItemIdentifier(rawValue: "COLUMN")
    }

    func tableView(_ tableView: NSTableView, rowViewForRow row: Int) -> NSTableRowView? {
        print("\(#function) \(row) \(BeebListViewController.beeblistdata.rows[row])")
        return nil
    }
    
    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        print("\(#function) \(tableColumn!.identifier) \(row) \(BeebListViewController.beeblistdata.rows[row])")

        if let cell = tableView.makeView(withIdentifier: tableColumn!.identifier, owner: nil) as? NSTableCellView {
            cell.textField?.stringValue = BeebListViewController.beeblistdata.rows[row]
            return cell
        }
        return nil
    }
    
}
