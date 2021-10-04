function makeIdentifier(s) {
  return s.replace(/\//g, "_");
}

module.exports = makeIdentifier;
