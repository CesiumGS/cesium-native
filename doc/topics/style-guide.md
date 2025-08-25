# C++ Style Guide         {#style-guide}

This is a guide to writing C++ code for cesium-native. These guidelines are intended to help us write better C++ code by:

* Guiding us toward less surprising, less buggy, higher performing code.
* Helping us all be more productive by standardizing all the arbitrary little decisions around naming and code formatting.

In all cases these are _guidelines_. There are sometimes good reasons to violate the advice given here. In particular, when writing code that is meant to integrate into another system, it is rarely a good idea to fight that other system's conventions.

**cesium-native uses ISO Standard C++20.**

We follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), with a few exceptions. We suggest proceeding as follows:

* Skim the sections below, which call out where (and why) our practices differ from the C++ Core Guidelines.
* Skim the C++ Core Guidelines. Read sections that seem interesting or surprising.
* Read the sections below more thoroughly.
<!--! [TOC] -->

## 💄 Source Code Formatting

A change to C++ Core Guidelines [NL.17](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#nl17-use-kr-derived-layout).

Use `clang-format` and the `.clang-format` configuration in the root of this repo. Easy. If you're using Visual Studio Code with the C/C++ extension installed, just run the "Format Document" command.

To format the source code from the command line, install node.js from [https://nodejs.org/](https://nodejs.org/), and run `npm install` in the project root directory.
Then, in order to apply the formatting, run `npm run format`.

We believe that source code should be readable and even beautiful, but that automated formatting is more important than either concern. Especially when readability is largely a matter of what we're used to, and beauty is mostly subjective. For our JavaScript / TypeScript code, we format our code with the ubiquitous [Prettier](https://prettier.io/) tool. In C++, clang-format is widely used but there's no de facto style, and so we've created a clang-format style that tracks as close to Prettier as possible.

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
* DO prefix optional values with `maybe`.
* DO prefix expected values with `expected`.
* DO prefix private fields with `_`, as in `_boundingVolume`.

## 💂‍♀️ Include Guards

A change to C++ Core Guidelines [SF.8](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sf8-use-include-guards-for-all-h-files).

Use `#pragma once` at the top of header files rather than manual inclusion guards. Even though `#pragma once` is not technically ISO Standard C++, it _is_ supported everywhere and manual inclusion guards are really tedious. If platform-specific differences in `#pragma once` behavior are changing the meaning of our program, we may need to reconsider some other life choices (like dodgy use of symlinks), but our choice to use `#pragma once` likely remains sound.

## 📄 Forward Declarations

Not covered by C++ Core Guidelines.

* Forward declare types in our own libraries whenever you can. Only `#include` when you must.
* For third-party libraries, prefer to `#include` a fwd.h type of file if one is provided. `#include` the full implementation only when you must.
* If you find yourself writing complicated forward declarations for our own types or for third-party ones that don't include a fwd.h, consider making a fwd.h file for it.

## 🛑 Exceptions

A change to C++ Core Guidelines [I.10](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-except) and [NR.3](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rnr-no-exceptions).

cesium-native may be used in environments with exceptions disabled, such as in Web Assembly builds. Thus, we should not rely on exceptions for correct behavior. Some guidelines:

* Consistent with the C++ Core Guidelines, we should write exception-safe code. Nothing should break or leak when some other code throws an exception. Use RAII to handle resource cleanup.
* Don't throw exceptions as a result of input data that is bad in common and predictable ways. Loading a glTF should never throw, no matter how broken the glTF is. Instead, the possibility of bad data should be incorporated into the normal, non-exceptional glTF loading API.
* Don't allow third-party code to throw in expected use-cases, because that might cause immediate program termination in some contexts. In rare cases this might require changing a third-party library or selecting a different one.
* Report improper API usage and precondition violations with `CESIUM_ASSERT` rather than by throwing exceptions. In CesiumJS, these kinds of checks would throw `DeveloperError` and would be removed from release builds. `CESIUM_ASSERT` is a more elegant way to do much the same. The C++ Core Guidelines ([I.6](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i6-prefer-expects-for-expressing-preconditions) and [I.8](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i8-prefer-ensures-for-expressing-postconditions)) suggest using the `Expects` and `Ensures` macros from the Guidelines Support Library instead, but we suggest sticking with `CESIUM_ASSERT` for the time being.
* Don't cause buffer overruns or other memory corruption. If it's not possible to continue safely, throwing an exception can be ok. When exceptions are disabled, throwing an exception will cause immediate termination of the program, which is better than memory corruption.
* Functions should be declared `noexcept`.

## 🏷️ Run-Time Type Information

Not covered by C++ Core Guidelines.

cesium-native may be used in environments with RTTI disabled. Thus, use of RTTI, including `dynamic_cast`, is disallowed.

## ✨ Const

A change to C++ Core Guidelines [Con.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconst-immutable) and [Con.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#con4-use-const-to-define-objects-with-values-that-do-not-change-after-construction).

In general we follow the advice suggested in https://quuxplusone.github.io/blog/2022/01/23/dont-const-all-the-things/:

* By default, make member functions `const` ([Con.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#con2-by-default-make-member-functions-const)).
* By default, make pointers and references `const` ([Con.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#con3-by-default-pass-pointers-and-references-to-consts)).
* Don't use `const` for local variables.
* Don't use `const` for by-value parameters.
* Don't use `const` for return types.
* Don't use `const` for data members ([C.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-constref)).

## 🔀 Auto

A change to C++ Core Guidelines [ES.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-auto).

Avoid using `auto` except in the following cases where writing the full type is burdensome or impossible:

* Iterators
* Lambdas
* Views
* Structured bindings
* When the type is on the right hand side, e.g. casts

## 🤔 Optional and Expected

Not covered by the C++ Core Guidelines.

* Use `->` and `*` instead of `value()` to access the contained value.
* Use the boolean operator instead of `has_value()`.

## 🔢 Integers

Partly covered by C++ Core Guidelines.

* By default, use signed types ([ES.106](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-nonnegative)).
* By default, use fixed width integer types, e.g. `int32_t` instead of `int`.
* Use constructor notation for very safe integer conversions (where the range is checked) rather than `static_cast`.
* Use a safe form of index checking when accessing elements in standard library containers:

```cpp
if (meshId >= 0 && size_t(meshId) < model.meshes.size())
```

## 🔧 Utility Functions

Not covered by C++ Core Guidelines.

* Utility functions that are only in-use in their particular file should exist in an anonymous namespace.
* Utility functions that provide common functionality should be static member functions of a struct. This is preferred over a nested namespace. For example, see [GltfUtilities.h](../CesiumGltfContent/include/CesiumGltfContent/GltfUtilities.h).

## 🎱 Use UTF-8 Everywhere

Not covered by C++ Core Guidelines.

We use UTF-8 everywhere, including on Windows where UTF-16 is the more common approach to unicode. This philosophy and the practicalities of adopting it are very well described by [UTF-8 Everywhere](https://utf8everywhere.org/). In short:

* Use `std::string` and `char*` everywhere. Mostly forget that `std::wstring` and `wchar_t` exist; you don't need them.
* Don't assume one element of a string or char array represents one character. The definition of a unicode "character" is ambiguous and usually doesn't matter, anyway. When we're using UTF-8, `std::string::size` and `strlen` return the number of UTF-8 code units, which is the same as the number of bytes.
* On Windows, when using Win32 and similar APIs, we must convert UTF-8 strings to UTF-16 and then call the wide character version of the system API (e.g. CreateFileW). Do this at the call site.
* Be careful when using the `fstream` API family on Windows. Make sure you read and understand [How to do text on Windows](https://utf8everywhere.org/#windows).

## ✅ Testing

Not covered by C++ Core Guidelines.

* Test files should be prefixed with `Test`, e.g. `TestModel` instead of `ModelTests`.

## ✏️ Documentation

Not covered by C++ Core Guidelines.

Our documentation is generated by Doxygen before being published [online](https://cesium.com/learn/cesium-native/ref-doc). Therefore, any public API should be documented with Doxygen-compatible comments that make use of the following tags (in the listed order):

- `@brief`: Summarize the purpose of the class, function, etc.
- `@remarks`: Use for any side comments, e.g., how invalid inputs or special cases are handled.
- `@warning`: Convey any warnings about the use of the class, function, etc., e.g., invalid values that make it unsafe.
- `@tparam`: Describe template parameters.
- `@param`: Denote and describe function parameters.
- `@returns`: Describe the return value of a non-`void` function.
- `@throws`: Describe any exceptions that a function throws.

Additionally, make sure to:

- Put comments **above** the thing being documented (instead of inline).
- Use the `/** ... */` comment style (instead of `///`).
- Use `@ref` when referencing other classes, functions, etc. in the Cesium Native API.
- Use proper capitalization, grammar, and spelling.
- Use back apostrophes (`) to encapsulate references to types and variable names.
- Use triple back apostrophes (```) to encapsulate any snippets of math or code.

You can optionally use `@snippet` to include examples of how functions are used in other parts of the code (usually test cases). First, surround the target code with comments containing a descriptive name:

```cpp
// In TestPlane.cpp...

TEST_CASE("Plane constructor from normal and distance") {
  //! [constructor-normal-distance]
  // The plane x=0
  Plane plane(glm::dvec3(1.0, 0.0, 0.0), 0.0);
  //! [constructor-normal-distance]
}
```

Then reference the file and the name of the snippet:

```cpp
/**
 * @brief Constructs a new plane from a normal and a distance from the origin.
 *
 * Example:
 * @snippet TestPlane.cpp constructor-normal-distance
 */
Plane(const glm::dvec3& normal, double distance);
```

If you find that comments are duplicated across multiple classes, functions, etc., then:

- Keep the comment on the most sensible instance, e.g., a base class or a pure virtual function.
- Use `@copydoc` for the others.

For `@copydoc`, keep the comment to one line instead of adding the usual linebreak after `/**`.

```cpp
/** @copydoc Foo */
struct Bar { ... }
```

> Although private classes and functions aren't required to have the same level of documentation, it never hurts to add any, especially if they have non-obvious assumptions, scope, or consequences.

## 🗂️ Other

Not covered by C++ Core Guidelines.

* Use `this->` when accessing member variables and functions. This helps distinguish member variables from local variables and member functions from anonymous namespace functions.
* Static function definitions in a .cpp file should be preceded by `/*static*/`.
* Unused parameter names in callbacks should be commented out rather than omitted. Only use `[[maybe_unused]]` when you can't comment out parameter names, such as when parameters are conditionally accessed by `constexpr` logic.
* Use `[i]` instead of `.at(i)` for sequence containers like `std::vector` and `std::array`.

## 📚 Resources

* [Effective Modern C++](https://www.amazon.com/Effective-Modern-Specific-Ways-Improve/dp/1491903996) - Highly recommended reading to improve your use of the new features in C++11 and C++14.
