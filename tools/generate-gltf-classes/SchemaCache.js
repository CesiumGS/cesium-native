const path = require("path");
const fs = require("fs");
const request = require('sync-request');

function readJsonStringFromFile(path) {
    try {
        const result = fs.readFileSync(path, "utf-8");
        return result;
    } catch(e) {
        return undefined;
    }
}
function readJsonStringFromUrl(url) {
    try {
        const res = request('GET', url);
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
    constructor(schemaPath, extensionPath) {
        this.schemaPath = schemaPath;
        this.extensionPath = extensionPath;
        this.cache = {};
        this.contextStack = [];
        this.byTitle = {};
    }

    load(name) {
        // First try loading relative to the current context
        let path = this.resolveRelativePath(name);

        const existing = this.cache[path];
        if (existing) {
            return existing;
        }

        let jsonString = readJsonString(path);
        if (jsonString === undefined) {
            // Next try resolving relative to the base URL
            const pathFromBase = this.resolvePathFromBase(name);
            
            const existingFromBase = this.cache[pathFromBase];
            if (existingFromBase) {
                return existingFromBase;
            }
    
            jsonString = readJsonString(pathFromBase);

            if (jsonString === undefined) {
                console.warn(`Could not resolve ${name}. Tried:\n  * ${path}\n  *${pathFromBase}`);
                return undefined;
            }

            path = pathFromBase;
        }

        const result = JSON.parse(jsonString);
        result.sourcePath = path;
        this.cache[path] = result;

        const upperTitle = result.title.toUpperCase();
        if (this.byTitle[upperTitle]) {
            console.warn(`*** Two schema files share the same title, things will be broken:\n  ${this.byTitle[upperTitle].sourcePath}\n  ${path}`);
        }

        this.byTitle[upperTitle] = result;

        return result;
    }

    loadExtension(name) {
        const path = this.resolvePath(this.extensionPath, name);

        const existing = this.cache[path];
        if (existing) {
            return existing;
        }

        const jsonString = readJsonString(path);
        const result = JSON.parse(jsonString, "utf-8");
        result.sourcePath = path;
        this.cache[name] = result;

        const upperTitle = result.title.toUpperCase();
        if (this.byTitle[upperTitle]) {
            console.warn(`*** Two schema files share the same title, things will be broken:\n  ${this.byTitle[upperTitle].sourcePath}\n  ${path}`);
        }

        this.byTitle[result.title.toUpperCase()] = result;

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

    resolvePathFromBase(name) {
        return this.resolvePath(this.schemaPath, name);
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
