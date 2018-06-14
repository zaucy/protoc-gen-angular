#!/usr/bin/env node

const protocGenAngular = require("./index.js");

protocGenAngular(process.argv.slice(2), {
  stdio: 'inherit',
  windowsHide: true
});
