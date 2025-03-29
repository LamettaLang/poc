Lametta is a C/C++ like language. If something is not documents it's probably because I took it from C/C++ without thinking 🤷

## Preamble

### Memory - Fixed and dynamic sized types

To make certain things easier, Lametta differentiates between fixed and dynamic sized types.

Dynamic sized types are anything that might require reallocation of memory, like lists and other containers, as well as type ambiguity that would usually be represented through a pointer, like any. Fixed sized types are primitives, arrays and compositions thereof, where the size is always known.

This is mostly relevant for reinterpreting memory. There it helps to avoid read/write out of bounds and additionally it helps to avoid reinterpreting any pointers as primitives, by disabling the reinterpretation operation view_as on dynamic sized types.

### Standard Library

A standard library is planned, but not subject of this document.

### Concurency and Parallelism

Concurency using threads and async/await is planned for a future version.

### Compiler and Tooling

Tooling is not subject of this document.

## Types

### Logic type

Booleans. You know them. Typename is bool, value is true of false, numeric for false is 0, what's not false is true.

### Numeric types

Built in type names should be short and precise. uint8_t is ugly, and unsigned int is very verbose. These primitive types in Lametta have very short names. They are, as expected, fixed sized.

* u8 / i8 = unsigned / signed byte
* u16 / i16 = unsigned / signed 16-bit integer
* u32 / i32 = unsigned / signed 32-bit integer
* u64 / i64 = unsigned / signed 64-bit integer
* u128 / i128 = unsigned / signed 128-bit integer
* uint / int = unsigned / signed integer
* f16 = IEEE-754 16-bit binary
* f32 = IEEE-754 32-bit binary
* f64 = IEEE-754 64-bit binary
* f128 = IEEE-754 128-bit binary
* float = IEEE-754 binary
* c16/c32/c64/c128 = real+imaginary pair of fN values
* complex = real+imaginary pair of float values

Not specifying a width most likely equates to "i dont want to think about this now" and thus uses the widest version (e.g. uint == u128).
However when this documentation omits widths in an explanation, it means that any sensable width should be supported.

### Address

The address type is a special numeric type that does not convert to other
numeric types, while other numeric types can convert to an address for the purpose of arithmetic operations like addition. Addresses have a platform dependend but fixed size. This means that structures can compile with different sizes based on the pointer size of a platform or architecture.

The main purpose of the address type is to interface with native libraries and for hardware abstraction (e.g. hardware registers that are mapped at spcific addresses).

### Text types

Strings are utf encoded, exact encoding depending on the storage type. Conversion functions exist to widen and narrow between utf8/utf16/utf32.
Strings are stored as unsigned integer arrays or vectors of u8, u16 or u32, corresponding to their encoding. There is no grapheme handling at this point, but a method map can handle this defficiency.

While string literals are fixed size, a string variable is dynamic sized. You can cast a string variable to an array if you need to.

The typename for strings is `str`, and behaves mostly like a method map on a character type array, while still allowing array operations without removing the method map. Note that indexing a string will give you the nth codepoint, and not utf units, as u32.

### Any / Auto

Any is a special type that stores, well, any type. This requires runtime type information (RTTI).
Auto is a compile time deduced type, that can not change after asigning.

While any is always dynamic sized, the sized-ness of an auto variable depends on the defered type.

### Optional / Expect

Optionals are written as `T?` and expects are written as `T!`.

Optionals can have a value of type T or be none. This means that `T? == none` is only true if there is no value present.
The prefered way of checking whether an optional has a value is to let it coerce to a boolean (`if t`), but if you have a bool?, you need to check manually.

Expect on the other hand either holds a value T or an error. It will coerce to true if it has a value and false if it stores an error.
To check for a value explicitly, you have to cast it to an optional, as this cast is guaranteed to not crash (e.g. `cast<bool?>(expectBool) == none`).

An error can be assigned to an expect though `fail` (e.g. `int! value = fail('No value set')`). To retrieve the message of a failed value, you can simply cast it to a string.

Retrieving a value can be done explicitly or implicitly by casting it to the non-optional/non-expect type.
If no value is present, this will crash unless you cast between expect and optional. An empty optional will `fail()` when casting to an expect, and a failed expect will turn `none` if cast to an optional.

As might be obvious, there are no error types, as the idea of failing is, that the application is in an unrecoverable state. This also means that there are no error classes. Handling a failed value is intended to allow for resource management before failing further up the call chain until the application exits. It ultimately comes down to preference whether you use optional and dont provide a message, or expect and give a reason. E.g. a div by 0 or failing to open a file should be checked beforehand and thus might fail(), while getting a value from a map of an unset key might prefer an optional.

