**Project 2: Semantic Checks**
**Reed CSCI 394 Fall 2026**

*submit .zip of your (cleaned) source folder by 10/16*

Your assignment is to add a semantic analysis stage to your
**Snek** code from Project 1. This compiler stage imeediately
follows the parse stage. It checks that a program doesn't contain a
certain variety of errors--- namely errors due to applying the wrong operations on
data, failing to meet the specification of a function
with a call, or failing to meet the specification of a function
with a `return`. In short: our analysis will add type checking and
returns analysis to **Snek**.

The kinds of coding errors we'll check are normally allowed
by Python--- Python doesn't do any type checking, for example--- so we
are taking the **Snek** language further from Python with these
changes. We invent a more restrictive dialect of the **Snek**
language, and the syntax is a little different.

Let's call it **tSnek**. It  has type annotations for its functions
and variables, in three ways. (1) A programmer has to describe the
types of the formal parameters of a function or procedure defined
with `def`. (2) A programmer has to describe the type of value a
function returns. (3) A programmer has to describe the type of each
(locally scoped) variable they introduce within a block of a statements.

These type annotations
limit the way variables can be used, they limit the ways
that functions and procedures can be called, and what values
the functions compute.
And so this new compiler stage is a gate-keeper for which programs
are run. The lexer stage tosses out programs that use unknown lexemes, the
parser stage tosses out programs with poor syntax, and this additional
stage tosses out programs that attempt to apply meaningless operations
on their values or that fail to make calculations that their
specification promises.

## Changes to the syntax

The type annotations change the language syntax. We modify the rule for
defining functions and procedures. A function is defined using
the notation
~~~ none
<defn> ::= def <name> ( <fmls> ) -> <type> : <nest>
~~~
and a procedure is defined using the notation
~~~ none
<defn> ::= def <name> ( <fmls> ) : <nest>
~~~
Here, `<fmls>` is a list of formal parameter names and their associated types.
They are given by the rule:
~~~ none
<fmls> ::= <nmdc> , ... , <nmdc>
<nmdc> ::= <name> : <type>
~~~
Here, for example, is a function that returns whether the square of
its parameter `x` is greater than its parameter `y`:
~~~ python
def square_exceeds(x : int, y : int) -> bool :
    return (x * x > y)
~~~
Note that functions explicitly return a value, and the type of the value
returned is given after the arrow notation `->`.
Here instead is a procedure that appends a value to a string and prints it,
conditionally including a comma in that output.
~~~ python
def output_with_value(s : str, comma : bool, x : int) :
    if comma:
        print(s + ", " + str(x))
    else:
        print(s + str(x))
    return
~~~
Note that procedures are distinguished
from functions by having no arrow in their `def` line.
Function calls can appear within value expressions and procedure
calls can appear as a statement line.

For all of this new syntax, we have the grammar production:
~~~ none
<type> ::= int | str | bool | unit
~~~
because data values in this language are either integers, strings, and
boolean conditions. The `unit` type, inhabited by the value `None`, happens
to be the "return type" of procedures.

Each of the formal parameter variables has an annotation that governs
their use within a definition's body. Local variables can also be introduced
within a block of statements. The syntax for variable introduction is
given below
~~~ none
<stmt> ::= <nmdc> = <expn> EOLN
~~~
For example, here is a procedure that prints the prime numbers up to a
given `number`.
~~~ python
def primesUpTo(number : int) :
    print(2)
    count : int = 2
    skip : bool = True
    while count <= number:
        if not skip and isPrime(number):
            print(count)
        count += 1
        skip = not skip
    return
~~~
It introduces an integer `count` and a boolean flag `skip` that are used
below their introduction.

The effect of all of these changes is that every name used--- whether it
is a function name, a procedure name, a parameter name, or a local variable
name--- has type information attached to it.

# The Starter Code

* [**Download the starter code.**](tsnki.zip)

This assignment builds from the last assignment. There is an additional
family of classes that need to be defined to support type checking and
return type analysis. These are defined in `snek-check.hh`.
They are

