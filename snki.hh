#ifndef _snki_hh
#define _snki_hh

// snki.hh
//
// Object classes and types to support the main driver of the Snek
// interpreter. This invents a `Driver` class for the `Snek` name-
// space. It houses the lexer, parser, and the interpreter components.
// It basically provides a housing for invoking these components. It
// also provides a top-level object that can be manipulated within the
// Bison parser code.
//
// Note: this housing was mainly added as a deliberate way of dealing
// with the mish-mash of C-like C++ interfaces invented by the C++
// variants of Flex and Bison. At issue was the sharing of pointers
// to their underlying components without C++ performing its own
// decisions about cleaning up their referenced objects. By building
// a "driver" object with some shared pointers to these components, we
// followed an idiom suggested by some C++ example code. This resulted
// in a cover to the various parse/run/etc. components that drive the
// Snek tolchain. This also provides a means to have the Bison parse
// code access the top-level components of the interpreter.
//

#include <string>
#include <istream>

#include "snek-flex.hh"
#include "snek-bison.tab.hh"

typedef std::shared_ptr<Snek::Lexer> Lexer_ptr;
typedef std::shared_ptr<Snek::Parser> Parser_ptr;
typedef std::shared_ptr<std::istream> istream_ptr;

/*
 * class Snek::Driver
 * 
 * Used by `main` to invoke the interpreter components. Also used by
 * the Bison code to set up the AST as a result of the parse.
 *
 * The methods it provides are:
 *   parse - runs the parser, building the AST
 *   set - sets the AST that results from a parse
 *   run - executes the parsed Snek program
 *   dump - (pretty) prints the AST
 *
 * Note that the constructor attempts to create a stream attached to
 * the provided name of the Snek source file. However, the success
 * of that operation is only checked when `parse` is called.
 */

namespace Snek {
    
    class Driver {
    public:
        Driver(std::string filename);
        void parse(void);
        void run(void);
        void check(void);
        void dump(bool pretty);
        void set(Prgm_ptr prgm) { main = prgm; }
        std::string src_name;
    private:
        istream_ptr src_stream = nullptr;
        Prgm_ptr    main = nullptr;
        Lexer_ptr   lexer = nullptr;
        Parser_ptr  parser  = nullptr;
    };

}

#endif 
