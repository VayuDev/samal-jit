# samal - Simple And Memory-wasting Awesomely-functional Language

Samal is a new programming language which attempts to be the perfect general-purpose (embeddable) language.
It mixes the great type system and syntax of Rust with the functional style of Elixir and Haskell.

Further information can be found at [samal.dev](https://samal.dev).

The language is currently in heavy development and not usable.

## Features

Features already implemented include:
* JIT-Compiler for most instructions on x86-64
* Lambdas
* Match expressions
* Templates
  * Automatic template type inference for function calls
* Easy API for embedding samal into your project
* Structs
* Enums
* Pointer type for recursive structs/enums (all structs/enums are on the stack)
* Strings/Chars (Strings are implemented as lists of chars, similar to Haskell)
  * Unicode (UTF-32) support; a single char can contain any unicode codepoint
* Explicit tail recursion optimization
* Copying garbage collector
* Native functions
* Type casting (using native functions)
* Using declarations (to avoid typing the module name every time)
* Very basic standard library

## Planned Features

Planned features (vaguely order):

* Comments
* A proper cli
* Floating point support
* More features & error checking in match statements
* Tuple deconstruction in assignment

## Examples

Fibonacci:

    fn fib(n : i32) -> i32 {
        if n < 2 {
            n
        } else {
            fib(n - 1) + fib(n - 2)
        }
    }

Sum up all fields of a list, making use of explicit tail recursion:

    fn sumRec<T>(current : T, list : [T]) -> T {
        if list == [] {
            current
        } else {
            @tail_call_self(current + list:head, list:tail)
        }
    }
    fn sum<T>(list : [T]) -> T {
        sumRec<T>(0, list)
    }


Iterate over a list, calling the specified callback on each element and returning a list of the return values:

    fn map<T, S>(l : [T], callback : fn(T) -> S) -> [S] {
        if l == [] {
            [:S]
        } else {
            callback(l:head) + map<T, S>(l:tail, callback)
        }
    }


Combine two lists, creating one list where each element is a tuple with one element from each list:

    fn zip<S, T>(l1 : [S], l2 : [T]) -> [(S, T)] {
        if l1 == [] || l2 == [] {
            [:(S, T)]
        } else {
            (l1:head, l2:head) + zip<S, T>(l1:tail, l2:tail)
        }
    }

Parse a http header (definitely not spec compliant, skips method+version+url line):

    fn parseHTTPHeader(socket : i32) -> [([char], [char])] {
        str = readUntilEmptyHeader(socket, "", "")
        lines = Core.splitByChar(str, '\n')
        
        headers =
            lines:tail
            |> Core.map(fn(line : [char]) -> Maybe<([char], [char])> {
                splitList =
                    Core.takeWhile(line, fn(ch : char) -> bool {
                        ch != '\r'
                    })
                    |> Core.takeWhileEx(fn(ch : char) -> bool {
                        ch != ':'
                    })
                if splitList:1 == "" {
                    Maybe<([char], [char])>::None{}
                } else {
                    Maybe<([char], [char])>::Some{(splitList:0, splitList:1:tail:tail)}
                }
            })
            |> Core.filter(fn(line : Maybe<([char], [char])>) -> bool {
                match line {
                    Some{m} -> true,
                    None{} -> false
                }
            })
            |> Core.map(fn(e : Maybe<([char], [char])>) -> ([char], [char]) {
                match e {
                    Some{n} -> n,
                    None{} -> ("", "")
                }
            })

        Core.map(headers, fn(line : ([char], [char])) -> () {
            Core.print(line)
        })
        headers
    }

Note: List syntax will maybe change in the future.

## License

The software is licensed under the very permissive MIT License. If you open a pull request, you implicitly 
license your code under the MIT license as well.

## Design notes

### Part 1: Why every programming language sucks

Please take all of this with a grain of salt, it's just my personal opinion, so feel free to disagree.

I really love programming in functional languages like Haskell or Elixir, but I really
dislike the type systems of both languages. Pattern matching is great, but Elixir is dynamically typed
which makes large projects quite difficult as I had to experience myself. Especially if you have a
GenServer which stores a lot of state, remembering the name of every key etc. becomes increasingly taxing.
Autocompletion just doesn't work most of the time. While Haskell is strongly typed, types are implicit
almost always which makes code sometimes hard to read and makes error messages hard to comprehend.

I like Rust a lot as well, but it's just not functional enough for me and seems overkill for most jobs. It's after
all a systems programming language and most of the time you don't need one. Rust (and C++) force you to always
think about memory management and ownership; but if I want to write a Compiler, I don't really care if a garbage
collector runs sometimes or not and rather take the speedup in development time.

Most garbage-collected languages have other problems: they are either not strongly typed, making large projects
impossible, or have other weird quirks like forcing OOP onto you like Java. Go might seem like it ticks a lot
of the boxes, but it's lack of generics, weird interface type, lack of VM and being neither fully functional
or object oriented makes it hard for me to enjoy using.

### Part 2: What makes samal different

So, where does this leave us? We want a language that:

* is functional
* has a great type system
* is fast enough for must stuff
* achieves the previous three points without becoming bloated and complicated

This is pretty much all I want to achieve with samal. I don't care about memory consumption (as long as it's
not well beyond bounds) and I don't care about compilation speed (as long as it stays faster than C++ compiled with
-O3).

### Part 3: Why samal sucks as well

So, what's bad about samal right now?

* It uses a custom-written peg parser with horrendous error messages - if you have a parsing error, the fastest
  thing will probably be to just try to find it yourself using the line information that it gives you (which is only
  sometimes correct)
* Calling native functions is slow
* There are unit tests, but they don't cover all features
* The Compiler class is bigger and messier than I want it to be
* We're still missing a lot of basic features

But with your help, I'm sure we can fix all of these problems! Feel free to open a pull request if you want to
improve the language, in any way, shape or form. Just ask yourself: What does the perfect language look like for me?
