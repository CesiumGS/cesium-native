const path = require("path");
const fs = require("fs");
const request = require("sync-request");

function readJsonStringFromFile(path) {
  try {
    const result = fs.readFileSync(path, "utf-8");
    return result;
  } catch (e) {
    return undefined;
  }
}
function readJsonStringFromUrl(url) {
  try {
    const res = request("GET", url);
    return res.getBody("utf-8");
  } catch (e) {
    return undefined;
  }
}
function readJsonString(pathOrUrl) {
  if (pathOrUrl.startsWith("http")) {
    return readJsonStringFromUrl(pathOrUrl);
  }
  return readJsonStringFromFile(pathOrUrl);
}

class SchemaCache {
  constructor(schemaPaths, extensionPaths) {
    this.schemaPaths = schemaPaths;
    this.extensionSchemaPaths = extensionPaths;
    this.cache = {};
    this.contextStack = [];
    this.byTitle = {};
  }

  load(name) {
    // First try loading relative to the current context
    // Then try loading from the base schema paths
    return this.loadFromSearchPaths(name, [
      this.resolveRelativePath(name),
      ...this.schemaPaths.map((path) => this.resolvePath(path, name)),
    ]);
  }

  loadExtension(name) {
    return this.loadFromSearchPaths(
      name,
      this.extensionSchemaPaths.map((path) => this.resolvePath(path, name))
    );
  }

  loadFromSearchPaths(name, paths) {
    let jsonString;

    let path;
    for (path of paths) {
      const existingSchema = this.cache[path];
      if (existingSchema) {
        return existingSchema;
      }

      jsonString = readJsonString(path);

      if (jsonString) {
        break;
      }
    }

    if (jsonString === undefined) {
      console.warn(
        `Could not resolve ${name}. Tried:\n${paths
          .map((path) => `  * ${path}`)
          .join("\n")}`
      );
      return undefined;
    }

    const result = JSON.parse(jsonString);
    result.sourcePath = path;
    this.cache[path] = result;

    const upperTitle = result.title.toUpperCase();
    if (this.byTitle[upperTitle]) {
      console.warn(
        `*** Two schema files share the same title, things will be broken:\n  ${this.byTitle[upperTitle].sourcePath}\n  ${path}`
      );
    }

    this.byTitle[upperTitle] = result;

    return result;
  }

  pushContext(schema) {
    this.contextStack.push(schema);
  }

  popContext() {
    this.contextStack.pop();
  }

  resolveRelativePath(name) {
    if (this.contextStack.length === 0) {
      return name;
    }

    const base = this.contextStack[this.contextStack.length - 1].sourcePath;
    return this.resolvePath(base, name);
  }

  resolvePath(base, name) {
    if (base.startsWith("http")) {
      return new URL(name, base).href;
    } else {
      return path.resolve(base, "..", name);
    }
  }
}

module.exports = SchemaCache;
