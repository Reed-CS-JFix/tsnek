#ifndef __snek_ast_h_
#define __snek_ast_h_

// snek-ast.hh
//
// Object classes used by the parser to represent the syntax trees of
// Snek programs.
//
// It defines a class for a Snek program and its block of statements,
// and also the two abstract classes whose subclasses form the syntax
// trees of a program's code.
//
//  * Prgm - a Snek program that constst of some definitions followed
//           by a block of statements.
//
//  * Defn - a Snek function or procedure definition.
//
//  * Blck - a series of Snek statements.
//
//  * Stmt - subclasses for the various statments that can occur
//           block of code, namely assignment, print, and pass.
//           These get executed when the program runs.
//
//  * Expn - subclasses for the various integer-valued expressions
//           than can occur on the right-hand side of an assignment
//           statement. These get evaluated to compute a value.
//

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <iostream>
#include <variant>
#include <optional> 
#include <functional>
#include "snek-util.hh"
#include "snek-check.hh"

//
// Below we "pre-declare" each AST subclass for mutually recursive definitions.
//

class Prgm;
class Defn;
class Blck;
//
class Stmt;
class Pass;
class Asgn;
class Ntro;
class PlEq;
class Prnt;
class IfEl;
class Whle;
class PCll;
class FRtn;
class PRtn;
    
//
class Expn;
//
class FCll;
//
class Biop;  
class Plus;
class Mnus;
class Tmes;
class IDiv;
class IMod;
class Less;
class LsEq;
class Equl;
class Conj;
class Disj;
//
class Inpt;
class IntC;
class StrC; 
class SLen;
class LNeg;
class INeg;
//
class Lkup;
class Ltrl; 

//
// We alias some types, including for pointers and vectors.
//

typedef std::string Name;
typedef std::unordered_map<SymInfo_ptr,Valu> Ctxt;
//
typedef std::shared_ptr<Prgm> Prgm_ptr; 
typedef std::shared_ptr<Defn> Defn_ptr; 
typedef std::shared_ptr<Blck> Blck_ptr; 
typedef std::shared_ptr<Stmt> Stmt_ptr; 
typedef std::shared_ptr<Expn> Expn_ptr;
//
typedef std::vector<Stmt_ptr> Stmt_vec;
typedef std::vector<Expn_ptr> Expn_vec;
//
typedef std::unordered_map<Name,Defn_ptr> Defs;
//
//
// ************************************************************

// Syntax tree classes used to represent a Snek program's code.
//
// The classes Blck, Stmt, Expn are all subclasses of AST.
//

//
// class AST 
//
// Cover type for all "abstract" syntax tree classes.
//

class AST {
private:
    Locn locn; // Location of construct in source code (for reporting errors).
public:
    AST(Locn lo);
    Locn where(void) const { return locn; }
    virtual void dump(std::ostream& os, std::string indent) const = 0; 
    virtual ~AST() = default;
};


//
// class Prgm
//
// An object in this class holds all the information gained
// from parsing the source code of a Snek program. A program
// is a series of Snek statements organized as a block.
//
// The method Prgm::run implements the Snek interpreter. This runs the
// Snek program, executing its statments, updating the state of
// program variables as a result, getting user input from the console,
// and outputting results to the console.  The interpreter relies on
// the Blck::exec, Stmt::exec, and Expn::eval methods of the various
// syntactic components that constitute the Prgm object.
//

class Prgm : public AST {
public:
    //
    Defs     defs;
    Blck_ptr main;
    SymT     main_symt;
    //
    Prgm(Defs ds, Blck_ptr mn, Locn lo);

    //
    void chck(void); // Type check the entire program.
    void run(void) const; // Execute the program by interpreting its code.
    void output(std::ostream& os) const; // Output formatted code.
    void dump(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os) const;  
};

