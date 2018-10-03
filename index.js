const child_process = require("child_process");
const path = require("path");

const PLATFORM = process.platform;
const EXTNAME = PLATFORM == 'win32' ? '.exe' : '';
const PLUGIN_PATH = path.resolve(
  __dirname, 'release', `protoc-gen-angular-${PLATFORM}${EXTNAME}`
);

module.exports = function(args, options) {
  return child_process.spawn(PLUGIN_PATH, args||[], options);
};

module.exports.PLUGIN_PATH = PLUGIN_PATH;
