function getNameFromSchema(nameMapping, schema) {
  const title = schema.title;
  return nameMapping[title] ? nameMapping[title] : title.replace(/\s/g, "");
}

module.exports = getNameFromSchema;