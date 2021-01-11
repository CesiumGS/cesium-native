const yargs = require("yargs");
const path = require("path");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate")

const argv = yargs.options({
  schema: {
    alias: "s",
    description: "The path to the glTF 2.0 JSONSchema files.",
    demandOption: true,
    type: "string"
  },
  output: {
    alias: "o",
    description: "The output directory for the generated glTF class files.",
    demandOption: true,
    type: "string"
  },
  readerOutput: {
    alias: "r",
    description: "The output directory for the generated reader files.",
    demandOption: true,
    type: "string"
  },
  extensions: {
    alias: "e",
    description: "The extensions directory.",
    demandOption: true,
    type: "string"
  }
}).argv;

const nameMapping = {
  "glTF": "Model",
  "glTF Id": "ExtensibleObject",
  "glTFRootProperty": "NamedObject",
  "glTF Property": "ExtensibleObject",
  "glTF Child of Root Property": "NamedObject"
};

// Custom `cesium` properties will be added to these objects.
// e.g. Buffer will gain `BufferCesium cesium;`
const cesiumProperties = {
  "Buffer": true,
  "Image": true
};

const schemaCache = new SchemaCache(argv.schema, argv.extensions);
const modelSchema = schemaCache.load("glTF.schema.json");

const options = {
  schemaCache,
  nameMapping,
  cesiumProperties,
  outputDir: argv.output,
  readerOutputDir: argv.readerOutput,
  // key: Title of the element name that is extended (e.g. "Mesh Primitive")
  // value: Array of extension type names.
  extensions: {}
};

let schemas = [modelSchema];

const extensionInfo = {
  "KHR_draco_mesh_compression": {
    extensionName: "KHR_draco_mesh_compression",
    schema: "Khronos/KHR_draco_mesh_compression/schema/mesh.primitive.KHR_draco_mesh_compression.schema.json",
    attachTo: [
      "mesh.primitive"
    ]
  }
};

for (const extensionClassName of Object.keys(extensionInfo)) {
  const extension = extensionInfo[extensionClassName];
  const extensionSchema = schemaCache.loadExtension(extension.schema);
  if (!extensionSchema) {
    console.warn(`Could not load schema ${inExtensions[extensionClassName]} for extension class ${extensionClassName}.`);
    continue;
  }

  nameMapping[extensionSchema.title] = extensionClassName;
  schemas.push(...generate(options, extensionSchema));

  for (const objectToExtend of extension.attachTo) {
    const objectToExtendSchema = schemaCache.load(`${objectToExtend}.schema.json`);
    if (!objectToExtendSchema) {
      console.warn("Could not load schema for ${objectToExtend}.");
      continue;
    }

    if (!options.extensions[objectToExtend]) {
      options.extensions[objectToExtendSchema.title] = [];
    }

    options.extensions[objectToExtendSchema.title].push({
      name: extension.extensionName,
      className: extensionClassName
    });
  }
}

const processed = {};

while (schemas.length > 0) {
  const schema = schemas.pop();
  if (processed[schema.title]) {
    continue;
  }
  processed[schema.title] = true;
  schemas.push(...generate(options, schema));
}
