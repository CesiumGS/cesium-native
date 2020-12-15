# C++ Style Guide

This is a guide to writing C++ code for cesium-native. These guidelines fall into two main categories:

* Guidelines that help us write faster, less surprising, less buggy, and overall _better_ code.
* Guidelines that don't really matter except that consistency helps us all be more productive.

In all cases these are _guidelines_. There are sometimes good reasons to violate the advice given here. In particular, when writing code that is meant to integrate into another system, it is rarely a good idea to fight that other system's conventions.

## Source Code Formatting

Use `clang-format` and the `.clang-format` configuration in the root of this repo. Easy. If you're using Visual Studio Code with the C/C++ extension installed, just run the "Format Document" command.

We believe that source code should be readable and even beautiful, but that automated formatting is more important than either concern. Especially when readability is largely a matter of what we're used to, and beauty is mostly subjective. For our JavaScript / TypeScript code, we format our code with the ubiquitous [Prettier](https://prettier.io/) tool. In C++, clang-format is widely used but there's no de facto style, and so we've created a clang-format style that tracks as close to Prettier as possible. We will be able to get closer once this [clang-format patch](https://reviews.llvm.org/D33029) is merged.

## Coding Guidelines

cesium-native uses ISO Standard C++17.

We follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - it's worth reading in its entirety - with the following exceptions:

### Naming

Where the naming guidelines in the C++ Core Guidelines linked above differ from the guidelines below, the guidelines below take precedence.

* DO name classes, structs, and enums with `PascalCase`.
* DO name methods, functions, fields, local variables, and parameters with `lowerCamelCase`.
* DO name macros with `UPPER_SNAKE_CASE`.
* DO name methods and functions with verb-like names, like `generatTextureCoordinates` or `getParent`.
* DO name classes, structs, enums, variables, fields, and parameters with noun-like names, like `parent` or `boundingVolume`.
* DON'T prefix classes with `C`.
* DO prefix pure interfaces - a class with most methods virtual and no fields - with `I`.
* DON'T use Hungarian notation, except that it's useful to prefix variables of pointer type with a `p`, e.g. `pThing`. This is justified because it's not meant to convey type information (not so useful), but rather as an easily-understood abbreviation for the descriptive name of what the variable actually is (very useful). The variable isn't a `thing`, it's a pointer to a thing, but `pointerToThing` would get annoying quickly.
* DO prefix private fields with `_`.
* DON'T use public fields, except in value-like classes where all of the following are true:
  * The class has no class invariants. _All_ possible values for all fields are valid.
  * All fields are public.
  * The type is a `struct`.
* DO use a `class` rather than a `struct`, except for the value-like classes described above.

### Include guards

Use `#pragma once` at the top of header files rather than manual inclusion guards. Even though `#pragma once` is not technically ISO Standard C++, it _is_ supported everywhere and manual inclusion guards are really tedious.

### Exceptions

cesium-native may be used in environments with exceptions disabled, such as in Web Assembly builds. Thus, we should not rely on exceptions for correct behavior. Some guidelines:

* Consistent with the C++ Core Guidelines, we should write exception-safe code. Nothing should break or leak when some other code throws an exception. Use RAII to handle resource cleanup.
* Don't throw exceptions as a result of input data that is bad in common and predictable ways. Loading a glTF should never throw, no matter how broken the glTF is. Instead, the possibility of bad data should be incorporated into the normal, non-exceptional glTF loading API.
* Report improper API usage and precondition violations with `assert` rather than by throwing exceptions. In CesiumJS, these kinds of checks would throw `DeveloperError` and would be removed from release builds. `assert` is a more elegant way to do much the same.
* Don't cause buffer overruns or other memory corruption. If it's not possible to continue safely, throwing an exception can be ok. When exceptions are disabled, throwing an exception will cause immediate termination of the program, which is better than memory corruption.

## Resources

