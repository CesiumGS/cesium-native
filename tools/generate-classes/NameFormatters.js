const qualifiedTypeNameRegex = /(?:(?<namespace>.+)::)?(?<name>[^<]+)/;

const NameFormatters = {
  getReaderNamespace: function getReaderNamespace(namespace, readerNamespace) {
    if (namespace === "CesiumUtility") {
      return "CesiumJsonReader";
    }
    return readerNamespace;
  },

  getName: function getName(name, namespace) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces && pieces.groups && pieces.groups.namespace) {
      if (pieces.groups.namespace === namespace) {
        return `${namespace}::${pieces.groups.name}`;
      } else {
        return `${pieces.groups.namespace}::${pieces.groups.name}`;
      }
    } else {
      return `${namespace}::${name}`;
    }
  },

  getIncludeFromName: function getIncludeFromName(name, namespace) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces && pieces.groups && pieces.groups.namespace) {
      if (pieces.groups.namespace === namespace) {
        return `<${namespace}/${pieces.groups.name}.h>`;
      } else {
        return `<${pieces.groups.namespace}/${pieces.groups.name}.h>`;
      }
    } else {
      return `<${namespace}/${name}.h>`;
    }
  },

  getJsonHandlerIncludeFromName: function getJsonHandlerIncludeFromName(
    name,
    readerNamespace
  ) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces && pieces.groups && pieces.groups.namespace) {
      const namespace = NameFormatters.getReaderNamespace(
        pieces.groups.namespace,
        readerNamespace
      );
      if (namespace === readerNamespace) {
        return `"${pieces.groups.name}JsonHandler.h"`;
      } else {
        return `<${namespace}/${pieces.groups.name}JsonHandler.h>`;
      }
    } else {
      return `"${name}JsonHandler.h"`;
    }
  },

  getJsonHandlerName: function getJsonHandlerName(name, readerNamespace) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces && pieces.groups && pieces.groups.namespace) {
      const namespace = NameFormatters.getReaderNamespace(
        pieces.groups.namespace,
        readerNamespace
      );
      return `${namespace}::${pieces.groups.name}JsonHandler`;
    } else {
      return `${name}JsonHandler`;
    }
  },

  getWriterName: function getWriterName(name) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces) {
      return pieces.groups.name;
    } else {
      return name;
    }
  },

  removeNamespace: function removeNamespace(name) {
    const pieces = name.match(qualifiedTypeNameRegex);
    if (pieces && pieces.groups && pieces.groups.namespace) {
      return pieces.groups.name;
    } else {
      return name;
    }
  },
};

module.exports = NameFormatters;
