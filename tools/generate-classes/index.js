const yargs = require("yargs");
const path = require("path");
const fs = require("fs");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate");
const generateRegisterExtensions = require("./generateRegisterExtensions");

const argv = yargs.options({
  schema: {
    description: "The path to the JSON schema.",
    demandOption: true,
    type: "string",
  },
  output: {
    description: "The output directory for the generated class files.",
    demandOption: true,
    type: "string",
  },
  readerOutput: {
    description: "The output directory for the generated reader files.",
    demandOption: true,
    type: "string",
  },
  extensions: {
    description: "The extensions directory.",
    demandOption: true,
    type: "string",
  },
  config: {
    description:
      "The path to the configuration options controlling code generation, expressed in a JSON file.",
    demandOption: true,
    type: "string",
  },
  namespace: {
    description: "Namespace to put the generated classes in.",
    demandOption: true,
    type: "string",
  },
  readerNamespace: {
    description: "Namespace to put the generated reader classes in.",
    demandOption: true,
    type: "string",
  },
  oneHandlerFile: {
    description:
      "Generate all the JSON handler implementations into a single file, GeneratedJsonHandlers.cpp.",
    type: "bool",
    default: true,
  },
}).argv;

function splitSchemaPath(schemaPath) {
  const schemaNameIndex = schemaPath.lastIndexOf("/") + 1;
  const schemaName = schemaPath.slice(schemaNameIndex);
  const schemaBasePath = schemaPath.slice(0, schemaNameIndex);
  return { schemaName, schemaBasePath };
}

const { schemaName, schemaBasePath } = splitSchemaPath(argv.schema);
const schemaCache = new SchemaCache(schemaBasePath, argv.extensions);
const rootSchema = schemaCache.load(schemaName);

const config = JSON.parse(fs.readFileSync(argv.config, "utf-8"));

if (argv.oneHandlerFile) {
  // Clear the handler implementation file, and then we'll append to it in `generate`.
  const readerHeaderOutputDir = path.join(
    argv.readerOutput,
    "generated",
    "src",
    argv.readerNamespace
  );
  fs.mkdirSync(readerHeaderOutputDir, { recursive: true });
  const readerSourceOutputPath = path.join(
    readerHeaderOutputDir,
    "GeneratedJsonHandlers.cpp"
  );
  fs.writeFileSync(readerSourceOutputPath, "", "utf-8");
}

const options = {
  schemaCache,
  oneHandlerFile: argv.oneHandlerFile,
  outputDir: argv.output,
  readerOutputDir: argv.readerOutput,
  config: config,
  namespace: argv.namespace,
  readerNamespace: argv.readerNamespace,
  // key: Title of the element name that is extended (e.g. "Mesh Primitive")
  // value: Array of extension type names.
  extensions: {},
};

let schemas = [rootSchema];

for (const extension of config.extensions) {
  const extensionSchema = schemaCache.loadExtension(extension.schema);
  if (!extensionSchema) {
    console.warn(
      `Could not load schema ${extension.schema} for extension class ${extension.className}.`
    );
    continue;
  }

  if (!config.classes[extensionSchema.title]) {
    config.classes[extensionSchema.title] = {};
  }
  config.classes[extensionSchema.title].overrideName = extension.className;
  config.classes[extensionSchema.title].extensionName = extension.extensionName;

  schemas.push(...generate(options, extensionSchema));

  for (const objectToExtend of extension.attachTo) {
    const objectToExtendSchema = schemaCache.load(
      `${objectToExtend}.schema.json`
    );
    if (!objectToExtendSchema) {
      console.warn("Could not load schema for ${objectToExtend}.");
      continue;
    }

    if (!options.extensions[objectToExtendSchema.title]) {
      options.extensions[objectToExtendSchema.title] = [];
    }

    options.extensions[objectToExtendSchema.title].push({
      name: extension.extensionName,
      className: extension.className,
    });
  }
}

const processed = {};

while (schemas.length > 0) {
  const schema = schemas.pop();
  if (processed[schema.sourcePath]) {
    continue;
  }
  processed[schema.sourcePath] = true;
  schemas.push(...generate(options, schema));
}

generateRegisterExtensions({
  readerOutputDir: argv.readerOutput,
  config: config,
  namespace: argv.namespace,
  readerNamespace: argv.readerNamespace,
  extensions: options.extensions,
});
