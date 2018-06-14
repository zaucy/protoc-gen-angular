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

if(!fs.existsSync(PLUGIN_PATH)) {
  const {version} = JSON.parse(fs.readFileSync(PACKAGE_JSON_PATH, 'utf8'));
  const PLUGIN_DOWNLOAD_URL = 
    `https://github.com/zaucy/protoc-gen-angular/releases/download/` +
    `v${version}/protoc-gen-angular-${PLATFORM}-${ARCH}${EXTNAME}`;

  progressDownload(PLUGIN_DOWNLOAD_URL, PLUGIN_PATH).then(() => {

  });
}