For truly unrecoverable errors, you can use the `crash()` built in, that will terminate the application with an optional message, without returning.

Since optionals are fixed sized if their value type is fixed sized; expect values are always dynamic sized.

### Handles

Handle types are opaque values that are used internally to map to object instances. These are intended to hide memory for libraries.
You can not perform arithmetic operations on handles and the byte size is not guaranteed, but fixed (Meaning it will be consistent for the platform you compile on and not turn your structs into dynamic sized objects).

Handles are ref counted, with every value copy. The ref count will go down for every Handle going out of scope, and the resource will be freed once all Handles to it are closed or their ref count reaches zero. A handle can also be deleted, invalidating it for every user. It is discouraged to delete handles manually, but might be required for edgecases. To prevent crashes from accessing invalid handle, you can use the built in `isValid(ref Handle hdl)->(bool valid)`.

Handles can also be copied, in cases where you want to completely hand off responsibility to a different part of the application. For this, you can use the built in `cloneHandle(ref Handle hdl)->(move Handle clone)`.

### Containers

Containers are default data structures for your convenience. Contained types are denoted in triangle brackets.
Containers are always dynamic sized and can not be used with `view_as`.
Sets, lists and vectors can be cast back and forth to arrays if needed. Performing such a cast will copy all elements.
Sets, lists, vectors, dicts and tuples  have a length and are iterable. With exception to sets they can be subscripted using `var[key]`.
Declarations for containers support omitting the element types, in which case the element type will be defered as auto, or be any. If you are not surewhat type a value has, you can use `is` or `as` to test.

* set\<T>
* list\<T> (linked list)
* vec\<T> (array list)
* dict\<K,V> (unordered hash map with K keys and V values)
* variant\<...> (holds one of the specified types)
* tuple\<...> (holds a fixed series of values of specified types, indexable like an array)

### Arrays

Arrays are collections of fixed amount of values. Arrays are fixed sized if the element type is fixed and dynamic sized if the element type is dynamic. If left uninitialized, best effort is taken to zero out memory.

### Structs

You know what a struct is. Structs are always packed always in system byte order. There is no padding. Structures exist as fixed size and dynamic size.
If a structure contains a container or any other dynamic sized element, it is marked dynamic itself. Memory reinterpretation with view_as\<R> is disabled for dynamic structures, because they contain indirections; you have to use cast\<R> instead.

The type is declared with `struct {}` where members are declared within the curly braces. To give the struct type a name, create a type alias.
Members are declared without initializers, and default to 0-bytes. C-like bitfield widths are not supported.

### Tuples

Tuples are similar to structs in that they hold a list of values of potentially different type. The main difference to structs is, that values in tuples are not refered to by name, but by index instead.

While tuples follow the same layout rules as structs, inheriting fixed size proertied from it's element types, it behaves like a sequence container otherwise, having a length and being subscriptable.

### (Method maps)

Method maps are a reference type wrapper to a base value that provide convenient methods where the referenced value is accessible as this.
You can imagine similar to a `template<class T> class interface {}` where member functions take an explicit `T& self` first argument.
See the method map section for more information. Operators in Lametta can not be overloaded.

## Operators

### Basic Maths

`+ - * / %`

### Bitwise Operators

`~ ^ & | << >> >>>`

### Logic Operators

`! && || == != < > <= >=`

### String operators

`+` - Concatenate, implicitly casts to string

`*` - Requires one side to be a positive integer, repeats the string n times.

### Others

`=` - Assignment

The assignment operator can destruct a tuple of a signle value into the value itself for convenience, if the left hand is not auto or any, and the right hand is a tuple with a compatible element type.

`.` - Member Access

Access an element in a namespace, struct or methodmap.

`#` - Length

This is a unary prefix operator that counts the elements of a container or the length of an array.

### Index and Range

`'[' INDEX ']'` - Single index access
`'[' START ':' END ']'` - Range access
`'[' START ':' END ':' STEP ']'` - Range access with step

This has three variants. The one you are probably most familiar with, using a single index for the range, getting
a ref to the element at the specified index. Using two numeric values, separated by a colon, you specify a
[ start, end ) rangeof indices to extract. Adding a third numeric value after another colon, specifies the stride
taking every nth element after (including) the start element.

### Streaming

