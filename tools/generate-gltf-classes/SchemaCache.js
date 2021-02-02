const path = require("path");
const fs = require("fs");

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

        const result = JSON.parse(fs.readFileSync(path.join(this.schemaPath, name), "utf-8"));
        this.cache[name] = result;
        return result;
    }

    loadExtension(name) {
        const existing = this.cache[name];
        if (existing) {
            return existing;
        }

        const result = JSON.parse(fs.readFileSync(path.join(this.extensionPath, name), "utf-8"));
        this.cache[name] = result;
        return result;
    }
}

module.exports = SchemaCache;