* `Type` : this is given by a C++ `typedef`. It represents a **Snek** program value type--- the integer type, the string type, the boolean type, and the unit `None` type.

* `Rtns` : this `typedef` represents the return behavior of a block of **Snek** code. It indicates whether a block definitely returns a value, or if executing its statements "falls through" because there is no `return`, or if both behaviors are possible.

* `SymT` and `SymInfo` : a table of information, and the information itself, that gives the type (and other things) about the local variables of a **Snek** definition's body.

Along with this header file, the semantic checking code is defined in
`snek-check.cc`.  Some of the code in this `.cc` file supports
`Type`, `Rtns`, `SymInfo`, and `SymT`. The remaining code defines the
AST methods `Prgm::chck`, `Defn::chck`, `Blck::chck`, the overrides of
`Stmt::chck` for all subclasses of `Stmt`, and the overrides of
`Expn::chck` for all subclasses of `Expn`. These methods' code aren't
complete. Your assignment will be to finish the code for some of these `chck`
methods.

The starter code lexes, parses, checks, and then runs a portion of
the **Snek** language, showcasing some of the checking that you
will need to perform. You'll want to modify your Project 1 code,
upgrading it to mimic the semantic checking that the starter
code performs, and then complete the suite of semantic checks
outlined below. You also need to add function and procedure calls
if you didn't already do that for Project 1.

# The Assignment

Modify your code for Project 1 so that it parses this typed variant
of **Snek** and then checks the code to make sure there are no
obvious errors. The checks you will perform are to

* make sure names are defined
* make sure expressions operate on data of the appropriate type
* make sure calls pass the right number of arguments of the appropriate type
* make sure every procedure and function returns
* make sure every function returns a value of the appropriate type
* make sure statements are reachable
* make sure that the main script never returns

