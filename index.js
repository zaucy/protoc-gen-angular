const child_process = require("child_process");
const fs = require("fs");
const path = require("path");
const progressDownload = require("progress-download");

const PACKAGE_JSON_PATH = path.resolve(__dirname, 'package.json');
const PLATFORM = process.platform;
const ARCH = process.arch;
const EXTNAME = PLATFORM == 'win32' ? '.exe' : '';
const PLUGIN_PATH = path.resolve(
  __dirname, 'release', `protoc-gen-angular-${PLATFORM}-${ARCH}${EXTNAME}`
);

module.exports = function() {
  return child_process.execFile.apply(
    child_process, [PLUGIN_PATH].concat(arguments)
  );
};

module.exports.PLUGIN_PATH = PLUGIN_PATH;
