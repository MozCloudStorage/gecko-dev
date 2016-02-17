"use strict";

SimpleTest.waitForExplicitFinish();

var iframeUrl = SimpleTest.getTestFileURL('file_virtualfilesystem_mount.html');

function runTest() {
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
      info("Command from iframe: " + message);
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