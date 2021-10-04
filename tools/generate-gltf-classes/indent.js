function indent(text, spaces) {
  let indentText = "";
  for (let i = 0; i < spaces; ++i) {
    indentText += " ";
  }

  return text.replace(new RegExp("\r?\n", "gm"), "\n" + indentText);
}

module.exports = indent;
