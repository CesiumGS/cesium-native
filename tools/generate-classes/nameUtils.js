function makeIdentifier(s) {
    return s.replace(/\//g, "_");
}

function toPascalCase(name) {
  if (name.length === 0) {
    return name;
  }

  return name[0].toUpperCase() + name.substr(1);
}

function formatPart(name) {
  const parts = name.split(/[. _-]/);
  if (parts[parts.length - 1] == "json")
    parts.pop();
  if (parts[parts.length - 1] == "schema")
    parts.pop();
  return parts.map(toPascalCase).join("");
}

function makeTypeName(name, suffix) {
  const refParts = name.split("#", 2);
  const refSchema = refParts[0];
  const refPath = (refParts[1] || "").split("/").filter((c) => c.length > 0);

  if (refPath[0] === "definitions" || refPath[0] === "defs") {
    refPath.shift();
  }

  if (suffix === undefined) {
    suffix = "";
  }
  return [refSchema, ...refPath]
    .filter((c) => c.length > 0)
    .map((part) => formatPart(part) + suffix)
    .join("::");
}

module.exports = {
  makeIdentifier: makeIdentifier,
  makeTypeName: makeTypeName
};

