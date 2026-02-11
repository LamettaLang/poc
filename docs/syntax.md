## Preamble

### Memory - Fixed and dynamic sized types

Lametta differentiates between fixed and dynamic sized types.

Fixed sized types are primitives, arrays and compositions thereof, where the size is known at compile time.
Dynamic sized types are anything that requires runtime allocation of memory, including containers, strings, and any.

This is mostly relevant for reinterpreting memory. There it helps to avoid read/write out of bounds and additionally it
helps to avoid reinterpreting any pointers as primitives, by disabling the reinterpretation operation view_as on dynamic
sized types.

### General notes

Comments in Lametta use C-Syntax with // prefix for line comments and /* */ for block comments.

Statements are in terminated by a semicolon. An alternative use for the semicolon is a NOP for empty statements. Control flow logic does not require paranthesis. The body of a conditional, loop or similar *always* requires a block statement. 

```
// examples for separators:
if condition { single_statement; }
// or
if condition {
   block_statement
}
// incorrect:
if condition
  single_statement;  //<- error, { expected after condition
```

### Standard Library

A standard library is planned, but not subject of this document. A small set of builtins can be found
at [Required builtins](#required-builtins)

### Concurency and Parallelism

Asynchronous functions are annotated with `@async`, and have to be awaited at the call site with `@await call()`. The call might need to be parenthesized for clarity. 

Additionally there's simple threading support through `thread(thread_main, thread_state)->(thread)`. This copies the thread context object into a new thread and passes the context as first argument to the thread function. The returned thread object is a handle representing the thread with methods to join and detach the thread.  Upon returning from the thread main, the return value is passed back to the parent thread and can be retrieved through the handle. 

### Compiler and Tooling

Tooling is not subject of this document. The idea is, however, to have a strong coupling with an external tool that does dependency resolution. This allows enterprise environments to roll their own dependency management if needed. 

## Types

Lametta is explicitly types. Every function or variable has to have a defined type. While there are defered types,
you still need to explicitly defer type with a keyword.

The syntax used in the following puts tokens in single quotes, parenthesis mean a part is optional,
elipsis denote repetition and unquoted words are placeholders.

### Generics

'<' ( GENERIC ( ':' CONSTRAINT ( '&'  ONSTRAINT ...) ) ( ',' GENERIC ':' CONSTRAINT ( '&'  ONSTRAINT ...) ...)  '>'

Some types support generics (see below). These follow the same general syntax as other languages. The element type(s) are placed after the container type name or function name, and can be constrained using interfaces (see below for custom interfaces).
Specifying generics when instantiating a strict or methodmap or when calling functions has to be done after the label. When possible the value type of a generic is derived by the compiler.
Built-in constraints include:
* FixedSized
* DynamicSized
* Signed
* Unsigned
* Integer
* Float
* Complex
* Iterable<V> - iteration over elements of type V is possible
* Castable<V> - can be cast to type V
* Viewable<V> - can be viewed as type V
* Composed<V> - all members of V packed into this struct or methodmap/interface containing all methods in V.
* Extends<V> - methodmap has V as a base methodmap.

Example: `Type<T: DynamicSized & Iterable<N>, N>`

### Logic type

Booleans. You know them. Typename is bool, value is true of false, numeric for false is 0, what's not false is true.

### Numeric types

Built in type names should be short and precise. uint8_t is ugly, and unsigned int is very verbose. These primitive
types in Lametta have very short names. They are, as expected, fixed sized.

* u8 / i8 = unsigned / signed byte
* u16 / i16 = unsigned / signed 16-bit integer
* u32 / i32 = unsigned / signed 32-bit integer
* u64 / i64 = unsigned / signed 64-bit integer
* u128 / i128 = unsigned / signed 128-bit integer
* uint / int = unsigned / signed integer
* f32 = IEEE-754 32-bit binary
* f64 = IEEE-754 64-bit binary
* f128 = IEEE-754 128-bit binary
* float = IEEE-754 binary
* c32/c64/c128 = real+imaginary pair of fN values
* complex = real+imaginary pair of float values

Not specifying a width most likely equates to "i dont want to think about this now" and thus uses the widest version (
e.g. uint == u128).
However when this documentation omits widths in an explanation, it means that any sensable width should be supported.

### Address

The address type is a special numeric type that does not convert to other
numeric types, while other numeric types can convert to an address for the purpose of arithmetic operations like
addition. Addresses have a platform dependend but fixed size. This means that structures can compile with different
sizes based on the pointer size of a platform or architecture.

The main purpose of the address type is to interface with native libraries and for hardware abstraction (e.g. hardware
registers that are mapped at spcific addresses).

An address has to be bound to a different type to dereference it. See `bind<T>()` later.

### Text types

Strings are utf encoded, exact encoding depending on the storage type. Conversion functions exist to widen and narrow
between utf8/utf16/utf32.
Strings are stored as unsigned integer arrays or vectors of u8, u16 or u32, corresponding to their encoding. There is no
grapheme handling at this point, but a method map can handle this defficiency.

While string literals are fixed size, a string variable is dynamic sized. You can cast string variables to and from
arrays of utf code units (u8, u16, u32) at any time.

The typename for strings is `str`, and is essentially a character type array, that allows array operations on a
codepoint level. Indexing a string will address the nth codepoint as u32, and not utf units. De-/encoding to the
strings character representation is handled for you. This automatic encoding is what makes it dynamic sized.

### Enums

Enums are named collections of literal values of the same type. These can be open or closed. For open enums, values that
are not explicitly listed can still be assigned to enum types, like in C/C++. This can be useful for example for bitflag
definitions, where combinations of flags are perfectly valid. Closed enums only accept values that are explicitly listed
with the definition. A classic enumeration like "payment methods", where algorithms can not work with values outside the
defined range would fit closed enums. Enum values have to be specified in their scoped form (e.g. the banana value of a
fruits enum would be written as `fruits.banana`). All enum values have to be fixed sized.

An enum can implicitly view_as value type and value types can explicitly view_as enum types. This allows for greater
compatibility with functions that don't take the enum type as parameter. To check whether a value type would be a valid
enum value, `value is enum`-Syntax can be used. Converting to and from the enum value to it's identifier as string can
be done using a string cast (`cast<str>(enum.value)`).

### Any / Auto

`any` is a special type that stores, well, any type. This requires runtime type information (RTTI).
`auto` is not really a type, but a keyword that is a shorthand for type deduction. That means writing `auto` instructs
the compiler to figure figure out a type based on the context. An auto deduced type can not change afterwards, unlike any.

While `any` is always dynamic sized, the sized-ness of an `auto` variable depends on the defered type.

### Optional

Optionals are written as `T?`.

Optionals can have a value of type T or be none. This means that `T? == none` is only true if there is no value present.
The prefered way of checking whether an optional has a value is to let it coerce to a boolean (`if t`), but if you have
a bool?, you need to check manually.

Retrieving a value can be done explicitly or implicitly by casting it to the non-optional type.
If no value is present, this will crash unless you cast to an expect `T!`. An empty optional will result in an error
when casting to an expect.

In some cases you might want to default the optional to a usable value. While you can do that using normal control flow,
you can use the short hand `or`, that takes a `T!` or `T?` left and a `T` right.

```rust
Type? optional = ...
// explicit
Type value = { if !optional: fallback; else: optional; };
// short hand:
Type value = optional or fallback;
```

Optionals are fixed sized if their value type is fixed sized, but not neccessarily the same size.

### Errors and Expect

#### Errors

Errors are represented though method maps similar to the class hirarchy of exceptions in other languages. An error
consists of at least error class and an error message. The base method map needs to be inherited from `Error`, as in
`MyErrorType is Error`.

`Error` is a method map that looks like this:

```rust
def Error as methodmap {
  protected fun new(const str? message) {/*...*/}
  /** @brief returns the given message or a fallback */
  export fun message()->(str message) {/*...*/}
  /** @brief spews stacktrace to stdout */
  export fun stacktrace() {/*...*/}
  /** @brief set context information */
  protected fun putContext(const str name, any value) {/*...*/}
  /** @brief retrieve context information
   * To be wrapped by derived methodmaps to make available context visible
   */
  protected fun context(const str name)->(const any? value) {/*...*/}
};

// example
def FileError as methodmap of Error {
  public fun new(const str file) {
    this = view_as<FileError>(fail("File Error: " + file));
    putContext("file", file);
  }
  export fun file()->(const str file):
    file = context("file");
}
```

#### Expect

Expects are written as `T!`.

Expect holds a value T or an error. It behaves identical to `variant<T, Error>`, but is easier to type. As union type,
use a type check to test for success.

To fail an expect, assign an error (e.g. `int! value = ValueError('No value given')`, see binding types). To check
for a value explicitly, type check it with is/as or cast it to an optional (e.g. `expectBool is bool`,
`cast<bool?>(expectBool)` or `cast<Error?>(expectBool)`).

Retrieving a value or error can be done explicitly or implicitly by casting it to the corresponding type.
If the cast fails the application will crash, unless you cast to an optional.
In that case a failed expect will turn `none` if cast to an optional value type.

Expects are fixed sized if their value type is fixed sized, but not neccessarily the same size.

### Handles

Handle types are opaque values that are used internally to map to object instances. These are intended to hide memory
for libraries.
You can not perform arithmetic operations on handles and the byte size is not guaranteed, but fixed (Meaning it will be
consistent for the platform you compile on and not turn your structs into dynamic sized objects).

Handles are ref counted, with every value copy. The ref count will go down for every Handle going out of scope, and the
resource will be freed once all Handles to it are closed or their ref count reaches zero. A handle can also be deleted,
invalidating it for every user. It is discouraged to delete handles manually, but might be required for edgecases. To
prevent crashes from accessing invalid handle, you can use the built in `isValid(ref Handle hdl)->(bool valid)`.

### Containers

Containers are default data structures for your convenience. Contained types are denoted in triangle brackets.
Containers are always dynamic sized and can not be used with `view_as`.
Sets, lists and vectors can be cast back and forth to arrays if needed. Performing such a cast will copy all elements.
Sets, lists, vectors, dicts and tuples have a length and are iterable. With exception to sets they can be subscripted
using `var[key]`.
Element types of containers can be omitted, in which case the element type will be defered by the compiler.
If you are not sure what type a value or element has, you can use `is` or `as` to test.

* set\<T>
* list\<T> (linked list)
* vec\<T> (dynamic array)
* dict\<K,V> (unordered hash map with K keys and V values)
* variant\<...> (holds one of the specified types)
* tuple\<...> (holds a fixed series of values of specified types, indexable like an array)

### Arrays

Arrays are collections of fixed amount of values. Arrays are fixed sized if the element type is fixed and dynamic sized
if the element type is dynamic. Arrays are zero-initialized by default. The type of an array of type `T` with size
`size` would be written as `T[size]`, where size has to be a known integer at compile time.

### Structs

You know what a struct is. Structs are always packed always in system byte order. There is no padding. Structures exist
as fixed size and dynamic size.
If a structure contains a container or any other dynamic sized element, it is marked dynamic itself. Memory
reinterpretation with `view_as<R>` is disabled for dynamic structures, because they contain indirections; you have to
use `cast<R>` instead.

The type is declared with `struct {}` where members are declared within the curly braces. To give the struct type a
name, create a type alias.
Members are declared without initializers, and default to 0-bytes. C-like bitfield widths are not supported.

Unnamed fields are composed into the types memory layout. Decomposition can be done by labeling or slicing the object with view_as.

### Variant

Variant is a type union, the value can only have one of the specified types. Most useful if named, to limit the accepted
types of an argument ahead, making exhaustive pattern matching easier, by removing the need for a default case with any.

### Tuples

Tuples are similar to structs in that they hold a list of values of potentially different type. The main difference to
structs is, that values in tuples are not refered to by name, but by index instead.

While tuples follow the same layout rules as structs, inheriting fixed size proertied from it's element types, it
behaves like a sequence container otherwise, having a length and being subscriptable.

### (Method maps)

Method maps are a reference type wrapper to a base value that provide convenient methods where the referenced value
is accessible as this. You can imagine it as a `template<class T> class interface {}` where member functions take an
explicit `T& self` first argument.
See the method map section for more information. Operators in Lametta can not be overloaded.

## Operators

### Basic Maths

`+ - * / %`

### Bitwise Operators

`~ ^ & | << >> >>>`

### Logic Operators

`! && || == != < > <= >=`

You can chain the `<` and `<=` operators as a shortcut to denote range checks. `l <= x < h` is equal to `(l <= x) && (x < h)`.

### String operators

`+` - Concatenate, implicitly casts to string

`*` - Requires one side to be a positive integer, repeats the string n times.

### Others

`=` - Assignment

The assignment operator can destruct tuples, if they contain a single value, for convenience, if the left hand of the
assignment is not auto or any typed, and the right hand has a compatible element type. It can also box/unbox
optionals and expect values. The left and right hand type have to be a pair of `T`, `T?` and `T!`.
If an empty optional or failed expect is unboxed into a literal T, the assignment crashes.

Similarly assignments can be made from and to variants as long as the variant supports the value type.

An assignment operator can also move data between vectors, lists and arrays, giving important utility when converting
between the two. Assigning to an array requires, that the vector/list has the same length as the array. In all other
case the vector/list will be resized accordingly.

`.` - Member Access

Access an element in a namespace, struct, methodmap or enum.

`#` - Length

This is a unary prefix operator that counts the elements of a container or the length of an array or string.

### Index and Range

`'[' INDEX ']'` - Single index access
`'[' START ':' END ( ']' | '[' )` - Range access
`'[' START ':' STEP ':' END ( ']' | '[' )` - Range access with step

Variant 1:
Returns a reference to the element at the specified index.
Variant 2:
Using two numeric values, separated by a colon. This specifies a range of values from START to END. Use a ']' to
terminate the range for end inclusive, or '[' to make it end exclusive.
Variant 3:
Adding a third numeric value after another colon adds a stride, taking every nth element after (including) the start
element.

Step defaults to 1 for variant 2. Use variant 3 if end < start.
As a range, these act as generators you can iterate over. If used for indexing, the result is a copy-on-write view
of the container. You can also index start or end with a negative value, where it wraps around the length of the
iterable.

```rust
//examples:
[ 1 : 10 ]
[ 1 : 2 : #string [
[ #string-1 : -1 : 0 ]
string[ -5 : -1 ] == string[ #string-5 : #string [
```

### Streaming

These operators all have the same precedence and are evaluated left to right. The streaming operators are only
applicable to iterable collections and arrays.

`ITERABLE '::' TRANSFORMER` - ForEach

For each operator, applying each element in a container to the expression with the specified type. Doubles as element
wise transformation.
The transformer has to be a function that takes one value, an element of the iterable, and returns up to one value.
This results in a new iterable, with equal length, containing the expression results in the same order.
If the function has zero return values, the result is an empty collection.

The transformer has the form `fun(const T element)->(move R transformed)` or `fun(const T element, uint index)->(move R tranformed)` where T and R are pairs of key and value for dicts.

`ITERABLE '::?' FILTER` - Filter

Filters the elements in the iterable, retaining only elements for which the expression returns a truthy value.
The filter has to be a function that takes one value, an element of the iterable, and returns a boolean.
If the filter returns true for an element, it is kept for the resulting collection.
The result is a new iterable of potentially less elements.

The filter is of the form `fun(const T element)->(bool keep)` or `fun(const T element, uint index)->(bool keep)` where T are pairs of key and value for dicts.

`ITERABLE '::%' REDUCER` - Reduce

Reduce the iterable to a single value by iteratively applying a binary function. The reducer function takes two arguments: the current element and the current accumulator (which is none for the first iteration) and returns the new accumulator. The result of the reduce expression is the final accumulator (which is none if the iterable is empty). The reducer function must be defined for the element type and the accumulator type (which is optional of the return type).

The freducer is of the form `fun(const T element, R? accumulator)->(R accumulated)` or `fun(const T element, R? accumulator, uint index)->(R accumulated)` where T are pairs of key and value for dicts.

`ITERABLE ::_` - Flatten

Flat map the container on the left to a vector, unpacking one layer of indexing.
If the elements are not iterable, they are left untouched.
This results in another interable, if an element is a map, it's spread into a vector of [key, value] pairs.

`ITERABLE '::[' EXPRESSION ']'` - Group

The grouping operator performs the inverse of a flat, chunking to a vector of vectors of the size specified by the expression.
If the amount of elements is not divisible by the number, the last element is a vector of inputSize modulo amount.
The expression can not be a callable, and has to evaluate to a positive, non-zero integer.
This returns a nested iterable.

### Pipe

`INPUT '|>' TARGET` - Pipe

Pipes the evaluated left hand side as arguments to the right hand side. If the right hand is a callable, left hand is
tried to
be passed as arguments; otherwise an assignment is tried. Right hand has to be a function or variable.
The resulting value of this expression is the return value of the called function or the variable as ref.
If the input is a tuple, from a variable or as result of a function call, the tuple is automatically destructed into
input arguments of a target function on the right, if every element in order has a matching type in the same position
of the input argument list. If the destruction does not match the argument list, passing the tuple as single argument
is tried instead, the function has to take a single tuple argument of matching type in the later case.

### Block expression

`'{' STATEMENT... '}'` - Statement group

I heard you like statements, so we put statements in your statement to make a bigger statement.
Of course you can use blocks as function bodies or control flow structures like you're used to.
However, you can also use blocks as expressions. While a block expression still has the variable context of the parent, 
and can access members as usual, it will also evaluate to the last statement evaluated inside the block before returning.
Block statements do not require to be terminated by a semicolon, they implicitly end an expression. Statements inside, however, do.

```rust
// examples for blocks as expression:

bool hitchhiking = true;
i32 bestNumber = {
  if hitchhiking {
    42;
    return; // leaving here will evaluate the block to 42
  }
  69;
}

bool even = with {
  floor(rng()*100);
} as roll:
match roll % 2 {
  notifyPlayer();
  true;
}
match *: false;
```

## Declarations

Lametta is type left. Type declarations can never share the same name, variables can be over-shadowed with function
context. All variables have to be explicitly declared with a type.

```rust
// examples:
i32 number;
auto function = fun(i32 v): (v*v,);
any result = [number] :: function;
```

### Attributes

Attributes are compiler-specific metadata prepended to declarations, and are formatted as follows:

`'@' NAME ( '(' ( VALUE ) ')' )`

The name for the attribute have to be `\p{L}[\p{L}_]*`, the brackets and value are optional. The content within value is
arbitrary and defined by the compiler.
In order for a compiler to be able to ignore unsupported attributes, the value still has to be parsed with basic book
keeping of parens and quotes to know when the value terminates (e.g. `@weirdValue( a(")") )` should parse with key
`weirdValue` and value `a(")")`). This includes backslash escapes `\"` and `\\`, ignoring any other backslash escape.
If an attribute is nor supported it is to be ignored silently.

There can be any amount of attributes, separated by space.

The only attribute required to be supported so far is `@cast` for functions.

### Values

For basic types, declarations look like this:

`TYPE NAME ( '=' INITIAL_VALUE )`

Names start with any `\p{L}` character and continues with `[^\s]`.

Initializers are optional, if no initializer is specified, the memory is zero-initialized.

#### Enums

Definition of enum values as part of the type:

`'enum' '<' T '>' ( 'open' ) '{' VALUES '}'`

Decalres a namespace for enum literal values of type T.

Numeric values are implicitly initialized start at 0 and increment by one for each value in definiton order. If at least
one value is explicitly initialized, or the value type is not numeric, all values have to be explicitly initialized.

To give the enum namespace a usable alias, type aliasing has to be used. Otherwise compilation will fail for dangling
enum values.

Enums are closed by default, meaning only values defined at compile time are valid for the enum. All other value will
fail compilation. Open enums can accept any new value of type T at runtime, for example for bit flags. Use `open` to
declare an enum as open.

#### Arrays

Inline definition of an array instance with size derived from the amount of values:

`'[' VALUE ( ',' VALUE ... ) ( ',' ) ']'`

An array of SIZE elements of type T. Arrays of arrays are possible. If initialized, the Initializer has to have no more
than SIZE elements, the last element of the initializer is repeated to fill the remaining elements. If no initializer
value is given, the elements are initialized to zero, or empty containers.


#### Vectors

Inline definition of a vector instance of values of type T:

`'vec' '<' T '>' '{' ( VALUE ( ',' VALUE ... ) ) '}'` or
`'vec' '{' VALUE ( ',' VALUE ... ) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the
first element.
All values have to be implicitly convertible to the element type.

#### List

Inline definition of a list instance of values of type T:

`'list' '<' T '>'{' ( VALUE ( ',' VALUE ... ) ) '}'` or
`'list' '{' VALUE ( ',' VALUE ... ) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the
first element.
All values have to be implicitly convertible to the element type.

#### Set

Inline definition of a set instance of values of type T:

`'set' '<' T '>' '{' ( VALUE ( ',' VALUE ... ) ) '}'` or
`'set' '{' VALUE ( ',' VALUE ... ) '}'`

No type has to be specified if at least one element is given. In this case the element type is equal to the type of the
first element.
All values have to be implicitly convertible to the element type.

#### Dict

Inline definition of a dictionary instance of pairs of key type K and value type V:

`'dict' '<' K ',' V '>' '{' ( KEY '=' VALUE ( ',' KEY '=' VALUE ... ) ) '}'` or
`'dict' '{' KEY '=' VALUE ( ',' KEY '=' VALUE ... ) '}'` or
`'{' KEY '=' VALUE ( ',' KEY '=' VALUE ... ) ( ',' ) '}'`

No type has to be specified if at least one entry is given. In this case the key and value types are equal to the types
of the first entry.
All entries have to be implicitly convertible to the entrie's key and value type.

#### Struct

Inline definition of a struct instance with members KEY and their initializing values.

`STUCTTYPE ( GENERICS ) '{' ( KEY '=' VALUE ( ',' KEY '=' VALUE ... ) ) '}'`

Structs are initialized similarly to dicts, every member of the struct has to be present in the initializer.
STRUCTTYPE needs to be an alias definition (Constructs like `struct{int a}{a=1}` are _not_ valid).

#### Tuples

Inline definition of a tuple instance of values of types T1, T2, ...:

`'tuple' '<' ( T1 ( ',' T2 ... ) ) '>' '{' VALUE1 ( ',' VALUE2 ... ) }` or
`'tuple' '{' VALUE1 ( ',' VALUE2 ... ) }` or
`'(' VALUE1 ( ',' VALUE2 ... ) ( ',' ) ')'`

Tuples are a series of value whose types have to follow the series of types T in the given order.
The amount of values has to match the amount of types, and empty tuples are possible.
If no value types are not specified, they are defered from the value types.
The third form, using round paranthesis requires a trailing comma, if the tuple has just one element to
avoid confusion with grouping syntax.

Tuples are the only containers that can be spread into function arguments as well, using elipsis.
The amount and type of values in the tuple have to match the amount and type of input arguments of the function it is
spread into.
Alternatively you can pipe a tuple into a function.

### Functions

Functions are declared like this:

`'fun' ( NAME ) ( GENERICS ) '(' INARGS ')' ( '->' '(' OUTARGS ')' ) ':' BODY` - regular
`'fun' ( GENERICS ) '(' INARGS ')' ':' TUPLE` - lambda

Functions access only INARGS and must assign all OUTARGS before exiting. Specifying OUTARGS is optional, if there are no
values returned by the function. Functions are anonymous, if NAME is omitted.

Function overloading is not supported, and have to have distinct names.

If named with a `def` statement, body nor name can not be used as in the statement above. This creats a function type,
usefull for example for callback parameters. While you can get the type of a named function, you can not reassign a
named function.

For assigning or passing functions, the function type has to be named with a type alias, and can then be assigned to a
named or anonymous function like `FUNC_TYPE VAR_NAME '=' ( FUNC_NAME | ANONYMOUS_FUNCTION )`. Anonymous function act as
expressions and can also be written inline as function arguments, if desired.

The result of a function call is a tuple of all output arguments in order. For a function to be valid, it has to
explicitly assign every element in the output arguments list with a value.

The second syntax is lambda syntax.
Lambdas omit the explicit output argument list and instead immediately follow with a statement.
The returned value represents the output argument(s) and is subject to automatical boxing into a tuple. This means if you want
to actually return a tuple you have to double it like so: `fun (): tuple{tuple{42}}`.
Lambda functions are always anonymous and can not have names.

```rust
// function without results
fun sendMessage(str message) {/* some implementation */}
// function with result
fun square(int value)->(int squared, bool even) {
  squared = value * value;
  even = value % 2 == 0;
}
// can be written as
auto square = fun(int value): ( value * value, value % 2 == 0 );
```

#### Arguments

An argument consists of `( MEM_MOD ) TYPE NAME`. This applies to input and output arguments.
Memory modifiers can be one of copy, ref, const, move. These should be pretty self explanatory, move is to change
ownership between caller and callee, default is copy. Important to not might be, that a move in argument pulls the
memory ownership into the function, making it inaccessible for the caller. If the caller tries to access such a variable
after it has been moved away, the application will crash. Moving memory out differs little to copy out, as the move
happens at the return point.
If an argument by name appreas in the input and output list, it has to have the same type and the memory modifier has to
be copy or reference.
Passing a method map by value will create a hidden value type copy, that is then newly referenced by the method map
type, passing a method map by value will avoid this.
Optional arguments have to be at the end of the input argument list and will be `none` if omitted. They can not have
explicit default values. Use `or` when you read the value instead.
Placing an elipsis after the last input argument creates a vararg function, collecting all remaining arguments given to
the function into a tuple of values of the specified type. E.g. in a function `fun x(copy any values ...)` values are
accessibla as if declared with `tuple<any,...> value` where the amount of type parameters for the tuple equals the
amount of values passed.
Optionals and varargs can not be used both in the same signature.

### Type aliasing

The syntax for type aliasing is `'def' NAME 'as' TYPE`. This is the only way to give names to enums, structs and methodmaps or to
create function types.

```rust
// usage examples
def ename as enum<int> open { RED=1, GREEN=2, BLUE=3 };
def layout as struct { int x; int y; };
```

### Method maps

`'methodmap' ( GENERICS ) ( 'of' INHERITED_MMAP ( ',' INHERITED_MMAP ... ) ) ( 'binds' BIND_TYPE ( ',' BIND_TYPE ... ) )`

Method maps are reference types, this means that they do not hold any storage in memory themselfes, but instead bind to
a value type by reference. This reference is then available as `this` within member functions.

Important distinction to classes: You can not define data members on method maps! A method map is not a value type, like handles or structs, but a vehicle to attach methods to a value type.

Methodmaps support inheritence. Other member functions, if visible according to the hierarchy, are collapsed onto `this`, overriding inherited implementations, to form a singular namespace. In case of name collisions between data
members of bound type and function names of the method map, functions take precedence. Because method maps are only a reference wrapper around a
different type, they can always be converted back to the underlying type. As there is no function
overloading, member functions are only distinguished and overridden by name.

Since method maps are bound to storage types by the developer, a mechanism needs to be in place to limit and discriminate the type of `this` from within a member function. This can be done with `is` and `as` using pattern matching in members and the 'binds' declaration.

A MethodMap can be bound to a value using the `bind<methodmap>(storagevalue)`syntax. This works similar to the placement new constructor in C++ where no allocation takes place, and the Interface is simply layered over the
memory. However binding a methodmap this way does not execute any code.

Method maps support a constructor, which is a special method named `new`, called with `new T()`. Calling a constructor has to assign `this`.
If this is assigned to a value allocated within the constructor, ownership of the memory is passed back out to the caller of the constructor. Assigning this, even inside the constructor, has to be done using bind syntax. If no constructor is available, the method map can only be bound.

In case construction fails, you can always signal this by binding an Error to `this`. Note that assigning `this` from
within the method map is only ever allowed within the constructor. For the purpose of construction, you can imagine
the function signature as `fun new(ref MethodMapType! this, /*input arguments*/)->()`.

The constructor can be a single pattern match (See [pattern matching](#Pattern-Matching) below). e.g.:

```rust
def methodmap_type as methodmap {
  fun new(ref any storage): with:
    match storage as storage_type value : this = value;
    match * : this = ArgumentError("Called bind on methodmap_type with unsupported storage type");
}
def MyError as methodmap of Error binds Handle {
  fun new(const str message):
    this = bind<MyError>(fail(message));
}
auto error = new MyError("Oh no :(");
```

However you bind a methodmap, if `BIND_TYPE`s are specified, the bound value has to be of that type. `BIND_TYPE` also can not be another method map, it has to be a value type, although you can use generics for bind types. Not specifying a bind type is equal to `binds any`. 

Member functions of a method map have an implicit this, initialized in `bind`. Also note that `view_as` from method
map type to the storage type is only valid inside members and binding has to be used otherwise. This is to ensure
that the method map only operates on value types it knows how to handle.

For method maps that do extra work during construction, like connecting to a database, there is a second special member
function, `free`. This is functionally equivalent to a destructor and will be called when the reference count of the
method map reaches zero. The unbind signature is `free()->()`, taking no arguments and not returning, as it's called automatically.

### Visibilities & Qualifiers

Functions, structs, method maps and method map members can have different visibility modifiers, as prefix keywords:

* `export` - this symbol is also visible to every project depending on this project
* `public` - default, project wide visibility
* `protected` - only visible to this method map and it's inheritors (only in method maps)
* `private` - only visible to this file / the implementing method map
* `native` - function only qualifier, that marks it as native. This means that the function is not implemented
  in place, but a promise to the linker, that an implementation exists somewhere. A function marked native can not have an implementation in-place.

Specifying a visibility on a method map changes where the method map can be used. For example, an exported function can
not have a public, protected or private method map as argument, because the caller can not access the type.

Importing a dependency (not a project file) with an exported object treats it as "already implemented", so you don't
have to implement it again.

## Statements and Expressions

Statements terminate with a linebreak or semicolon. A linebreak is not terminating a statement if the line ends with an
operator or the following line starts with one.

Compound expressions are one or more statements within curly braces. Making blocks not statements allows for some
syntactical sugar.

The semicolon does not only terminate statements, but, if no statement preceeds to be terminated, also doubles as NOP (
like `pass` in python).

For all loops, `break` and `continue` work as usual.

### If

`'if' EXPRESSION ':' STATEMENT ( 'else' ':' STATEMENT )`

As with most other languages, one of the fundamentals is the if statement, that evaluates the first statement if the
expression resolves true, or the second statement otherwise.

If also works as expression, allowing it to replace ternary statement like in rust.

```rust
// example
int value = { if day == cold: 42; else: 30; }
```

### For

`'for' ( MEM_MOD ) TYPE NAME 'in' RANGE ':' STATEMENT`
`'for' ( MEM_MOD ) TYPE NAME 'in' ITERABLE ':' STATEMENT`

Runs the statement of every value in an iterable or range. As with functions, `MEM_MOD` is optional and defaults to
`copy`. For range syntax, read below.

### Do, While, Until

`'while' CONDITION ':' STATEMENT`
`'until' CONDITION ':' STATEMENT`
`'do' BLOCK_STATEMENT 'while' CONDITION`
`'do' BLOCK_STATEMENT 'until' CONDITION`

Execute the statement in a loop while condition holds true. The until form inverts the condition by default. Tail
controlled loops require a block statement for a body to disamiguate nested loops.

### Pattern Matching

Imagine c/c++ switch-case on steroids if you've never heard of pattern matching before. Also as expression, not a
statement.

`'with' ( EXPRESSION 'as' NAME ) ':'` starts the match statement. Lines that follow, starting with 'match' implement the
match cases.
Using 'EXPRESSION as NAME' is optional if you match against variables from the parent context. If you want to reuse the
result of a more complex expression for matching without adding to the parent context, you can use this form. Name is a single label storing the evaluated expression.

Match cases can span multiple lines and, while not full statements, have to be terminated like statements. Match cases
are valid until a statement does not match the syntax anymore. If specified, the result of the with-expression is available
as if assigned with `const auto NAME = EXPRESSION`.

The result value of a match block is the value of the expression for the fist case that matched in declaration order.
Matches never fall-through and only support expressions as values (nothing with compound statements, call a function if
you want to do more complex stuff).

* `'match' EXPRESSION ':' EXPRESSION` - The match expression has to be a comparison, that has to evaluate true for the case to hit
* `'match' '*' ':' EXPRESSION` - Default case, has to be last.

Examples:

```rust
with strToInt("42") as it:
    match it % 2 == 0 : print("Number is even");
    match 50 <= it <= 100 : print("value is between 50 and 100");
    match * :;

