const yargs = require("yargs");
const path = require("path");
const fs = require("fs");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate");
const generateCombinedWriter = require("./generateCombinedWriter");
const generateRegisterExtensions = require("./generateRegisterExtensions");
const getNameFromTitle = require("./getNameFromTitle");

const argv = yargs.options({
  schemas: {
    description: "The paths to the JSON schemas.",
    demandOption: true,
    type: "array",
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
  writerOutput: {
    description: "The output directory for the generated writer files.",
    demandOption: true,
    type: "string",
  },
  extensions: {
    description: "The extension directories.",
    demandOption: true,
    type: "array",
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
    description: "Namespace to put the generated reader files in.",
    demandOption: true,
    type: "string",
  },
  writerNamespace: {
    description: "Namespace to put the generated writer files in.",
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

const config = JSON.parse(fs.readFileSync(argv.config, "utf-8"));

const schemaBasePaths = [];
for (const schemaPath of argv.schemas) {
  const { _, schemaBasePath } = splitSchemaPath(schemaPath);
  schemaBasePaths.push(schemaBasePath);
}

const schemaCache = new SchemaCache(schemaBasePaths, argv.extensions);

// Add extensions to schema cache so that extensions can extend objects in other extensions
for (const extension of config.extensions) {
  const extensionSchema = schemaCache.loadExtension(
    extension.schema,
    extension.extensionName
  );
  const { _, schemaBasePath } = splitSchemaPath(extensionSchema.sourcePath);
  schemaCache.schemaPaths.push(schemaBasePath);
}

let schemas = [];
for (const schemaPath of argv.schemas) {
  const { schemaName, _ } = splitSchemaPath(schemaPath);
  const schema = schemaCache.load(schemaName);
  schemas.push(schema);
}
const rootSchema = schemas[0];

if (argv.oneHandlerFile) {
  // Clear the handler implementation file, and then we'll append to it in `generate`.
  const readerHeaderOutputDir = path.join(
    argv.readerOutput,
    "generated",
    "src"
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
  writerNamespace: argv.writerNamespace,
  // key: Title of the element name that is extended (e.g. "Mesh Primitive")
  // value: Array of extension type names.
  extensions: {},
};

const writers = [];

function getObjectToExtend(schema, extensionName) {
  const parts = schema.split("/");
  const last = parts[parts.length - 1];
  const subParts = last.split(".");
  const extensionNameIndex = subParts.indexOf(extensionName);
  if (extensionNameIndex == -1 || extensionNameIndex == 0) {
    return undefined;
  }
  const objectToExtend = subParts.slice(0, extensionNameIndex).join(".");
  return objectToExtend;
}

for (const extension of config.extensions) {
  const extensionSchema = schemaCache.loadExtension(
    extension.schema,
    extension.extensionName
  );
  const extensionClassName = getNameFromTitle(config, extensionSchema.title);

  if (!extensionSchema) {
    console.warn(
      `Could not load schema ${extension.schema} for extension class ${extensionClassName}.`
    );
    continue;
  }

  if (!config.classes[extensionSchema.title]) {
    config.classes[extensionSchema.title] = {};
  }
  config.classes[extensionSchema.title].overrideName = extensionClassName;
  config.classes[extensionSchema.title].extensionName = extension.extensionName;

  schemas.push(...generate(options, extensionSchema, writers));

  var objectsToExtend = [];
  if (extension.attachTo !== undefined) {
    objectsToExtend = objectsToExtend.concat(extension.attachTo);
  } else {
    const attachTo = getObjectToExtend(
      extension.schema,
      extension.extensionName,
      extensionClassName
    );
    if (attachTo !== undefined) {
      objectsToExtend.push(attachTo);
    }
  }
  if (objectsToExtend.length === 0) {
    console.warn(
      `Could not find object to extend for extension class ${extensionClassName}`
    );
    continue;
  }

  for (const objectToExtend of objectsToExtend) {
    const objectToExtendSchema = schemaCache.load(
      `${objectToExtend}.schema.json`
    );
    if (!objectToExtendSchema) {
      console.warn(`Could not load schema for ${objectToExtend}.`);
      continue;
    }

    if (!options.extensions[objectToExtendSchema.title]) {
      options.extensions[objectToExtendSchema.title] = [];
    }

    options.extensions[objectToExtendSchema.title].push({
      name: extension.extensionName,
      className: extensionClassName,
    });
  }
}

function processSchemas() {
  const processed = {};

  while (schemas.length > 0) {
    const schema = schemas.pop();
    if (processed[schema.sourcePath]) {
      continue;
    }
    processed[schema.sourcePath] = true;

    if ((options.config.classes[schema.title] || {}).manuallyDefined) {
      continue;
    }
    schemas.push(...generate(options, schema, writers));
  }
}

processSchemas();

generateRegisterExtensions({
  readerOutputDir: argv.readerOutput,
  writerOutputDir: argv.writerOutput,
  config: config,
  namespace: argv.namespace,
  readerNamespace: argv.readerNamespace,
  writerNamespace: argv.writerNamespace,
  rootSchema: rootSchema,
  extensions: options.extensions,
});

generateCombinedWriter({
  writerOutputDir: argv.writerOutput,
  config: config,
  namespace: argv.namespace,
  writerNamespace: argv.writerNamespace,
  rootSchema: rootSchema,
  writers: writers,
  extensions: options.extensions,
});
