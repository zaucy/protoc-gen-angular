#!/usr/bin/env node

const protocGenAngular = require("./index.js");

protocGenAngular(process.argv.slice(2), {
  windowsHide: true,
  stdio: 'inherit',
});