any something;
with:
    match something as uint number && number == 42
        print("value is 42");
    match *
        print("value is something else");

float? optNum = ...;
float value = with:
    match optNum is float && !isNaN(optNum): optNum;
    match *: 0;
```

### Return

`return` has no arguments and immediately exits a function. It serves only for control flow.
Do not however that all output arguments of a function, declared by the signature, have to be assigned before
the function exits, or the application is malformed.

### Multiple files

`'include' PATH_OR_LIBRARY ( 'as' NAMESPACE ) ( ATTRIBUTES )`

Writing big projects in single files is ugly. Split your code across multiple files and share "public" and "external"
symbols by including them in you main file. PATH_OR_LIBRARY has to be a string literal representing a file system path,
that should not leave the project root directory (but you do you), or the name of a library. It's the dependency
resolvers job to return the correct file from PATH_OR_LIBRARY if it doesn't point to a file.

If the namespace is not given, all symbols exported from the library are imported into you project namespace. If a
namespace is specified, the symbols are moved to `NAMESPACE.SYMBOL`, like a module.

### Dependency management

Dependency management is done with project files like in many other languages. The version and metadata of a dependency is managed outside the compiler, and referenced by name.

@attributes can be appended to dependencies for additional information. How this information is used is up to the compiler. One possible case could be conditional dependencues.

Further mechanisms for depdenency management and control, like handling lockfiles, caching depdendency files, etc. are
the responsibility of the dependency resolver.

Structure and constraints for dependencies are responsibility of the compilation toolchain. Library name and namespace can not contain spaces or characters illegal in filesystem paths. Since libraries can have groups, GROUP:NAME can be used for disambiguation between dependencies with the same name.

Imported symbols always have public visibility (project wide, not transitive).

To ensure compatibility between compilers and resolvers the following contract should be followed. The dependency
resolver should expect to be invoked with arguments as follows: Attributes in the form @attribName("attribValue") are
passed as `--@attribName attribValue`, PATH_OR_LIBRARY is passed as is. Assume we want to include a theoretical openssl
library at version 3.4.0+ because we use specific features of that version. The include line would look like this:
`include openssl @minversion("3.4.0")`. The arguments to your dependency
resolver would be `["--@minversion", "3.4.0", "openssl"]`. The dependency
resolver should return a set of files with Lametta exports (one per line) and exit code 0 in case the dependency was
optionally fetched and can be provided. Otherwise it should return a non-0 exit code and give a short explanation for
the compiler to throw at the developer.

### Reinterpreting Memory

`'view_as' '<' T '>' '(' r ')'`

view_as will reinterpret a value of type R as if it was of type T. The constraint for this to be valid is, that both R
and T are fixed sized value types. If view_as is called inside a method map member, the method map type is allowed for T
or this for R, giving members access to the underlying data.

### Converting Types

`'cast' '<' T '>' '(' r ')'`

Cast converts a value of type R to and expect of type T. This will, in most cases invoke a custom conversion function
`@cast fun cast(const R r)->(move T! t)` that can deal with fixed and dynamically sized types and method maps, possibly
failing if e.g. a value range is exceeded.

### Binding Types

`'bind' '<' T '>' '(' ( NAME | ADDRESS ) ')'`

If the value is of type address and T represents a fixed sized type, this will create a reference variable of type T
that is pointing to the given address. Be careful as taking such a variable by value will copy the current memory (if
readable) away from the bound address.

When T is a method map type, then bind will return a reference type with the interface overlayed on top of the storage
value, or fail, if the value type is not a valid bind type for the method map. The exptected `T!` returned from such a
bind call can be assigned to a `T` variable without checking, crashing the application if the bind failed.

In any other case, bind is invalid.

### is and as

`r 'is' T` and `r 'as' T t`

The keywords `is` and `as` can be used to check if the type of an instance r matches the specified type T. In case of
mehtod maps the check is valid for value types as well as base method maps to e.g. check for the existance of
interfaces. The difference between `is` and `as` is, that `as` will give you a reference T to r. This will view_as,
if possible, allowing to convert `any` to a concrete type.

For the purpose of compound statements, t shall be available immediately after the check. (e.g. `r as uint t && t > 100`
is valid syntax and behaves as expected).

### or

`x or y`

Or is a special keyword that can be used to provide a fallback for empty optionals or similar. The left side needs to be an
expression of optional, expect T type or primitive, while the right hand needs to be of T type. If the left hand is a failing expect, empty optional or string, numeric 0 or false,
the right hand side will be returned.

### type_of

`'type_of' THING`

Returns a `Type` methodmap that allows you to inspect various aspects of a type. Check the reference implementation for
what you can expect from this interface.
For a struct this includes things like fix sized, name and members. The member list might be obfuscated or inaccurate.

```rust
def Type as methodmap of Handle {
  native fun name() -> (str v);
  native fun size() -> (uint v);
}
def IterableType as methodmap of Type {
  native fun element_type() -> (Type v);
}
def ArrayType as methodmap of IterableType {
  native fun array_shape() -> (vec<uint> v);
}
def DictType as methodmap of Type {
  native fun key_type() -> (Type v);
  native fun value_type() -> (Type v);
}
def DataType as methodmap of Type {
  native fun is_fixed_size() -> (bool v);
  native fun is_dynamic_size() -> (bool v);
  native fun members() -> (dict<str,Type> v);
  native fun offsetOf(str member) -> (uint v);
  native fun sizeOf(str member) -> (uint v);
}
def MethodMapType as methodmap of Type {
  native fun members() -> (dict<str,Type> v);
  native fun parents() -> (vec<Type> v);
  native fun binds() -> (vec<Type> v);
}
```

### decomposing data

To decompose a struct, you can view it as a type that is memory compatible and names the sub type, or view as a memory range returned by offset_of and size_of.

## Required Builtins

### collections

All collections are countable (`#`), subscriptable (`[]`) and support stream operators. Additionally thay have the following methods:

