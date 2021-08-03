const path = require("path");
const fs = require("fs");
const request = require('sync-request');
const lodash = require("lodash");

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
    constructor(schemaPaths) {
        this.schemaPaths = schemaPaths;
        this.cache = {};
    }

    #loadFromPath(name) {
      for (const basePath of this.schemaPaths) {
        try {
            const jsonString = readJsonString(basePath, name);
            const result = JSON.parse(jsonString, "utf-8");
            this.cache[name] = result;
            return result;
        } catch (err) {}
      }
    }

    load(name, path) {
        const result = this.cache[name] || this.#loadFromPath(name);

        const refPath = (path || "").split("/").filter((c) => c.length > 0);
        if (refPath.length > 0) {
            return lodash.get(result, refPath);
        } else {
            return result;
        }
    }
}

module.exports = SchemaCache;
