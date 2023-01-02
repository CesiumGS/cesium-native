function unindent(indentedText) {
  let skip = 0;
  let indent = 0;
  let spaces = "";

  for (let i = 0; i < indentedText.length; ++i) {
    if (indentedText[i] == "\n" || indentedText[i] == "\r") {
      ++skip;
    } else if (indentedText[i] == " ") {
      ++indent;
      spaces += " ";
    } else {
      break;
    }
  }

  const regex = new RegExp("^(" + spaces + ")", "gm");
  const lastLine = new RegExp("^\\s+$", "gm");
  const unindented = indentedText
    .substr(skip)
    .replace(regex, "")
    .replace(lastLine, "");
  return unindented;
}

module.exports = unindent;