Vec <T>
```rust
def VecMethods as methodmap binds vec<T> {
  native fun clear() -> ();
  native fun add(const T element) -> ();
  native fun insert(uint index, const T element) -> (bool success);
  native fun remove(uint index) -> (T? removed);
  native fun find(fun(const T element)->(bool match), predicate)->(uint index);
}
```

List <T> 
```rust
def ListMethods as methodmap binds list<T> {
  native fun clear() -> ();
  native fun add(const T element) -> ();
  native fun insert(uint index, const T element) -> (bool success);
  native fun remove(uint index) -> (T? removed);
  native fun find(fun(const T element)->(bool match), predicate)->(uint index);
}
```

Set <T>
```rust
def SetMethods as methodmap binds set<T> {
  native fun clear() -> ();
  native fun add(const T element) -> (bool added);  // false if already present
  native fun remove(const T element) -> (bool removed);
  native fun contains(const T element) -> (bool present);
}
```

Dict <K, V>
```rust
def DictMethods as methodmap binds dict<K,V> {
  native fun clear() -> ();
  native fun remove(const K key) -> (V? removed);
  native fun contains(const K key) -> (bool present);
  native fun keys() -> (set<K> keySet);
  native fun values() -> (vec<V> valueList);
}
```

### print

`fun print(const str? format, const any args...)->()`

