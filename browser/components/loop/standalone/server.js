/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* eslint-env node */

/* XXX We should enable these and fix the warnings, but at the time of this
 * writing, we're just bootstrapping the linting infrastructure.
 */
/* eslint-disable no-path-concat,no-process-exit */

var express = require("express");
var app = express();

// make dev-server performance more similar to the production server by using
// gzip compression
var compression = require("compression");
app.use(compression());

var path = require("path");

var port = process.env.PORT || 3000;
var feedbackApiUrl = process.env.LOOP_FEEDBACK_API_URL ||
                     "https://input.allizom.org/api/v1/feedback";
var feedbackProductName = process.env.LOOP_FEEDBACK_PRODUCT_NAME || "Loop";
var loopServerUrl = process.env.LOOP_SERVER_URL || "http://localhost:5000";

// This is typically overridden with "dist" so that it's possible to test the
// optimized version, once it's been built to the "dist" directory
var standaloneContentDir = process.env.LOOP_CONTENT_DIR || "content";

// Remove trailing slashes as double slashes in the url can confuse the server
// responses.
if (loopServerUrl[loopServerUrl.length - 1] === "/") {
  loopServerUrl = loopServerUrl.slice(0, -1);
}

function getConfigFile(req, res) {
  "use strict";

  res.set("Content-Type", "text/javascript");
  res.send([
    "var loop = loop || {};",
    "loop.config = loop.config || {};",
    "loop.config.serverUrl = '" + loopServerUrl + "/v0';",
    "loop.config.feedbackApiUrl = '" + feedbackApiUrl + "';",
    "loop.config.feedbackProductName = '" + feedbackProductName + "';",
    "loop.config.downloadFirefoxUrl = 'https://www.mozilla.org/firefox/new/?scene=2&utm_source=hello.firefox.com&utm_medium=referral&utm_campaign=non-webrtc-browser#download-fx';",
    "loop.config.privacyWebsiteUrl = 'https://www.mozilla.org/privacy/firefox-hello/';",
    "loop.config.learnMoreUrl = 'https://www.mozilla.org/hello/';",
    "loop.config.legalWebsiteUrl = 'https://www.mozilla.org/about/legal/terms/firefox-hello/';",
    "loop.config.roomsSupportUrl = 'https://support.mozilla.org/kb/group-conversations-firefox-hello-webrtc';",
    "loop.config.tilesIframeUrl = 'https://tiles.cdn.mozilla.net/iframe.html';",
    "loop.config.tilesSupportUrl = 'https://support.mozilla.org/tiles-firefox-hello';",
    "loop.config.guestSupportUrl = 'https://support.mozilla.org/kb/respond-firefox-hello-invitation-guest-mode';",
    "loop.config.generalSupportUrl = 'https://support.mozilla.org/kb/respond-firefox-hello-invitation-guest-mode';",
    "loop.config.unsupportedPlatformUrl = 'https://support.mozilla.org/en-US/kb/which-browsers-will-work-firefox-hello-video-chat'"
  ].join("\n"));
}

app.get("/content/config.js", getConfigFile);
app.get("/content/c/config.js", getConfigFile);

// Various mappings to let us end up with:
// /test - for the test files
// /ui - for the ui showcase
// /content - for the standalone files.

app.use("/ui", express.static(path.join(__dirname, "..", "ui")));
app.use("/ui/loop/", express.static(path.join(__dirname, "..", "content")));
app.use("/ui/shared/", express.static(path.join(__dirname, "..", "content",
                                                "shared")));

// This exists exclusively for the unit tests. They are served the
// whole loop/ directory structure and expect some files in the standalone directory.
app.use("/standalone/content", express.static(path.join(__dirname, "content")));

// We load /content this from  both /content *and* /../content. The first one
// does what we need for running in the github loop-client context, the second one
// handles running in the hg repo under mozilla-central and is used so that the shared
// files are in the right location.
app.use("/content", express.static(path.join(__dirname, standaloneContentDir)));
app.use("/content", express.static(path.join(__dirname, "..", "content")));
// These two are based on the above, but handle call urls, that have a /c/ in them.
app.use("/content/c", express.static(path.join(__dirname,
  standaloneContentDir)));
app.use("/content/c", express.static(path.join(__dirname, "..", "content")));

// Two lines for the same reason as /content above.
app.use("/test", express.static(path.join(__dirname, "test")));
app.use("/test", express.static(path.join(__dirname, "..", "test")));

// As we don't have hashes on the urls, the best way to serve the index files
// appears to be to be to closely filter the url and match appropriately.
function serveIndex(req, res) {
  "use strict";

  return res.sendFile(path.join(__dirname, standaloneContentDir, "index.html"));
}

app.get(/^\/content\/[\w\-]+$/, serveIndex);
app.get(/^\/content\/c\/[\w\-]+$/, serveIndex);

var server = app.listen(port);

var baseUrl = "http://localhost:" + port + "/";

console.log("Static contents are available at " + baseUrl + "content/");
console.log("Tests are viewable at " + baseUrl + "test/");
console.log("Use this for development only.");

// Handle SIGTERM signal.
function shutdown(cb) {
  "use strict";

  try {
    server.close(function() {
      process.exit(0);
      if (cb !== undefined) {
        cb();
      }
    });

  } catch (ex) {
    console.log(ex + " while calling server.close)");
  }
}

process.on("SIGTERM", shutdown);
