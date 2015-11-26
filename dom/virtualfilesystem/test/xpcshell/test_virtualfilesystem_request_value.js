"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const metaData = {isDirectory: false,
                  name: "test1.txt",
                  size: 10,
                  modificationTime: 100,
                  mimeType: "text/plain"};

const metaData1 = {isDirectory: false,
                   name: "test2.txt",
                   size: 100,
                   modificationTime: 200,
                   mimeType: "text/plain"};

function run_test() {
  run_next_test();
}

add_test(function test_virtualfilesystem_readdirectory_request_value() {
  var value = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-readdirectory-request-value;1"].
              createInstance(Ci.nsIVirtualFileSystemReadDirectoryRequestValue);
  value.addEntryMetadata(metaData);
  var entries = value.getEntries();
  equal(entries[0].isDirectory, metaData.isDirectory);
  equal(entries[0].name, metaData.name);
  equal(entries[0].size, metaData.size);
  equal(entries[0].modificationTime, metaData.modificationTime);
  equal(entries[0].mimeType, metaData.mimeType);

  var value1 = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-readdirectory-request-value;1"].
               createInstance(Ci.nsIVirtualFileSystemReadDirectoryRequestValue);
  value1.addEntryMetadata(metaData1);
  value.Concat(value1);
  entries = value.getEntries();
  ok(entries.length === 2);

  equal(entries[1].isDirectory, metaData1.isDirectory);
  equal(entries[1].name, metaData1.name);
  equal(entries[1].size, metaData1.size);
  equal(entries[1].modificationTime, metaData1.modificationTime);
  equal(entries[1].mimeType, metaData1.mimeType);

  run_next_test();
});