"use strict";

SimpleTest.waitForExplicitFinish();

var Ci = SpecialPowers.Ci;
var iframeUrl = SimpleTest.getTestFileURL('file_virtualfilesystem_event.html');
var fileSystem = undefined;

function getVirtualFileSystem(aFileSystemId) {
  let vfsService = SpecialPowers.Cc["@mozilla.org/virtualfilesystem/virtualfilesystem-service;1"].getService(Ci.nsIVirtualFileSystemService);
  return vfsService.getVirtualFileSystemById(aFileSystemId);
}

function testGetFileSystem() {
  fileSystem = getVirtualFileSystem(mountOption.fileSystemId);
  ok(fileSystem, "must have file system");
}

function doNextTest() {
  let emptyCallback = {
    onSuccess: function(requestId, value, hasMore) {},
    onError: function(requestId, error) {}
  };
  getVirtualFileSystem(mountOption.fileSystemId).abort(0, emptyCallback);
}

function testCloseFileEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      ok(true, "callback is invoked.");
      doNextTest();
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.closeFile(0, callback);
}

function compareMetadata(metadataA, metadataB)
{
  is(metadataA.name, metadataB.name, "entryMetadata.name should be the same.");
  is(metadataA.isDirectory, metadataB.isDirectory, "entryMetadata.isDirectory should be the same.");
  is(metadataA.size, metadataB.size, "entryMetadata.size should be the same.");
  is(metadataA.modificationTime, metadataB.modificationTime, "entryMetadata.modificationTime should be the same.");
}

function testGetMetadataEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      let val = value.QueryInterface(Ci.nsIVirtualFileSystemGetMetadataRequestValue);
      compareMetadata(val.metadata, entryMetadata);
      is(val.metadata.mimeType, entryMetadata.mimeType, "entryMetadata.mimeType should be the same.");
      doNextTest();
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.getMetadata(testPath, callback);
}

function testOpenFileEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      let info = navigator.fileSystemProvider.get(mountOption.fileSystemId);
      is(info.openedFiles[0].filePath, testPath, "opened file path should be the same");
      doNextTest();
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.openFile(testPath, Ci.nsIVirtualFileSystem.OPEN_FILE_MODE_READ, callback);
}

function testReadDirectoryEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      let val = value.QueryInterface(Ci.nsIVirtualFileSystemReadDirectoryRequestValue);
      let entries = val.getEntries();
      compareMetadata(entries[0], entryMetadata);
      compareMetadata(entries[1], entryMetadata1);
      doNextTest();
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.readDirectory(testPath, callback);
}

function testReadFileEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      let val = value.QueryInterface(Ci.nsIVirtualFileSystemReadFileRequestValue);
      is(val.data, "ABCDE", "Data should be the same");
      doNextTest();
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.readFile(0, 0, 5, callback);
}

function testUnmountEvent() {
  let callback = {
    onSuccess: function(requestId, value, hasMore) {
      ok(true, "callback is invoked.");
    },
    onError: function(requestId, error) {
    }
  };
  fileSystem.unmount(callback);
}

var tests = {
  "testGetFileSystem": testGetFileSystem,
  "testCloseFileEvent": testCloseFileEvent,
  "testGetMetadataEvent": testGetMetadataEvent,
  "testOpenFileEvent": testOpenFileEvent,
  "testReadDirectoryEvent": testReadDirectoryEvent,
  "testReadFileEvent": testReadFileEvent,
  "testUnmountEvent": testUnmountEvent
};

function runTest() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  SpecialPowers.addPermission("filesystemprovider",
                              true, { url: iframeUrl,
                                      originAttributes: {
                                        appId: SpecialPowers.Ci.nsIScriptSecurityManager.NO_APP_ID,
                                        inBrowser: true }});
  var iframe = document.createElement("iframe");
  var oop = location.pathname.indexOf('_inproc') == -1;
  iframe.setAttribute("mozbrowser", "true");
  iframe.setAttribute("remote", oop);
  iframe.setAttribute("src", iframeUrl);

  iframe.addEventListener('mozbrowsershowmodalprompt', function receiverListener(aEvent) {
    var message = aEvent.detail.message;
    if (/^OK /.exec(message)) {
      ok(true, "Message from iframe: " + message);
    } else if (/^KO /.exec(message)) {
      ok(false, "Message from iframe: " + message);
    } else if (/^INFO /.exec(message)) {
      info("Message from iframe: " + message);
    } else if (/^COMMAND /.exec(message)) {
      var command = JSON.parse(message.replace(/^COMMAND /, ''));
      var test = tests[command.doTest];
      test();
    } else if (/^DONE$/.exec(message)) {
      ok(true, "Messaging from iframe complete.");
      iframe.removeEventListener('mozbrowsershowmodalprompt', receiverListener);
      SimpleTest.finish();
    }
  }, false);

  document.body.appendChild(iframe);
}

SpecialPowers.pushPrefEnv({"set": [["dom.filesystemprovider.enabled", true],
                                    //to ignore app scope check.
                                   ["dom.ignore_webidl_scope_checks", true],
                                   ["dom.mozBrowserFramesEnabled", true],
                                   ["dom.ipc.tabs.disabled", false]]}, function() {
  SpecialPowers.pushPermissions(
    [{type: "browser", allow: true, context: document },
     {type: 'filesystemprovider', allow: true, context: document},], runTest);
});