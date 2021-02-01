function getNameFromSchema(config, schema) {
  const title = schema.title;
  return config.classes[title] && config.classes[title].overrideName ? config.classes[title].overrideName : title.replace(/\s/g, "");
}

module.exports = getNameFromSchema;