//
// class Defn
//
// This should become the AST node type that results from parsing
// a `def`. Right now it contains no information.
//
class Defn : public AST {
public:
    Name name;
    SymT symt;
    Type rety;
    Blck_ptr body;
    //
    Defn(Name nm, SymT sy, Type rt, Blck_ptr bd, Locn lo);
    //
    unsigned int arity(void) const;
    Type returns(void) const;
    SymInfo_ptr formal(int i) const;
    //
    void chck(Defs& defs);
    Valu call(const Defs& defs,
              const Expn_vec& args, const Ctxt& ctxt) const;                     
    void dump(std::ostream& os, std::string indent) const;
    void output(std::ostream& os) const; // Output formatted code.
};

//
// class Stmt
//
// Abstract class for program statment syntax trees,
//
// Subclasses are
//
//   PlEq - +=
//   Ntro - variable introduction "v:t = e"
//   Asgn - assignment statement "v = e"
//   Prnt - output statement "print(e1,e2,...,ek)"
//   Pass - statement that does nothing
//   IfEl - conditional statement
//   Whle - loop
//   PCll = procedure call
//   PRtn - procedure return
//   FRtn - function return
//
// These each support the methods:
//
//  * exec(defs, ctxt): execute the statement within the context
//                      Return whether a `return` occurred.
//
//  * output(os), output(os,indent): output formatted Snek code of
//        the statement to the output stream `os`. The `indent` string
//        gives us a string of spaces for indenting the lines of its
//        code.
//
//
class Stmt : public AST {
public:
    Stmt(Locn lo);
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt) = 0;
    virtual VOpt exec(const Defs& defs, Ctxt& ctxt) const = 0;
    virtual void output(std::ostream& os, std::string indent) const = 0;
};

