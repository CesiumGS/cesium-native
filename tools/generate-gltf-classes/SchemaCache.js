const path = require("path");
const fs = require("fs");
const request = require('sync-request');

function readJsonStringFromFile(basePath, name) {
    const result = fs.readFileSync(path.join(basePath, name));
    return result;
}
function readJsonStringFromUrl(baseUrl, name) {
    var res = request('GET', new URL(name, baseUrl).href);
    return res.getBody();
}
function readJsonString(base, name) {
    if (base.startsWith("http")) {
        return readJsonStringFromUrl(base, name);
    }
    return readJsonStringFromFile(base, name);
}

class SchemaCache {
    constructor(schemaPath, extensionPath) {
        this.schemaPath = schemaPath;
        this.extensionPath = extensionPath;
        this.cache = {};
    }

    load(name) {
        const existing = this.cache[name];
        if (existing) {
            return existing;
        }

        const jsonString = readJsonString(this.schemaPath, name);
        const result = JSON.parse(jsonString, "utf-8");
        this.cache[name] = result;
        return result;
    }

    loadExtension(name) {
        const existing = this.cache[name];
        if (existing) {
            return existing;
        }

        const jsonString = readJsonString(this.extensionPath, name);
        const result = JSON.parse(jsonString, "utf-8");
        this.cache[name] = result;
        return result;
    }
}

module.exports = SchemaCache;