Simple line based output, using a python style format string and arguments.

### crash

`fun crash(const str? msg)->()`

Handling a failed value, by casting to error, allows for resource management before failing further up the call chain.
Optionals on the other hand are intended for all instances where the absence of a value does not prevent execution form
continuing, like getting a value for a key not set in a map.

For truly unrecoverable errors, you can use the `crash()` built in with an optional message. Crashing, be it through
`crash()` or unboxing an optional/expect incorrectly, will always terminate the application on the spot.

Crash will still generate aand dump a stacktrace, so you don't have to get creative with errors.

### fail

`fun fail(const str? msg)->(move Error errorBase)`

This function allocates the base object for errors, that supports the methods of the `Error` method map.

### cast

Cast functions can have any name, but require the `@cast`-attribute and a signature of single in, single out. On most
hardware, to increase performance, complex types it should be const in, move out and for primitives like numbers it
should be copy in, copy out.

The compiler will generate cast chains, if no direct cast from A to B is possible, at a possible performance penalty.

For all numeric types A and B:
Widen with sign-extend or zero-extend, truncate when narrowing
`@cast fun cast_A_B(A a)->(B b)`

For strings of all bit widths X for encodings UTF-8, UTF-16 and UTF-32:
Invalid codepoints are encoded as well.
`@cast fun cast_str_vuX(const str in)->(move vec<uX> out)`
`@cast fun cast_vuX_str(const vec<uX> in)->(move str out)`

For all string <-> number conversions:
`@cast fun cast_str_N(const str in)->(N out)`
`@cast fun cast_N_str(N in)->(move str out)`

### isValid

`fun isValid(ref Handle hdl)->(bool valid)`

Check if the resource backed by a handle is still valid. Validity is based on the opaque backend (e.g. native
dependency):

### real/imag

For all complex number C of some bit width and a float number F of the same width:
`fun real(C complex)->(F realPart)`
`fun imag(C complex)->(F imagPart)`