//
// Ntro - variable introduction with initializer
//
class Ntro : public Stmt {
public:
    Name        name;
    SymInfo_ptr name_info;
    Type        type;
    Expn_ptr    expn;
    Ntro(Name x, Type t, Expn_ptr e, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};


//
// Asgn - assignment statement AST node
//
class Asgn : public Stmt {
public:
    Name        name;
    SymInfo_ptr name_info;
    Expn_ptr    expn;
    Asgn(Name x, Expn_ptr e, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// PlEq - "plus equals" statement AST node
//
class PlEq : public Stmt {
public:
    Name        name;
    SymInfo_ptr name_info;
    Expn_ptr    expn;
    PlEq(Name x, Expn_ptr e, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// MnEq - "minus equals" statement AST node
//
class MnEq : public Stmt {
public:
    Name        name;
    SymInfo_ptr name_info;
    Expn_ptr    expn;
    MnEq(Name x, Expn_ptr e, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};


//
// Prnt - print statement AST node
//
class Prnt : public Stmt {
public:
    Expn_vec exps;
    Prnt(Expn_vec es, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// Pass - pass statement AST node
//
class Pass : public Stmt {
public:
    Pass(Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

class PCll : public Stmt {
public:
    Name     name;
    Expn_vec params;
    PCll(Name nm, Expn_vec ps, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

class PRtn : public Stmt {
public:
    PRtn(Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

class FRtn : public Stmt {
public:
    Expn_ptr expn;
    FRtn(Expn_ptr e, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

class IfEl : public Stmt {
public:
    Expn_ptr cndn;
    Blck_ptr then_blck;
    Blck_ptr else_blck;
    IfEl(Expn_ptr cd, Blck_ptr tb, Blck_ptr eb, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

class Whle : public Stmt {
public:
    Expn_ptr cndn;
    Blck_ptr blck;
    Whle(Expn_ptr cd, Blck_ptr bd, Locn l);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// class Blck
//
// Represents a sequence of statements.
//
class Blck : public AST {
public:
    Stmt_vec stmts;
    Blck(Stmt_vec ss, Locn lo);
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    VOpt exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// class Expn
//
// Abstract class for integer expression syntax trees,
//
// Subclasses are
//
//   FCll - function call
//   BiOp - binary operation applied to two sub-expressions
//   Ltrl - literal value expression
//   Lkup - variable access (i.e. "look-up") within function frame
//   Inpt - obtains an integer input (after output of a prompt)
//   IntC - conversion to integer
//   StrC - conversion to string
//   SLen - length of a string
//
// These each support the methods:
//
//  * eval(defs, ctxt): evaluate the expression; return its result
//  * output(os): output formatted Snek code of the expression.
//
class Expn : public AST {
public:
    Expn(Locn lo);
    virtual Type chck(const Defs& defs, const SymT& symt) = 0;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const = 0;
    virtual void output(std::ostream& os) const = 0;
};

//
// FCll - function invocation AST node
//
class FCll : public Expn {
public:
    Name     name;
    Expn_vec params;
    FCll(Name nm, Expn_vec ps, Locn l);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// BiOp - abstract class for any binary operation's AST node
//

// Table of binary operation type rules.
//
typedef std::unordered_map<std::string, // Will encode pair as "foo*bar"
                           std::pair<std::string, // Result type.
                                     std::function<Valu(Valu, Valu)>>>
    BiOp_sigs;

//
class BiOp : public Expn {
public:
    std::string oper;
    Expn_ptr  left;
    Expn_ptr  rght;
    BiOp_sigs sigs;
    BiOp(std::string op, Expn_ptr lf, Expn_ptr rg, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
protected:
    void addRule(std::string lt, std::string rt, std::string ty,
                 std::function<Valu(Valu,Valu)>);
};
//
// subclasses of BiOp
//
class Plus : public BiOp { // addition binary operation's AST node
public:
    Plus(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Tmes : public BiOp { // multiplication binary operation's AST node
public:
    Tmes(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Mnus : public BiOp { // subtraction binary operation's AST node
public:
    Mnus(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class IDiv : public BiOp { // quotient binary operation's AST node
public:
    IDiv(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class IMod : public BiOp { // remainder binary operation's AST node
public:
    IMod(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Conj : public BiOp { // and binary operation's AST node
public:
    Conj(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Disj : public BiOp { // or binary operation's AST node
public:
    Disj(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Less : public BiOp { // < binary operation's AST node
public:
    Less(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class LsEq : public BiOp { // <= binary operation's AST node
public:
    LsEq(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
class Equl : public BiOp { // == binary operation's AST node
public:
    Equl(Expn_ptr lf, Expn_ptr rg, Locn lo);
};
    
//
// Ltrl - value literal AST node
//
class Ltrl : public Expn {
public:
    Valu valu;
    Ltrl(Valu vl, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// Lkup - variable use/look-up AST node
//
class Lkup : public Expn {
public:
    Name        name;
    SymInfo_ptr name_info;
    Lkup(Name nm, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// LNeg - logical negation (not) expression AST node
//
class LNeg : public Expn {
public:
    Expn_ptr expn;
    LNeg(Expn_ptr e, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// INeg - integer negation expression AST node
//
class INeg : public Expn {
public:
    Expn_ptr expn;
    INeg(Expn_ptr e, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// Inpt - input expression AST node
//
class Inpt : public Expn {
public:
    Expn_ptr prpt;
    Inpt(Expn_ptr e, Locn lo);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// IntC - int conversion expression AST node
//
class IntC : public Expn {
public:
    Expn_ptr expn;
    IntC(Expn_ptr e, Locn l);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// StrC - str conversion expression AST node
//
class StrC : public Expn {
public:
    Expn_ptr expn;
    StrC(Expn_ptr e, Locn l);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

//
// SLen - string length expression AST node
//
class SLen : public Expn {
public:
    Expn_ptr expn;
    SLen(Expn_ptr e, Locn l);
    Type chck(const Defs& defs, const SymT& symt);
    Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    void output(std::ostream& os) const;
    void dump(std::ostream& os, std::string indent) const;
};

#endif
