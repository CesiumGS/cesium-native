# C++ Style Guide

This is a guide to writing C++ code for cesium-native. These guidelines are intended to help us write better C++ code by:

* Guiding us toward less surprising, less buggy, higher performing code.
* Helping us all be more productive by standardizing all the arbitrary little decisions around naming and code formatting.

In all cases these are _guidelines_. There are sometimes good reasons to violate the advice given here. In particular, when writing code that is meant to integrate into another system, it is rarely a good idea to fight that other system's conventions.

**cesium-native uses ISO Standard C++17.**

We follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), with a few exceptions. We suggest proceeding as follows:

* Skim the sections below, which call out where (and why) our practices differ from the C++ Core Guidelines.
* Skim the C++ Core Guidelines. Read sections that seem interesting or surprising.
* Read the sections below more thoroughly.

## 💄 Source Code Formatting

A change to C++ Core Guidelines [NL.17](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#nl17-use-kr-derived-layout).

Use `clang-format` and the `.clang-format` configuration in the root of this repo. Easy. If you're using Visual Studio Code with the C/C++ extension installed, just run the "Format Document" command.

To format the source code from the command line, install node.js from [https://nodejs.org/](https://nodejs.org/), and run `npm install` in the project root directory.
Then, in order to apply the formatting, run `npm run format`. 

We believe that source code should be readable and even beautiful, but that automated formatting is more important than either concern. Especially when readability is largely a matter of what we're used to, and beauty is mostly subjective. For our JavaScript / TypeScript code, we format our code with the ubiquitous [Prettier](https://prettier.io/) tool. In C++, clang-format is widely used but there's no de facto style, and so we've created a clang-format style that tracks as close to Prettier as possible. We will be able to get closer once this [clang-format patch](https://reviews.llvm.org/D33029) is merged.

## 📛 Naming

A change to C++ Core Guidelines [NL.10](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rl-camel).

Our naming differs from the naming proposed in the C++ Core Guidelines, primarily for consistency with our JavaScript and TypeScript code bases, but also because we don't like typing underscores as much as Bjarne does. Our conventions are:

* DO name classes, structs, and enums with `PascalCase`.
* DO name methods, functions, fields, local variables, and parameters with `lowerCamelCase`.
* DO name macros with `SCREAMING_SNAKE_CASE`.
* DO name methods and functions with verb-like names. Functions should be named for the actions they perform, like `generateTextureCoordinates` or `getParent`, or named for the question they answer, like `contains`.
* DO name classes, structs, enums, variables, fields, and parameters with noun-like names, like `Tileset`, `parent` or `boundingVolume`.
* DON'T prefix classes with `C`.
* DO prefix pure interfaces - a class with most methods virtual and no fields - with `I`.
* DON'T use Hungarian notation, except that it's useful to prefix pointer-like variables with a `p`, e.g. `pThing`. This is justified because it's not meant to convey type information (not so useful), but rather as an easily-understood abbreviation for the descriptive name of what the variable actually is (very useful). The variable isn't a `thing`, it's a pointer to a thing, but `pointerToThing` would get annoying quickly. (see [NL.5](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#nl5-avoid-encoding-type-information-in-names))
* DO prefix private fields with `_`, as in `_boundingVolume`.

## 💂‍♀️ Include Guards

A change to C++ Core Guidelines [SF.8](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sf8-use-include-guards-for-all-h-files).

Use `#pragma once` at the top of header files rather than manual inclusion guards. Even though `#pragma once` is not technically ISO Standard C++, it _is_ supported everywhere and manual inclusion guards are really tedious. If platform-specific differences in `#pragma once` behavior are changing the meaning of our program, we may need to reconsider some other life choices (like dodgy use of symlinks), but our choice to use `#pragma once` likely remains sound.

## 🛑 Exceptions

A change to C++ Core Guidelines [I.10](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-except) and [NR.3](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rnr-no-exceptions).

cesium-native may be used in environments with exceptions disabled, such as in Web Assembly builds. Thus, we should not rely on exceptions for correct behavior. Some guidelines:

* Consistent with the C++ Core Guidelines, we should write exception-safe code. Nothing should break or leak when some other code throws an exception. Use RAII to handle resource cleanup.
* Don't throw exceptions as a result of input data that is bad in common and predictable ways. Loading a glTF should never throw, no matter how broken the glTF is. Instead, the possibility of bad data should be incorporated into the normal, non-exceptional glTF loading API.
* Don't allow third-party code to throw in expected use-cases, because that might cause immediate program termination in some contexts. In rare cases this might require changing a third-party library or selecting a different one.
* Report improper API usage and precondition violations with `assert` rather than by throwing exceptions. In CesiumJS, these kinds of checks would throw `DeveloperError` and would be removed from release builds. `assert` is a more elegant way to do much the same. The C++ Core Guidelines ([I.6](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i6-prefer-expects-for-expressing-preconditions) and [I.8](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i8-prefer-ensures-for-expressing-postconditions)) suggest using the `Expects` and `Ensures` macros from the Guidelines Support Library instead of `assert`, but we suggest sticking with the more standard `assert` for the time being.
* Don't cause buffer overruns or other memory corruption. If it's not possible to continue safely, throwing an exception can be ok. When exceptions are disabled, throwing an exception will cause immediate termination of the program, which is better than memory corruption.

## ✨ Const by-value parameters

Not covered by the C++ Core Guidelines.

Whether a by-value parameter is `const`, e.g. `void someFunction(int foo)` versus `void someFunction(const int foo)` does not affect how that function is called in any way. From the standpoint of overload resolution, overriding, and linking, these two functions are _identical_. In fact, it is perfectly valid to _declare_ a function with a non-const value parameter and _define_ it with a const value parameter. Therefore we follow the advice suggested in https://abseil.io/tips/109:

> 1. Never use top-level const on function parameters in declarations that are not definitions (and be careful not to copy/paste a meaningless const). It is meaningless and ignored by the compiler, it is visual noise, and it could mislead readers.
> 2. Do use top-level const on function parameters in definitions at your (or your team’s) discretion. You might follow the same rationale as you would for when to declare a function-local variable const.

## 🎱 Use UTF-8 Everywhere

Not covered by C++ Core Guidelines.

We use UTF-8 everywhere, including on Windows where UTF-16 is the more common approach to unicode. This philosophy and the practicalities of adopting it are very well described by [UTF-8 Everywhere](https://utf8everywhere.org/). In short:

* Use `std::string` and `char*` everywhere. Mostly forget that `std::wstring` and `wchar_t` exist; you don't need them.
* Don't assume one element of a string or char array represents one character. The definition of a unicode "character" is ambiguous and usually doesn't matter, anyway. When we're using UTF-8, `std::string::size` and `strlen` return the number of UTF-8 code units, which is the same as the number of bytes.
* On Windows, when using Win32 and similar APIs, we must convert UTF-8 strings to UTF-16 and then call the wide character version of the system API (e.g. CreateFileW). Do this at the call site.
* Be careful when using the `fstream` API family on Windows. Make sure you read and understand [How to do text on Windows](https://utf8everywhere.org/#windows).

## 📚 Resources

* [Effective Modern C++](https://www.amazon.com/Effective-Modern-Specific-Ways-Improve/dp/1491903996) - Highly recommended reading to improve your use of the new features in C++11 and C++14.
