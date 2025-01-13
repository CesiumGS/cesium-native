/**
 * Attempts to conform all `#include` directives to match the style guide.
 * Including header files from the same directory as the current file will
 * be formatted as #include "file.h"
 * Including header files from other directories will be formatted as 
 * #include <file.h>
 * 
 * This script should be run in the root directory of the project, 
 * like `node tools/conform-includes.js`.
 */

const fs = require("fs");
const path = require("path");

const dirs = fs.readdirSync(".").filter(d => d.startsWith("Cesium"));
console.log(dirs);

const libs = {};
dirs.forEach(d => {
  const lib = {
    name: d,
    files: [],
    headerFiles: [],
    privateHeaders: []
  };

  const srcDir = path.join(d, "src");
  if (fs.existsSync(srcDir)) {
    const srcFiles = fs.readdirSync(srcDir).filter(t => t.endsWith(".cpp") || t.endsWith(".h"));
    lib.files = lib.files.concat(srcFiles.map(f => path.join(srcDir, f)));
    lib.privateHeaders = lib.privateHeaders.concat(srcFiles.filter(f => f.endsWith(".h")));
  }

  const includeDir = path.join(d, "include", d);
  if (fs.existsSync(includeDir)) {
    const includeFiles = fs.readdirSync(includeDir).filter(t => t.endsWith(".cpp") || t.endsWith(".h"));
    lib.files = lib.files.concat(includeFiles.map(f => path.join(includeDir, f)));
    lib.headerFiles = lib.headerFiles.concat(includeFiles);
  }

  const generatedIncludeDir = path.join(d, "generated", "include", d);
  if (fs.existsSync(generatedIncludeDir)) {
    const generatedIncludeFiles = fs.readdirSync(generatedIncludeDir).filter(t => t.endsWith(".h"));
    // We don't want to actually process generated files, but we do want to know they're there.
    lib.headerFiles = lib.headerFiles.concat(generatedIncludeFiles);
  }

  const testDir = path.join(d, "test");
  if (fs.existsSync(testDir)) {
    const testFiles = fs.readdirSync(testDir).filter(t => t.endsWith(".cpp") || t.endsWith(".h"));
    lib.files = lib.files.concat(testFiles.map(f => path.join(testDir, f)));
  }

  libs[d] = lib;
});

const includeRegex = /#include\s*(\<(.+?)\>|\"(.+?)\")/g;
function conformFile(file, libName) {
  const fileContents = fs.readFileSync(file, "utf-8");
  const newContents = fileContents.replace(includeRegex, (str, _, a, b) => {
    const filePath = a || b;
    const pathParts = path.parse(filePath);
    const pathLib = pathParts.dir;
    const pathFile = pathParts.base;
    // No directory part of the path name, or it's the current lib anyways
    if (!pathLib.trim() || pathLib == libName) {
      // Points to a known header file of this lib
      if (libs[libName].headerFiles.indexOf(pathFile) != -1) {
        return `#include <${libName}/${pathFile}>`;
      }

      // Private include in the source directory
      if (libs[libName].privateHeaders.indexOf(pathFile) != -1) {
        return `#include "${pathFile}"`;
      }

      if (pathLib == libName) {
        console.error(`${filePath} is supposed to be in lib ${libName} but wasn't found?`);
        return str;
      }

      // It's an include we don't know how to deal with, just ignore it
      return str;
    }

    if (libs[pathLib] && libs[pathLib].headerFiles.indexOf(pathFile) != -1) {
      return `#include <${pathLib}/${pathFile}>`;
    }

    return str;
  });
  fs.writeFileSync(file, newContents);
}

Object.keys(libs).forEach(libName => {
  libs[libName].files.forEach(f => {
    console.log(`conforming ${f}`);
    conformFile(f, libName);
  });
});