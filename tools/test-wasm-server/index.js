const express = require("express");

const app = express();
const port = 3123;

app.use(express.static("build/CesiumNativeTests", {
  setHeaders: (res, path, stat) => {
    res.set("Cross-Origin-Opener-Policy", "same-origin");
    res.set("Cross-Origin-Embedder-Policy", "require-corp");
  }
}));

app.listen(port, () => {
  console.log(`listening on port ${port}`);
});