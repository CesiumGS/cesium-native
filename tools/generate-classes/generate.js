const fs = require("fs");
const lodash = require("lodash");
const handlebars = require("handlebars");
const path = require("path");
const util = require('util');

const reslib = require("./resolveProperty");
const SchemaCache = require("./SchemaCache");

function loadTemplate(name) {
  return handlebars.compile(fs.readFileSync(path.join("templates", name)).toString(), {noEscape: true});
}

const modelTemplate = loadTemplate("Model.hb");
const modelPartials = {
  object: loadTemplate("ObjectModel.hb"),
  enum: loadTemplate("EnumModel.hb")
};

const readerTemplate = loadTemplate("Reader.hb");
const readerPartials = {
  object: loadTemplate("ObjectReader.hb"),
  enum: loadTemplate("EnumReader.hb")
};

const readerImplTemplate = loadTemplate("ReaderImpl.hb");
const readerImplPartials = {
  object: loadTemplate("ObjectReaderImpl.hb"),
  enum: loadTemplate("EnumReaderImpl.hb")
};

const writerTemplate = loadTemplate("Writer.hb");
const writerPartials = {
  object: loadTemplate("ObjectWriter.hb"),
  enum: loadTemplate("EnumWriter.hb")
};

const writerImplTemplate = loadTemplate("WriterImpl.hb");
const writerImplPartials = {
  object: loadTemplate("ObjectWriterImpl.hb"),
  enum: loadTemplate("EnumWriterImpl.hb")
};

function generateModel(rootSchema, options) {
  const { outputDir, namespace, extension } = options;
  const baseName = rootSchema.baseName;
  const output = modelTemplate({
    namespace: namespace + (extension ? `::EXT_${extension}` : ""),
    root: rootSchema,
    apiName: namespace.toUpperCase(),
    extension: extension,
    headers: lodash.uniq(lodash.flatten(
      rootSchema.refs.map((ref) => ref.modelHeaders)
    ))
  }, {partials: modelPartials, allowProtoPropertiesByDefault: true});

  const headerOutputDir = path.join(outputDir, "include", namespace);
  fs.mkdirSync(headerOutputDir, { recursive: true });
  const headerOutputPath = path.join(headerOutputDir, `${baseName}.h`);
  fs.writeFileSync(headerOutputPath, output, "utf-8");
}

function generateReader(rootSchema, options) {
  const { outputDir, namespace, extension } = options;
  const baseName = rootSchema.baseName;

  const output = readerTemplate({
    namespace: namespace + (extension ? `::EXT_${extension}` : ""),
    root: rootSchema,
    apiName: `${namespace}Reader`.toUpperCase(),
    headers: lodash.uniq(lodash.flatten(
      rootSchema.refs
        .map((ref) => ref.readerHeaders)
        .concat([`<${namespace}/${baseName}.h>`])
    ))
  }, {partials: readerPartials, allowProtoPropertiesByDefault: true});

  const readerOutputDir = path.join(outputDir, "include", namespace);
  fs.mkdirSync(readerOutputDir, { recursive: true });
  const readerOutputPath = path.join(readerOutputDir, `${baseName}Reader.h`);
  fs.writeFileSync(readerOutputPath, output, "utf-8");
}

function generateReaderImpl(rootSchema, options) {
  const { outputDir, namespace, extension } = options;
  const baseName = rootSchema.baseName;

  const output = readerImplTemplate({
    namespace: namespace + (extension ? `::EXT_${extension}` : ""),
    root: rootSchema,
    headers: [`<${namespace}/${baseName}Reader.h>`]
  }, {partials: readerImplPartials, allowProtoPropertiesByDefault: true});

  const readerOutputDir = path.join(outputDir, "src");
  fs.mkdirSync(readerOutputDir, { recursive: true });
  const readerOutputPath = path.join(readerOutputDir, `${baseName}Reader.cpp`);
  fs.writeFileSync(readerOutputPath, output, "utf-8");
}

function generateWriter(rootSchema, options) {
  const { outputDir, namespace, extension } = options;
  const baseName = rootSchema.baseName;

  const output = writerTemplate({
    namespace: namespace + (extension ? `::EXT_${extension}` : ""),
    root: rootSchema,
    headers: [
      `<${namespace}/${baseName}.h>`,
      '<CesiumJsonWriter/JsonWriter.h>',
      '<CesiumJsonWriter/ExtensionWriterContext.h>'
    ]
  }, {partials: writerPartials, allowProtoPropertiesByDefault: true});

  const writerOutputDir = path.join(outputDir, "include", namespace);
  fs.mkdirSync(writerOutputDir, { recursive: true });
  const writerOutputPath = path.join(writerOutputDir, `${baseName}Writer.h`);
  fs.writeFileSync(writerOutputPath, output, "utf-8");
}

function generateWriterImpl(rootSchema, options) {
  const { outputDir, namespace, extension } = options;
  const baseName = rootSchema.baseName;

  const output = writerImplTemplate({
    namespace: namespace + (extension ? `::EXT_${extension}` : ""),
    root: rootSchema,
    headers: [`<${namespace}/${baseName}Writer.h>`]
  }, {partials: writerImplPartials, allowProtoPropertiesByDefault: true});

  const writerOutputDir = path.join(outputDir, "src");
  fs.mkdirSync(writerOutputDir, { recursive: true });
  const writerOutputPath = path.join(writerOutputDir, `${baseName}Writer.cpp`);
  fs.writeFileSync(writerOutputPath, output, "utf-8");
}

function generate(options, rootSchemaName, ...schemaPaths) {
  const schemaCache = new SchemaCache(schemaPaths);

  const ctx = new reslib.Context(schemaCache, options.overrides);
  const result = ctx.resolve(rootSchemaName);
  result.extension = options.extension;

  generateModel(result, options);
  generateReader(result, options);
  generateReaderImpl(result, options);
  generateWriter(result, options);
  generateWriterImpl(result, options);

  return [];
}

module.exports = generate;
