const express = require("express");
const fs = require("fs");

const app = express();
const port = 3123;

app.use(express.static("./", {
  setHeaders: (res, path, stat) => {
    res.set("Cross-Origin-Opener-Policy", "same-origin");
    res.set("Cross-Origin-Embedder-Policy", "require-corp");
  }
}));

const emsdk = process.env.EMSDK;

if (emsdk && emsdk.length > 0) {
  console.log(`Using emsdk at: ${emsdk}`);

  app.use("/emsdk/emscripten", express.static(`${emsdk}/upstream/emscripten`, {
    setHeaders: (res, path, stat) => {
      res.set("Cross-Origin-Opener-Policy", "same-origin");
      res.set("Cross-Origin-Embedder-Policy", "require-corp");
    }
  }));

  app.use("/emsdk/upstream", express.static(`${emsdk}/upstream`, {
    setHeaders: (res, path, stat) => {
      res.set("Cross-Origin-Opener-Policy", "same-origin");
      res.set("Cross-Origin-Embedder-Policy", "require-corp");
    }
  }));
} else {
  console.log("Not mapping emsdk path because EMSDK environment variable is not set.");
}

const ezvcpkg = process.env.EZVCPKG_BASEDIR;
if (!ezvcpkg || ezvcpkg.length === 0) {
  ezvcpkg = `${process.env.HOME ?? ""}/.ezvcpkg`;
}

if (fs.existsSync(ezvcpkg)) {
  console.log(`Using ezvcpkg at: ${ezvcpkg}`);
  
  app.use("/ezvcpkg", express.static(ezvcpkg, {
    setHeaders: (res, path, stat) => {
      res.set("Cross-Origin-Opener-Policy", "same-origin");
      res.set("Cross-Origin-Embedder-Policy", "require-corp");
    }
  }));
} else {
  console.log("Not mapping ezvcpkg path because EZVCPKG_BASEDIR environment variable is not set or directory does not exist.");
}

app.listen(port, () => {
  console.log(`listening on port ${port}`);
});