The starter code has all the methods for `Expn::chck` written except
for `FCll::chck`, the code that should check a function call. It has
`Prgm::chck` and `Defn::chck` written.  It also has the methods
`Ntro::chck`, `Pass::chck`, and `Prnt::chck` written. The last two are
trivial, and then `Ntro` checking, `Defn `Lkup` checking can be used
as your model for some of the others.

Write `chck` for these classes:
* `Blck`, the checking of a sequence of statements
* `Asgn`, `MnEq`, and `PlEq`, three kinds of variable re-assignments
* `Whle` and `IfEl`, the two *branch-y* "control flow" statements
* `PCll` and `FCll`, procedure and function calls
* `PRtn` and `FRtn`, returns from proceudres and funxtionx.
If you didn't for Project 1, you'll also write:
* `Defn::call` which serves `PCll::exec` and `FCll::eval`.

# Our Checking Rules

In lecture we formulated a notation for describing the rules for checking
a program. These were laid out as *inference rules* for a judgement
we'd like to make about code. We write these rules with each concluding
judgement sitting under a horizontal line, putting above the line a set of
conditions that, if met, allow us to make that judgement.

## Checking expressions

As a first example, we have the rule:
```math
\frac{
\Delta; \Gamma \vdash e_1\ :\ \mathtt{int}
\ \ \ \ \ 
\Delta; \Gamma \vdash e_2\ :\ \mathtt{int}
}
{
\Delta; \Gamma \vdash e_1 + e_2\ :\ \mathtt{int}
}
```
This tells us that we can conclude that the addition of two expressions
$e_1$ and $e_2$ is okay, just so long as checking both $e_1$ and $e_2$
is okay. Furthermore, if we can conclude that each expression will
yield an integer when evaluated, then we are allowed to conclude that
the sum expression will yield an integer.

**Snek** uses the same `+` operator for string concatenation and so there
is a similar rule for its use. The more general rule is
```math
\frac{
\Delta; \Gamma \vdash e_1\ :\ \sigma
\ \ \ \ \ 
\Delta; \Gamma \vdash e_2\ :\ \sigma
\ \ \ \ \ 
\sigma \in \{ \mathtt{int} , \mathtt{str} \}
}
{
\Delta; \Gamma \vdash e_1 + e_2\ :\ \sigma
}
```

In general we use the judgement
```math
\Delta; \Gamma \vdash e : \tau
```
to express that an expression $e$ checks out okay and that, furthermore,
it is known at compile time that it will yield a value of type $\tau$
when it is evaluated.  The additional symbols $\Delta$ and $\Gamma$
give program context information for the expression we are checking.
$\Delta$ gives the type signatures of program definitions, and
$\Gamma$ gives the types of variables whose scope includes the expression.
So, for example
~~~ python
def sqr(n : int) -> int :
    return n * n
a : int = 3
print(sqr(a) + (sqr(a+1)))
~~~
checks out because `a : int` and `sqr` takes an `int` and returns an `int`.
The code below would not check out, for several reasons:
~~~ python
def qsr(n : int) -> bool :
    return n * n
a : str = "3"
print(sqr(a) + (sqr(a+1)))
~~~
The sum `a+1` won't check because the left side is not an integer (or else
because the right side is not a string). And the printed sum doesn't check
out because its subexpressions don't check out.

### &bull; Coding these expression checks

In the code, within the file `snek-check.cc`, these expression checks are
all implemented with methods with the signature:
~~~
    Type chck(Defs& defs, SymT& symt);
~~~
In that signature, `defs` is acting as $\Delta$ and `symt` is acting
as `Gamma`. The methods of each `Expn` subclass each check their
sub-expressions (if they have them), make sure they yield values of the
appropriate type, and then return back the type of their AST's expression.
For example, this code below mimics our rule for addition:
~~~
    Type Plus::chck(Defs& defs, SymT& symt) {
        Type t1 = left->chck(defs, symt);
        Type t2 = rght->chck(defs, symt);
        if (is_int(t1) && is_int(t2)) {
            return Type{IntTy {}};
        }
        if (is_str(t1) && is_str(t2)) {
            return Type{StrTy {}};
        }
        throw SnekError {where(), "Type mismatch for plus!"};
    }
~~~
And so what we see is that the type(s) returned by an `Expn::chck`
corresponds to the $: \tau$ of our expression typing judgement.

You might find it useful to take a look at `Ltrl::chck` and `Lkup::chck`
to see other examples. In particular, checking variable lookups involves
looking up the variable's name in `symt` under the current scope of
the block being executed. Furthermore, it has the side effect of
recording the symbol table information of the looked up `name` within
that `Lkup` node's `name_info` attribute.

## Checking statements and blocks of statements

We also formulated rules for reasoning about the return behavior
of code. We used the notation
```math
\Delta; \Gamma \vdash \mathcal{B} \Uparrow \rho
```
to describe that the statements and subexpressions in a program block
$\mathcal{B}$ check out, and also that executing the block has the
return behavior $\rho$. We use the same notation for individual
statements $\mathcal{s}$:
```math
\Delta; \Gamma \vdash \mathcal{s} \Uparrow \rho
```
The return behavior, denoted $\rho$ above, is notated in three
different ways, namely
```math
\rho ::= \mathtt{void}\ |\ \mathtt{void} + \tau\downarrow\ |\ \tau\downarrow
```
In the above, $\tau$ is either `int`, `str`, `bool`, or `unit`.

The behavior $\mathtt{void}$ corresponds to the behavior of a statement
that does not return. Rather, the next statement below it is
executed. The statements `pass`, `print`, a procedure call, and
assignment do not return.  Here, for example, is a function `f` whose
first three statements do not return, and whose last statement does
return.
~~~ python
def f(x : int) -> int:
    x = x + 1
    print(x)
    pass
    return x * 42
~~~
Using our notation, we'd say that the first three $\Uparrow
\mathtt{void}$ and the last one $\Uparrow \mathtt{int}\downarrow$.

What about the third kind of return behavior? In our notation, it
would be written $\mathtt{void} + \tau\downarrow$ for some type
$\tau$. This notation would say that a statement definitely could
return a value of type $\tau$ and definitely could not return.

What kind of statement does that? An example is in the code of this function:
~~~ python
def g(x : int, y : int) -> int:
    y = x + 1
    if y > 42:
        return x
    else:
        print(x)
~~~
If `x` is smaller than 42, the `else`
clause of the `if` statement runs, and no value is returned by `g` in
that case. In our notation, we would say that the `if` statement
has the behavior $\Uparrow \mathtt{void} + \mathtt{int}\downarrow$
because there is the possibility of it returning an integer whenever
`x` is 42 or larger and there is also the possibility that it prints
(and does not return) whenever `x` is smaller than 42.

Our checking should complain about `g` because of the possibility
that the code might not return an integer.

Let's give two examples of statement and block checking rules.
We have the rule
```math
\frac{
\Delta; \Gamma \vdash e : \tau
}
{\Delta; \Gamma \vdash \mathtt{return}\ e \Uparrow \tau}
```
This says that, if we can conclude that expression $e$ has
type $\tau$, then the statement $\mathtt{return}\ e$ 
returns a value of type $\tau$.

We have the rule
```math
\frac{
 \Gamma \vdash x:\tau\ \ \ \ \ \Delta; \Gamma \vdash e:\tau
}
{\Delta; \Gamma \vdash x = e \Uparrow \mathtt{void}}
```
This says that an assignment statement checks out so long as the
type of the right-hand side expression matches the type of
the variable being assigned on the left-hand side. Furthermore,
an assignment statement does not return.

For blocks we have, for example, the rule
```math
\frac
{
\mathcal{s} \not\in \mbox{Intro}
\ \ \
\Delta; \Gamma \vdash \mathcal{s} \Uparrow \mathtt{void}
\ \ \
\Delta; \Gamma \vdash \mathcal{B} \Uparrow \mathtt{void} + \tau\downarrow
}
{
\Delta; \Gamma \vdash \mathcal{s} ; \mathcal{B} \Uparrow  \mathtt{void} + \tau
}
```
This says that a block that starts with a statement $\mathcal{s}$ checks out
fine when the statement doesn't return. This means that the block $\mathcal{B}$
will be executed. Now, if that statement is not a variable introduction,
we can check $\mathcal{B}$ using the same local variable context $\Gamma$.
If that block checks out and we see that its code could either not return
or could return a value of type $\tau$, then the whole block
$\mathcal{s} ; \mathcal{B}$ can be concluded to have the same
overall return behavior.

### &bull; Coding statement and block checks

If you look at the starter code, you'll see that we've added methods with
signature
~~~
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
~~~
one for each subclass of `Stmt`, and also for `class Blck`.

Like for expressions, `defs` serve as $\Delta$ and `symt` serves
as $\Gamma$. And then both the return value of this method and the `expd`
parameter that is handed to the method serve as $\rho$. These denote
the return behavior of the statement (or sequence of statements in the case
of `Blck`).

There is some pragmatism here, somehow differing from our judgement inference
rules, allowing us to direct the checking. When, for example, we are checking
a function's body, we already know from its declaration what type the
programmer expects its code to return. And so, when we check a `Defn`, we
call `chck` on its body, passing it `def.returns()`.

And so the expectation with `chck` is that, as we are checking each
statement according to our judgment rules, we will see whether a
statement has a return behavior that comports with the return behavior
expected by `expd`. If, for example, the function we are checking
returns an `int`, and when we check a loop and somewhere in its code
it instead returns a `bool`, then we know that the code has a problem.
If it sometimes resturns an `int` within the loop, and sometimes falls
through, then things are okay, just so long as later statements definitively
return an `int`.

## &bull; Code that checks `print` and `pass`

The type signature for `Stmt::chck` is
~~~ C++
Rtns Stmt::chck(Rtns expd, Defs& defs, SymT& symt);
~~~
This checks the typing and the return behavior of a statement, when
told the type signatures and return behavior of the definitions `defs`
and the local variable type info recorded in `symt`.

This code corresponds to the statement judgement 
```math
\Delta; \Gamma \vdash \mathcal{s} \Uparrow \rho
```

Here are some rules we devised for checking statements:
```math
\frac{}{\Delta; \Gamma \vdash \mbox{pass} \Uparrow \mbox{void}}
```
```math
\frac{
\Delta; \Gamma \vdash e\ :\ \tau
}
{\Delta; \Gamma \vdash \mbox{print}(e) \Uparrow \mbox{void}}
```
The first says that `pass` statements don't return.
The second says that `print` statements don't return either,
but that their expression $e$ had better pass our type checking.

Here is the equivalent C++ code for the first two rules.
~~~ C++
Rtns Pass::chck(Rtns expd, Defs& defs, SymT& symt) {
    return Rtns{Void {}};
}

Rtns Prnt::chck(Rtns expd, Defs& defs, SymT& symt) {
    Type expn_ty = expn->chck(defs,symt);
    return Rtns{Void {}};
}
~~~

## &bull; Code that checks introductions `Ntro`

In lecture we said that the typing rule for variable introductions is
```math
\frac
{
\Delta; \Gamma \vdash e : \tau
\ \ \ \ 
\Delta; \Gamma, x : \tau \vdash \mathcal{B} \Uparrow \rho
}
{
\Delta; \Gamma \vdash x : \tau = e ; \mathcal{B} \Uparrow \rho
}
```
In saying this, we are making the assumption that new type
designations of variables "stack up" to the right when we build
$\Gamma$. And then we are assume that the "look-up" of a variables
type within $\Gamma$ follows this stacking discipline. We scan through
$\Gamma$ from right to left.

Here is how this is coded in the starter code:
~~~
Rtns Ntro::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs, SymT& symt) {
    Type t = expn->chck(defs, symt);
    name_info = symt.add_locl(name, t);
    if (t != type) {
        throw SnekError {where(), "...Error raised!..."};
    }
    return Rtns {Void {}}; 
}
~~~
We use the `SymT` method `add_locl`
to put a new local variable named `name` onto the
`symt` stack in the current scope.
If you look at `Lkup::chck` you'll see that we then can look up
the information about a `name` with `lookup_in_scope`.

## &bull; Starter code that checks `Blck`

We are asking you to get `Blck::chck` working in step 5. But also we are
asking you to get `Stmt::chck` working for several statements in steps 1
through 4. Because of this, we've written a somewhat bogus, but useful,
`Blck::chck` method shown below:
~~~
Rtns Blck::chck(Rtns expd, Defs& defs, SymT& symt) {

    symt.push_scope();
    for (Stmt_ptr stmt : stmts) {
        // FIX THIS!
        Rtns stmt_rtns = stmt->chck(expd, defs, symt);
    }
    symt.pop_scope();

    return Rtns {Void {}}; // FIX THIS!
}
~~~
This, it turns out, gives us the needed stack-like behavior for the
scoping rules of variable introductions. The key way this happens is
with `push_scope` and `pop_scope`. If you consider that blocks of
statements might be nested deeply (with `if:else` and with `while`),
and that these might introduce variables, we want to limit the scopes
of those variables to only within their nested body. That is what these
push and pop calls do.

What's missing in this code (and why it is bogus) is the check within the
loop that some earlier statement doesn't definitely `return`. Doing so
would only make later statements moot. They would never get executed.

You'll fix this in Step 5.

## Checking the whole program

We have the two judgements
```math
\vdash \mathcal{P}\ \mbox{ok} \ \ \ \ \Delta \vdash \mathcal{d}\ \mbox{ok}
```
The right judgement describes whether a function definiton $d$ checks out.
We need to know the type signatures of all the functions, as given by
$\Delta$, to reach this judgement. The left judgement describes
whether a program $\mathcal{P}$ checks out. A program is modeled as
```math
\mathcal{P} = d_1 \ldots d_k\ \mathcal{M}
```
It is just a series of definitions $d_1 \ldots d_k$ followed by a
main script block $\mathcal{M}$.
We have the rule
```math
\frac
{\Delta(\mathcal{P}) \vdash d_1\ \mbox{ok}
\ \ \ \cdots \ \ \ 
\Delta(\mathcal{P}) \vdash d_k\ \mbox{ok}
\ \ \ \ 
\Delta(\mathcal{P}); \cdot \vdash \mathcal{M} \Uparrow \mbox{void}}
{\vdash d_1 \ldots d_k\ \mathcal{M} \ \mbox{ok}} 
```
This says that the program checks out okay if its definitions
$d_1 \ldots d_k$ each check out, if its main script $\mathcal{M}$
checks out, and if none of the statements in $\mathcal{M}$ return.

### &bull; Coding whole-program checking

These rules are coded up in `snek-check.cc` under the method definitions
~~~
void Prgm::chck(void) { ... }
void Defn::chck(Defs& defs) { ... }
~~~
Check these two out!

Checking a program just involves checking each of the program's definitions
and then checking that the main script never returns. The starter code does
this work already and is shown below:
~~~ C++
void Prgm::chck(void) {

    // Check each definition.
    for (std::pair<Name,Defn_ptr> dfpr : defs) {
        Defn_ptr d = dfpr.second
        d->chck(defs); 
    }

    // Check the main script.
    Rtns rtns = main->chck(Rtns{Void {}}, defs, main_symt);

    // Make sure the block never has a return statement.
    if (!std::holds_alternative<Void>(rtns)) {
        SnekError(main->where(), "Main script should not return.");
    }
}
~~~
The code for checking each definition has a similar quality
~~~

    // Check the body.
    Rtns rtns = body->chck(Rtns{rety}, defs, symt);

    // If function, make sure it returns in all cases.
    if (std::holds_alternative<Void>(rtns) && rety != UnitTy {}) {
        throw SnekError(body->where(), "Function body never returns.");
    }
    // if procedure, make sure it never returns a non-unit value.
    if (std::holds_alternative<VoidOr>(rtns)) {
        // Similar checks, but allowing returns and fall throughs....
        ...
    }
}
~~~
What's hidden here is that, in the starter code, the symbol table `symt`
already has the formal parameter's types loaded during parsing. The local
context for the definition's body is read from the `def` line.

---

# The coding steps for this assignment

Below, we lead you though the coding of the assignment as a series of
steps. With each step, we relate the C++ code you need to write to perform
checking to the notation and rules from lecture that prescribe when a
**Snek** program checks out okay.

1. Check basic statments.

2. Check returns.

3. Check conditionals and loops.

4. Check calls.

5. Check blocks of statements.

6. Handle function call and return within the interpreter. 

We go through each of these below.

## Step 1: checking basic statements

Since we at least have a `Blck::chck` that scans through each of the
statements and checks them, lets work on the statement checks.

### &bull; Checking assignment statements

Write `Asgn::chck` so that it mimics the inference rule we gave
for assignment statements. You make a `chck` call to check the type
of the right-hand side. After that you need to look at `symt` to see
what info it has recorded for the type of its variable.
This type had better match the type deduced by the call to `expn->chck`.

### &bull; Checking update statements

These are just `PlEq` and `MnEq`. The work is similar to what's done
for `Asgn::chck`, but also what's done for `Plus::chck` and `Mnus::chck`.

## Step 2: checking `return`

### &bull; Checking function value return

Let's examine a more interesting rule. I call this kind
of statement a "function return." I name its AST node
class `FRtn`. In your code for `FRtn::chck`, with signature
~~~ C++
Rtns FRtn::chck(Rtns expd, Defs& defs, SymT& symt)
~~~
you'll want to several things:
1. You'll check the expression of the return, examine its type.
2. You'll compare that type with the return behavior expected by `expd`.
If there is a mismatch you raise an error. Otherwise, all is good,
and you can end the method by returning the return behavior of this
function return statement.

### &bull; Checking procedure return

Similar to the `FRtn` class, there is the `PRtn` class. I use this to
model statements that return from a procedure---the `return` with no
expression. We model this internally in **Snek** the way that
Python does. We treat it as a `return None` and so it `Rtns {UnitTy {}}`.
Use this when you write the code for `PRtn::chck`.

## Step 3: checking conditionals and loops

Let's look at the rules for `if` and `while`. For the `if` statement we
say that
```math
\frac{
\Delta; \Gamma \vdash  e: \mathtt{bool}
\ \ \ \
\Delta; \Gamma \vdash  \mathcal{B}_1: \rho_1
\ \ \ \ 
\Delta; \Gamma \vdash  \mathcal{B}_2: \rho_2
}
{\Delta; \Gamma \vdash
\mathtt{if}\ e: \mathcal{B}_1\ \mathtt{else}: \mathcal{B}_2 \Uparrow \rho_1 + \rho_2}
```
This says that the condition of the `if` had better check out as a `bool`.
And then whatever the behaviors of the two blocks are--- one might return,
and the other might not, say--- the return behavior is the "sum" of their
respective behaviors.

Here is the rule for the `while` statement:
```math
\frac{
\Delta; \Gamma \vdash  e: \mathtt{bool}
\ \ \ \
\Delta; \Gamma \vdash  \mathcal{B}: \rho
}
{\Delta; \Gamma \vdash
\mathtt{while}\ e: \mathcal{B} \Uparrow \mathtt{void} + \rho}
```
This says that, again, the condition had better be of type `bool`.
And then, if the body returns, since it might not run, the return
behavior of the statement is either to not return (when the condition
is immediately `False`), or to return. There could be other cases---
the body might and might not return; the body might definitely not
return--- and in lecture we defined the meaning of $\mathtt{void} + \rho$
for all these cases.

### &bull; Coding `IfEl::chck` and `Whle::chck`

Okay, these are significant. Have the C++ mimic the rules!

## Step 4: checking calls

There are two kinds of `def` calls in **Snek**, given by the syntax:
~~~ none
<expn> ::= <name> ( <expn> , ... , <expn> )
<stmt> ::= <name> ( <expn> , ... , <expn> )
~~~
The first is a function call expression. It can be used within
a calculation of some value. The call is made to any `def` whose
type signature has a return type designated by `->`. The second
is a procedure call statement. It can be used as a line in
a block of statements. The call is made to any `def` whose
type signature has no designated return type, and so it is
to those whose bodies use a `return` statement, rather than
one that explicitly returns a value.

In the starter code, these are checked with the methods
~~~ C++
Type FCll::chck(Defs& defs, SymT& symt);
Rtns PCll::chck(Rtns expd, Defs& defs, SymT& symt);
~~~
For both of these, the type checking occurs as follows:

1. Look up the name of the function/procedure within `defs`. This will yield a `Defn_ptr`. Access its symbol table that holds the formal parameter information.

2. Check (using `get_frmls_size`) the number of parameters expected by this definition and see if it equals the number of arguments passed at this call site.

3. Check each of the argument expressions passed at this call site and see whether the type matches the type signature as stored in the symbol table for that definition. Each parameter type can be accessed using `get_frml`.

Lastly, for function call, we need `FCll::chck` to return the type of
the return value of the function. This can also be found in the `Defn_ptr`
of this function (obtained from parsing).

And then, lastly, for procedure call, we need `PCll::chck` to verify that
the name being called is actually a procedure. Recall that we, mimicking
Python, model procedures as a `Defn` whose return type is `Type {UnitTy {}}`.
Also for `PCll::chck`, we need to say that procedure calls themselves are
not `return` statements. And so their return behavior is `Rtns {Void {}}`.

### &bull; Coding `FCll::chck` and `PCll::chck`

Again, you'll transcribe the above as C++. 

## Step 5: checking a block's return behavior

You've reached the last checking step. Recall that we wrote this rule for
checking a block of statements:
```math
\frac
{
\mathcal{s} \not\in \mbox{Intro}
\ \ \
\Delta; \Gamma \vdash \mathcal{s} \Uparrow \rho_1
\ \ \
\Delta; \Gamma \vdash \mathcal{B} \Uparrow \rho_2
\ \ \
\rho_1 \rhd \rho_2 \not= \bot
}
{
\Delta; \Gamma \vdash \mathcal{s} ; \mathcal{B} \Uparrow \rho_1 \rhd \rho_2
}
```
This rule governed the return behavior of the sequencing of statements.
The entire aggregate behavior of the sequencing was encoded in the
$\rhd$ operator. We devised the table
```math
\begin{array}{c|c|c|c}
\rho_1 \rhd \rho_2                    & \mathtt{void} & \mathtt{void} + \tau & \tau\downarrow \\ \hline
\mathtt{void}         & \mathtt{void} & \mathtt{void} + \tau\downarrow & \tau\downarrow \\ \hline
\mathtt{void} + \tau  & \mathtt{void} + \tau\downarrow & \mathtt{void} + \tau\downarrow & \tau\downarrow \\ \hline
\tau\downarrow                & \bot        & \bot               & \bot \\ \hline
\end{array}
```
The table says that, if the statement $\mathcal{s}$ definitely
returns a value of type $\tau$, then it is an error (indicated by $\bot$)
to have statements in a block following $\mathcal{s}$. This is the bottom
row of the table. It also says that, should the statement have *some*
route through its execution
that definitely does not return (maybe it is an `if` statement
where one of the blocks has no `return` in it), then $\mathcal{B}$
can behave any way. In that case, the sequence returns a $\tau$ if the
block does, the sequence behaves like $\mathtt{void} + \tau\downarrow$
if either or both the statement or the block could. And the sequence
does not return if both the statement and the block don't either.

### &bull; Coding Step 5

Your code for `Blck::chck` should mimic this $\rhd$ operator. It should
scan the block's code, statement by statement, making sure that:

1. If a statement *is not* the last statement, it "falls through" or it could "fall through". This means that its return behavior is `Rtns {Void {}}` or `Rtns {VoidOr {type}}` for the same `type` that is expected by `expd`.

2. If a statement *is* the last statement, it definitely returns.

## Step 6: making function and procedure calls run in `snki`

Lastly, we return to the coding that was tabled in Project 1.
If you never got `FCll::eval` and `PCll::exec` running in that project,
here is your chance.

I've already done some of the work for you in the starter code, but kicked
the can a little further down the road. You will find these method definitions
in `snek-ast.cc`:
~~~
VOpt PCll::exec(const Defs& defs, Ctxt& ctxt) const {
    if (defs.count(name) == 0) {
        throw SnekError {where(), "...error!..."};
    }
    Valu v = defs.at(name)->call(defs, params, ctxt);
    if (std::holds_alternative<unit>(v)) {
        return std::nullopt;
    } else {
        throw SnekError {where(), "...error!..."};
    }        
}

Valu FCll::eval(const Defs& defs, const Ctxt& ctxt) const {
    if (defs.count(name) == 0) {
        throw SnekError { where(), "...error!..."};
    }
    return defs.at(name)->call(defs, params, ctxt);
}
~~~
To complete the work here for Step 6, you need to write `Defn::call`.
Its code should create a new local context and add the parameter
bindings to it. To do this it needs to `eval` each of the parameter
expressions, and add their (name, value) pairings to the local context.

Once that's built it needs to then execute the body of the function.
And then it needs to return whatever value gets returned by the body.

It's pretty simple but there *is* a lot going on under the covers.
The lookup of names (in `Lkup::eval`) is based on their `SymInfo_ptr`
data. This is what serves as the "key" for the dictionary provided
as a `Ctxt`. The `SymInfo_ptr` data is stored with every variable's
use in the AST. Setting local variables' values within a `Ctxt` is
done by `SymInfo_ptr` within `Ntro::exec` and `Asgn::exec`.

Here, then, when you set up the local context, you'll want to set
the value of each formal parameter's variable in the same way for
the new `Ctxt` you've created for this call.
