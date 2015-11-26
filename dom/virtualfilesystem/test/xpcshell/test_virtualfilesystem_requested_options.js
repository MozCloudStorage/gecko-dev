"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const fileSystemId = "testFileSystemId";
const requestId = 1;
const path = "/dummy/path/";

function run_test() {
  run_next_test();
}

add_test(function test_virtualfilesystem_abort_option() {
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-abort-requested-options;1"].
                createInstance(Ci.nsIVirtualFileSystemAbortRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.operationRequestId = requestId;

  equal(options.fileSystemId, fileSystemId);
  equal(options.operationRequestId, requestId);

  run_next_test();
});

add_test(function test_virtualfilesystem_getMetadata_option() {
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-getmetadata-requested-options;1"].
                createInstance(Ci.nsIVirtualFileSystemGetMetadataRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.entryPath = path;

  equal(options.fileSystemId, fileSystemId);
  equal(options.entryPath, path);

  run_next_test();
});

add_test(function test_virtualfilesystem_closeFile_option() {
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-closefile-requested-options;1"].
               createInstance(Ci.nsIVirtualFileSystemCloseFileRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.openRequestId = requestId;

  equal(options.fileSystemId, fileSystemId);
  equal(options.openRequestId, requestId);

  run_next_test();
});

add_test(function test_virtualfilesystem_openFile_option() {
  var mode = Ci.nsIVirtualFileSystemOpenFileRequestedOptions.OPEN_MODE_READ;
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-openfile-requested-options;1"].
                createInstance(Ci.nsIVirtualFileSystemOpenFileRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.filePath = path;
  options.mode = mode;

  equal(options.fileSystemId, fileSystemId);
  equal(options.filePath, path);
  equal(options.mode, mode);

  run_next_test();
});

add_test(function test_virtualfilesystem_readDirectory_option() {
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-readdirectory-requested-options;1"].
               createInstance(Ci.nsIVirtualFileSystemReadDirectoryRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.dirPath = path;

  equal(options.fileSystemId, fileSystemId);
  equal(options.dirPath, path);

  run_next_test();
});

add_test(function test_virtualfilesystem_readFile_option() {
  var offset = 0;
  var length = 100;
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-readfile-requested-options;1"].
               createInstance(Ci.nsIVirtualFileSystemReadFileRequestedOptions);
  options.fileSystemId = fileSystemId;
  options.openRequestId = requestId;
  options.offset = offset;
  options.length = length;

  equal(options.fileSystemId, fileSystemId);
  equal(options.openRequestId, requestId);
  equal(options.offset, offset);
  equal(options.length, length);

  run_next_test();
});

add_test(function test_virtualfilesystem_unmount_option() {
  var options = Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-unmount-requested-options;1"].
               createInstance(Ci.nsIVirtualFileSystemUnmountRequestedOptions);
  options.fileSystemId = fileSystemId;

  equal(options.fileSystemId, fileSystemId);

  run_next_test();
});