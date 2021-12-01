function getNameFromTitle(config, title) {
  return config.classes[title] && config.classes[title].overrideName
    ? config.classes[title].overrideName
    : makeName(title);
}

function makeName(title) {
  const parts = title.split(/\s/);

  // Capitalize each space-delimited part
  for (let i = 0; i < parts.length; ++i) {
    parts[i] = parts[i][0].toUpperCase() + parts[i].substr(1);
  }

  return parts.join("");
}

module.exports = getNameFromTitle;