`ITERABLE '::' '(' MEM_MOD NAME ')' EXPRESSION` - ForEach

For each operator, applying each element in a container to the expression with the specified type. Doubles as element wise transformation.
This results in a new iterable, with equal length, containing the expression results in the same order.
If the expression is a function call that does not return, the result is an empty collection.
For modifiers, see [Arguments](#Arguments).

`ITERABLE '::?' '(' MEM_MOD NAME ')' EXPRESSION` - Filter

Filters the elements in the iterable, retaining only elements for which the expression returns a truthy value.
The result is a new iterable of potentially less elements.

`ITERABLE ::_` - Flatten

Flat map the container on the left to a vector, unpacking one layer of indexing.
If the elements are not iterable, they are left untouched.
This results in another interable, if an element is a map, it's spread into a vector of [key, value] pairs.

`ITERABLE '::[' EXPRESSION ']'` - Group

The grouping operator performs the inverse of a flat, making a vector of vectors of size specified by the expression.
If the amount of elements is not divisible by the number, the last element is a vector of inputSize modulo amount.
This returns a nested iterable.

### Pipe

`INPUT '|>' TARGET` - Pipe

Pipes the evaluated left hand side as arguments to the right hand side. If the right hand is a callable, left hand is tried to
be passed as arguments; otherwise an assignment is tried. Right hand has to be a function or variable.
The resulting value of this expression is the return value of the called function or the variable as ref.

### Block statements

`'{' STATEMENT... '}'` - Statement group

I heard you like statements, so we put statements in your statement to make a bigger statement.
If not used for a fuction body, upon leaving, this takes on
the value of the last statement executed inside.

## Declarations

Lametta is type left. Type declarations can never share the same name, variables can be over-shadowed with function context.

### Attributes

Attributes are compiler specific additional pieces of information you can prepend to a declaration, and are formatted as follows:

`'@' NAME ( '(' ( VALUE ) ')' )`

The name for the attribute have to be `\p{L}[\p{L}_]*`, the brackets and value are optional. The content within value is arbitrary and defined by the compiler.
In order for a compiler to be able to ignore unsupported attributes, the value still has to be parsed with basic book keeping of parens and quotes to know when the value terminates (e.g. `@weirdValue( a(")") )` should parse with key `weirdValue` and value `a(")")`). This includes backslash escapes `\"` and `\\`, ignoring any other backslash escape.
If an attribute is nor supported it is to be ignored silently.

There can be any amount of attributes, separated by space.

### Values

For basic types, declarations look like this:

`TYPE NAME ('=' INITIAL_VALUE)`

Names starts with any `\p{L}` character and continues with `[^\s]`.

Initializers are optional, if no initializer is specified, the memory is zero-initialized.

#### Arrays

`T '[' SIZE ']'`

An array of SIZE elements of type T. Arrays of arrays are possible. If initialized, the Initializer has to have no more than SIZE elements, the last element of the initializer is repeated to fill the remaining elements. If no initializer value is given, the elements are initialized to zero, or empty containers.

#### Vectors

`'vec' '<' T '>' '{' (VALUE (',' VALUE ...)) '}'`
`'vec' '{' VALUE (',' VALUE ...) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the first element.
All values have to be implicitly convertible to the element type.

#### List

`'list' '<' T '>'{' (VALUE (',' VALUE ...)) '}'`
`'list' '{' VALUE (',' VALUE ...) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the first element.
All values have to be implicitly convertible to the element type.

#### Set

`'set' '<' T '>' '{' (VALUE (',' VALUE ...)) '}'`
`'set' '{' VALUE (',' VALUE ...) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the first element.
All values have to be implicitly convertible to the element type.

#### Dict

`'dict' '<' K ',' V '>' '{' (KEY '=' VALUE (',' KEY '=' VALUE ...)) '}'`
`'dict' '{' KEY '=' VALUE (',' KEY '=' VALUE ...) '}'`

No type has to be specified if at least one entry is given. In this case the key and value types are equal to the types of the first entry.
All entries have to be implicitly convertible to the entrie's key and value type.

#### Struct

`STUCTTYPE '{' (KEY '=' VALUE (',' KEY '=' VALUE ...)) '}'`

Structs are initialized similarly to dicts, every member of the struct has to be present in the initializer.
STRUCTTYPE needs to be an alias definition (Constructs like `struct{int a}{a=1}` are _not_ valid).

#### Tuples

`'tuple' '<' (T (',' T ...)) '>' '{' VALUE (',' VALUE ...) }`
`'tuple' '{' VALUE (',' VALUE ...) }`

Tuples are a series of value whose types have to follow the series of types T in the given order.
The amount of values has to match the amount of types, and empty tuples are possible.
If no value types are not specified, they are defered from the value types.

Tuples are the only containers that can be spread into function arguments as well, using elipsis.
The amount and type of values in the tuple have to match the amount and type of input arguments of the function it is spread into.
Alternatively you can pipe a tuple into a function.

### Functions

Functions are declared like this:

`'fun' ( NAME ) '(' INARGS ')' ( '->' '(' OUTARGS ')' ) BODY`

Functions can only access INARGS and return OUTARGS. OUTARGS with the surrounding syntax are optional, if there are no out args. Functions are anonymous, if NAME is omitted. If the body is a single statement, it does not need curlies (see statements).

If named with a `def` statement, body and name can not be used as in the statement above. This creats a function type, usefull for example for callback parameters. While you can get the type of a named function, you can not reassign a named function.

For assigning or passing functions, the function type has to be named with a type alias, and can then be assigned to a named or anonymous function like `FUNC_TYPE VAR_NAME '=' ( FUNC_NAME | ANONYMOUS_FUNCTION )`. Anonymous function act as expressions and can also be written inline as function arguments, if desired.

The result of a function call is always tuple of all output arguments. The assignment operator can destruct a tuple of a signle value into the value itself for convenience.

Function overloading is not supported, and have to have distinct names.

#### Arguments

An argument consists of `( MEM_MOD ) TYPE NAME`
Memory modifiers can be one of copy, ref, const, move. These should be pretty self explanatory, move is to change ownership between caller and callee, default is copy. Important to not might be, that a move in argument pulls the memory ownership into the function, making it inaccessible for the caller. If the caller tries to access such a variable after it has been moved away, the application will crash. Moving memory out differs little to copy out, as the move happens at the return point.
If an argument by name appreas in the input and output list, it has to have the same type and the memory modifier has to be copy or reference.
Passing a method map by value will create a hidden value type copy, that is then newly referenced by the method map type, passing a method map by value will avoid this.
Optional arguments have to be at the end of the input argument list and will be `none` if omitted.
Placing an elipsis after the last input argument creates a vararg function, collecting all remaining arguments given to the function into a tuple of values of the specified type. E.g. in a function `fun x(copy any values ...)` values are accessibla as if declared with `tuple<any,...> value` where the amount of type parameters for the tuple equals the amount of values passed.
Optionals and varargs can not be mixed.

### Type aliasing

The syntax for type aliasing is `'def' NAME 'as' TYPE`. This is the only way to structs and methodmaps a name or to create function types.

### Method maps

`'methodmap' ( '<' INHERITED_MMAP ( ',' INHERITED_MMAP ... ) )`

Method maps are reference types, this means that they do not hold any storage in memory themselfes, but instead bind to a value type by reference. This reference is then available as `this` within member functions.
Methodmaps support inheritence. Other member functions, if visible according to the hierarchy, are collapsed onto `this`, overriding inherited implementations, to form a singular namespace. In case on name collisions between data members and function names, functions take precedence. Because method maps are only a reference wrapper around a different type, they can always be implicitly converted back to the underlying type. Because there is no function overloading, member functions are only distinguished and overridden by name.

Since method maps are bound to storage types by the developer, a mechanism needs to be in place to limit and discriminate the type of `this` from within a member function. This can be done with `is` and `as` using pattern matching in members and the binder.

The binder function is analogous to a constructor but handles the case where a methodmap is bound to a storage type via `bind<methodmap>(storageInstance)`. To avoid typing the name of the method map multiple times, the function is just called `bind`, analogous to `new` or `constructor` functions in other languages. A methodmap can not be bound unless it has a bind function. It takes a storage type value by reference as first argument and has to assign this or `fail`. Reaching the end of the bind function without assigning this or failing is an error. Most of the time the implementation will be a single pattern match (See [pattern matching](#Pattern-Matching) below). e.g.:
```rust
def methodmap_type as methodmap {
  bind(ref any storage)->() with
    match storage as storage_type value : this = value
    match * : fail("Called bind on methodmap_type with unsupported storage type")
}
```

Member functions of a method map have an implicit this, initialized in `bind`. Also note that `view_as` from and to the method map type is only valid inside members and binding has to be used otherwise. This is to ensure that the method map only operates on value types it knows how to handle.

For method maps that do extra work during binding, like connecting to a database, there is a second special member function, `unbind`. This is functionally equivalent to a destructor and will be called when the reference count of the method map reaches zero.

### Visibilities

Functions, structs, method maps and method map members can have different visibility modifiers, as prefix keywords:

* `export` - this symbol is also visible to every project depending on this project
* `public` - default, project wide visibility
* `protected` - only visible to this method map and it's inheritors (only in method maps)
* `private` - only visible to this file / the implementing method map

Specifying a visibility on a method map changes where the method map can be used. For example, an exported function can not have a public, protected or private method map as argument, because the caller can not access the type.

## Statements and Expressions

Statements terminate with a linebreak or semicolon. A linebreak is not terminating a statement if the line ends with an operator or the following line starts with one.

Compound statements are one or more statements within curly braces.

The semicolon does not only terminate statements, but, if no statement preceeds to be terminated, also doubles as NOP (like `pass` in python).

Branching/conditionals are in general separated by "then-statements" through a colon, or a linebreak. If the following statement is a compound statement, the explicit separator becomes optional.
In the following sections, this conditionally optional sepratator is shortened with `sep`.

For all loops, `break` and `continue` work as usual.

### If

`'if' EXPRESSION sep STATEMENT ( 'else' sep STATEMENT )`

As with c, one of the fundamentals is the if statement, that evaluates the first statement if the expression resolves true, or the second statement otherwise.

### For

`'for' TYPE NAME 'in' RANGE sep STATEMENT`
`'for' TYPE NAME 'in' ITERABLE sep STATEMENT`

Runs the statement of every value in an iterable or range. For ranges, the syntax is mathematical, meaning `[]` denote inclusive bounds and `()` or `][` denote exclusive bounds. Within bounds markers, you can put two or three expressions, evaluating to initial value, increment value and last value. Default increment is 1, the last value can be '...' for "infinite" sequences, separator is the comma.

e.g. `for auto i in [1, ...[`, `for auto i in [0, 0.1, 1.0)`

### Do, While, Until

`'while' CONDITION sep STATEMENT`
`'until' CONDITION sep STATEMENT`
`'do' sep STATEMENT sep2 'while' CONDITION`
`'do' sep STATEMENT sep2 'until' CONDITION`

Execute the statement in a loop while condition holds true. The until form inverts the condition by default. sep2 is similar to sep, but peeks backwards for a block statement instead of forward.

### Pattern Matching

Imagine c/c++ switch-case on steroids if you've never heard of pattern matching before. Also as expression, not a statement.

`'with' ( EXPRESSION 'as' NAME )` starts the match statement. Lines that follow, starting with 'match' implement the match cases.
Declaring an expression with name is optional if you match against variables from the parent context.

Match cases can span multiple lines and are not standalone statements but have to be terminated like one. Match cases are valid until a line does not match the syntax anymore. If specified, the result of the with-expression is available as if assigned with `const auto NAME = EXPRESSION`.

The result value of a match block is the value of the expression for the fist case that matched in declaration order. Matches never fall-through and only support expressions as values (nothing with compound statements, call a function if you want to do more complex stuff).

* `'match' EXPRESSION sep EXPRESSION` - the most basic form, checks for equality, if the expression does not contain `it`, checks true if it does.
* `'match' RANGE sep EXPRESSION` - checks if the with-expression (if numeric) falls withing the range. The range syntax is equivalent to the one in for-loops, but without step size.
* `'match' '*' sep EXPRESSION` - default case, has to be last.

Examples:

```rust
with strToInt("42") as it
    match it % 2 == 0 : print("Number is even")
    match [50, 100] : print("value is between 50 and 100")
    match * :;

any something
with
    match something as uint number && number == 42 :
        print("value is 42")
    match * :
        print("value is something else")

float? optNum = ...
float value = with
    match optNum && !isNaN(optNum): optNum
    match *: 0
```

### Return

Has no arguments and jumps to the end of a function. All return values have to be assigned before the function exits or the application is malformed.

### Multiple files

`'include' PATH_OR_LIBRARY ( 'as' NAMESPACE ) ( ATTRIBUTES )`

Writing big projects in single files is ugly. Split your code across multiple files and share "public" and "external" symbols by including them in you main file. PATH_OR_LIBRARY has to be a string literal representing a file system path, that should not leave the project root directory (but you do you), or the name of a library. It's the dependency resolvers job to return the correct set of files from PATH_OR_LIBRARY if it doesn't point to a file.

If the namespace is not given, all symbols exported from the library are imported into you project namespace. If a namespace is specified, the symbols are moved to `NAMESPACE::SYMBOL`.

ATTRIBUTES can be used by the depdencency resolver to specify more information like the version. It's recommended to support at least the attributes `@version("version spec")` and `@source("source spec")`.

It is recommended to strongly prefer SemVer, and to only use exact versions, as developers often don't follow SemVer and any change in version might break your code. If a version can not be resolved, compilation shall fail. If no version is specified the compiler toolchain can try to get the latest version.

Specifing a source for the library could mean a git repo, group and artifact name (like java does it), just a plain name like less capable dependency systems use, or literally anything else. If the source is unknown or not supported, compilation shall fail. If no source is specified the compiler toolchain can try to hit a default remote.

Further mechanisms for depdenency management and control, like handling lockfiles, caching depdendency files, etc. are the responsibility of the dependency resolver.

Structure and constraints for dependencies are responsibility of the compilation toolchain. Library name, version and namespace can not contain spaces or characters illegal in filesystem paths.

Imported symbols always have public visibility (project wide, not transitive).

To ensure compatibility between compilers and resolvers the following contract should be followed. The dependency resolver should expect to be invoked with arguments as follows: Attributes in the form @attribName("attribValue") are passed as --attribName attribValue, PATH_OR_LIBRARY is passed as is. Assume we want to include a theoretical openssl library at version 3.4.0 from a github repository. The include line would look like this: `include openssl @version("3.4.0") @source("https://github.com/openssl/openssl")`. The arguments to your dependency resolver would be `["openssl", "--version", "3.4.0", "--source", "https://github.com/openssl/openssl"]`. The dependency resolver should return a set of files with Lametta exports (one per line) and exit code 0 in case the dependency was optionally fetched and can be provided. Otherwise it should return a non-0 exit code and give a short explanation for the compiler to throw at the developer.

### Dependency management

Dependency management is done with project files in many other languages, but using a certain library is integral to keeping the code working. For this reason, dependency management is part of the language. Python is almost there with the option to invoke pip if a dependency is missing and then late loading it. Because Lametta is a compiled langauge, we can not late load dependencies, but we can still give the compiler extensive specs for libraries we use.

`'project'`

This is a keyword that does nothing on it's own, but can host various @attributes describing project metadata:

`'@build(' NAME VERSION ( 'application' | 'staticlib' | 'dynamiclib' ) ')'`

Matching the import attribute, projects can define a name and version using this statement, in order for the compilation toolchain to put the resulting artifacts in the correct place for other projects to find them again (if desired). The default type, if not specified, is application. Name and version can not contain spaces or characters illegal in file system paths.

### Reinterpreting Memory

`'view_as' '<' T '>' '(' r ')'`

view_as will reinterpret a value of type R as if it was of type T. The constraint for this to be valid is, that both R and T are statically sized value types, or that T is a method map and view_as is called in a member of T.

### Converting Types

`'cast' '<' T '>' '(' r ')'`

Cast converts a value of type R to and expect of type T. This will, in most cases invoke a custom conversion function `fun cast(ref R r)->(move T! t)` that can deal with statically and dynamically sized types and method maps, possibly failing if e.g. a value range is exceeded.

### Binding Types

`'bind' '<' T '>' '(' ( NAME | ADDRESS ) ')'`

If the value is of type address and T represents a statically sized type, this will create a reference variable of type T that is pointing to the given address. Be careful as taking such a variable by value will copy the current memory (if readable) away from the bound address.

When T is a method map type, the bind will try to call the `bind`-function of the method map to construct an return the result. If the value to be bound is not accepted by the method map's `bind`-function, compilation will fail.

In any other case, bind is invalid.

### is and as

`r 'is' T` and `r 'as' T t`

The keywords `is` and `as` can be used to check if the type of an instance r matches the specified type T. In case of mehtod maps the check is also valid for base method maps to e.g. check for the existance of interfaces. The difference between `is` and `as` is, that `as` will give you a reference T to r. This will not bind, cast or view_as, but can convert `any` to a concrete type.

For the purpose of compound statements, t shall be available immediately after the check. (e.g. `r as uint t && t > 100` is valid syntax and behaves as expected).

### typeof

`'typeof' THING`

Returns a `Type` methodmap that allows you to inspect various aspects of a type. Check the reference implementation for what you can expect from this interface.
For a struct this includes things like fix sized, name and members. The member list might be obfuscated or inaccurate.
