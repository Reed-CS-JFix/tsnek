#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <iostream>
#include <variant>
#include <optional>
#include <exception>
#include <algorithm>
#include <sstream>

#include "snek-ast.hh"
#include "snek-util.hh"
#include "snek-check.hh"

//
// snek-ast.cc
//
// Below are the implementations of methods of AST nodes. They are organized
// into groups. The first group represents the Snek interpreter using
//
//    Prgm::run, Blck::exec, Stmt::exec, Expn::eval
//
// The second group AST::output performs pretty printing of SLP code.
//

// * * * * *
// The Snek interpreter
//

//
// Constructors
//

AST::AST(Locn lo) : locn {lo} { }
//
Prgm::Prgm(Defs ds, Blck_ptr mn, Locn lo) :
    AST {lo}, defs {ds}, main {mn}, main_symt {} { }
Defn::Defn(Name nm, SymT sy, Type rt, Blck_ptr bd, Locn lo) :
    AST {lo}, name {nm}, symt {sy}, rety {rt}, body {bd} { }
Stmt::Stmt(Locn lo) : AST {lo} { }
//
Ntro::Ntro(Name x, Type t, Expn_ptr e, Locn l) :
    Stmt {l}, name {x}, name_info {nullptr}, type {t}, expn {e} { }
Asgn::Asgn(Name x, Expn_ptr e, Locn l) :
    Stmt {l}, name {x}, name_info {nullptr}, expn {e} { }
PlEq::PlEq(Name x, Expn_ptr e, Locn l) :
    Stmt {l}, name {x}, name_info {nullptr}, expn {e} { }
MnEq::MnEq(Name x, Expn_ptr e, Locn l) :
    Stmt {l}, name {x}, name_info {nullptr}, expn {e} { }
Prnt::Prnt(Expn_vec es, Locn l) : Stmt {l}, exps {es} { }
Pass::Pass(Locn l) : Stmt {l} { }
IfEl::IfEl(Expn_ptr cd, Blck_ptr tb, Blck_ptr eb, Locn l) :
    Stmt {l}, cndn {cd}, then_blck {tb}, else_blck {eb} { }
Whle::Whle(Expn_ptr cd, Blck_ptr bd, Locn l) :
    Stmt {l}, cndn {cd}, blck {bd} { }
PCll::PCll(Name nm, Expn_vec ps, Locn l) : Stmt {l}, name {nm}, params {ps} { }
PRtn::PRtn(Locn l) : Stmt {l} { }
FRtn::FRtn(Expn_ptr e, Locn l) : Stmt {l}, expn {e} { }
//
Blck::Blck(Stmt_vec ss, Locn lo) : AST {lo}, stmts {ss} { }
//
Expn::Expn(Locn lo) : AST {lo} { }
//
FCll::FCll(Name nm, Expn_vec ps, Locn l) : Expn {l}, name {nm}, params {ps} { }
BiOp::BiOp(std::string op, Expn_ptr lf, Expn_ptr rg, Locn lo)
  : Expn {lo}, oper {op}, left {lf}, rght {rg}, sigs {} { }
Plus::Plus(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"+", lf, rg, lo} {
    // Add two integers.
    addRule("int", "int", "int", [](Valu l, Valu r) {
        return Valu {std::get<int>(l) + std::get<int>(r)};
    });
    // Concatenate two strings.
    addRule("str","str", "str", [](Valu l, Valu r) {
        return Valu {std::get<std::string>(l) + std::get<std::string>(r)};
    });
}
Tmes::Tmes(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"*", lf, rg, lo} {
    // Multiply two integers.
    addRule("int", "int", "int", 
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) * std::get<int>(r)};
        });
    // Repeat a string an integer number of times.
    addRule("int", "str", "str", 
            [](Valu l, Valu r) {
                int n = std::get<int>(l);
                std::string s = std::get<std::string>(r);
                std::string t = "";
                while (n > 0) { t += s; n--; }
                return Valu {t};
            });
    // Same.
    addRule("str", "int", "str", 
            [](Valu l, Valu r) {
                int n = std::get<int>(r);
                std::string s = std::get<std::string>(l);
                std::string t = "";
                while (n > 0) { t += s; n--; }
                return Valu {t};
            });
}
Mnus::Mnus(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"-", lf, rg, lo} {
    // Subtract two integers.
    addRule("int", "int", "int", 
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) - std::get<int>(r)};
            });
}
IDiv::IDiv(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"//", lf, rg, lo} {
    // Compute the quotient of two integers.
    addRule("int", "int", "int", 
            [this](Valu l, Valu r) {
                int lv = std::get<int>(l);
                int rv = std::get<int>(r);
                if (rv == 0) {
                    throw SnekError {
                        where(), "Run-time error: division by 0 for '" + oper +"'."
                    };
                }
                return Valu {
                    lv / rv - ((lv % rv != 0) && ((lv ^ rv) < 0))
                };
            });
}
IMod::IMod(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"%", lf, rg, lo} {
    // Compute the remainder due to an integer division.
    addRule("int", "int", "int", 
        [this](Valu l, Valu r) {
        int lv = std::get<int>(l);
        int rv = std::get<int>(r);
        if (rv == 0) {
            throw SnekError {
                where(), "Run-time error: division by 0 for '" + oper +"'."
            };
        }
        int mv = lv % rv;
        return Valu {
            mv != 0 && ((lv < 0) ^ (rv < 0)) ? mv + rv : mv
        };
    });
}
Conj::Conj(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"and", lf, rg, lo} {
    // Compute logical and of two bools.
    addRule("bool", "bool", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<bool>(l) && std::get<bool>(r)};
        });
}
Disj::Disj(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"or", lf, rg, lo} {
    // Compute logical or of two bools.
    addRule("bool", "bool", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<bool>(l) || std::get<bool>(r)};
        });
}
Less::Less(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"<", lf, rg, lo} {
    // Compare two integers.
    addRule("int", "int", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) < std::get<int>(r)};
        });
    // Compare two strings.
    addRule("str", "str", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<std::string>(l) < std::get<std::string>(r)};
        });
}
LsEq::LsEq(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"<=", lf, rg, lo} {
    // Compare two integers.
    addRule("int", "int", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) <= std::get<int>(r)};
        });
    // Compare two strings.
    addRule("str", "str", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<std::string>(l) <= std::get<std::string>(r)};
        });
}
Equl::Equl(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"==", lf, rg, lo} {
    addRule("int", "int", "bool", 
            [](Valu l, Valu r) {
                bool b = std::get<int>(l) == std::get<int>(r);
                return Valu {b};
        });
    addRule("str", "str", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<std::string>(l) == std::get<std::string>(r)};
        });
    addRule("bool", "bool", "bool", 
            [](Valu l, Valu r) {
                return Valu {std::get<bool>(l) == std::get<bool>(r)};
        });
}
//
Ltrl::Ltrl(Valu vl, Locn lo) : Expn {lo}, valu {vl} { }
Lkup::Lkup(Name nm, Locn lo) :
    Expn {lo}, name {nm}, name_info {nullptr} { }
//
Inpt::Inpt(Expn_ptr e, Locn lo) : Expn {lo}, prpt {e} { }
IntC::IntC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
StrC::StrC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
SLen::SLen(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
LNeg::LNeg(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
INeg::INeg(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }

//
// Prgm::run, Blck::exec, Stmt:: exec
//
//  - execute Snek statements, changing the runtime context mapping
//    variables to their current values.
//

void Prgm::run(void) const {
    Ctxt main_ctxt { };
    // Execute the main block within the empty context.
    main->exec(defs, main_ctxt);
}

Valu Defn::call(const Defs& defs,
                const Expn_vec& exps,
                const Ctxt& ctxt) const {
    // ***
    // FIX THIS!

    /*
     * Code here should create a local context, add the parameter
     * bindings (evaluate each of the `exps`) to it, then execute
     * the body of the function. It then needs to return whatever
     * value gets returned by the body.
     *
     * Note that lookup of names (in Lkup::eval) is based on their
     * SymInfo_ptr data. This is what serves as the "key" for the
     * dictionary provided as a Ctxt. The SymInfo_ptr data is stored
     * with every variable's use in the AST. You'll also see that
     * setting variable's values within Ctxt is done by SymInfo_ptr
     * within Ntro::exec and Asgn::exec.
     */
    (void)ctxt;
    (void)exps;
    (void)defs;
    
    // FIX THIS!
    // ***
    return Valu {None}; // FIX THIS!
}

VOpt Blck::exec(const Defs& defs, Ctxt& ctxt) const {
    // Execute each statement of a block.
    for (Stmt_ptr s : stmts) {
        VOpt rv = s->exec(defs, ctxt);
        if (rv.has_value()) {
            return rv;
        }
    }
    return std::nullopt;
}

VOpt Ntro::exec(const Defs& defs, Ctxt& ctxt) const {
    // Add or update the context with this named variable's new value.
    ctxt[name_info] = expn->eval(defs, ctxt);
    return std::nullopt;
}

VOpt Asgn::exec(const Defs& defs, Ctxt& ctxt) const {
    // Add or update the context with this named variable's new value.
    ctxt[name_info] = expn->eval(defs, ctxt);
    return std::nullopt;
}

VOpt PlEq::exec(const Defs& defs, Ctxt& ctxt) const {
    // Add or update the context with this named variable's new value.
    Valu v = ctxt.at(name_info);
    Valu ev = expn->eval(defs, ctxt);
    if (std::holds_alternative<int>(v)) {
        int i = std::get<int>(v);
        int ei = std::get<int>(ev);
        ctxt[name_info] = Valu {i + ei};
    } else if (std::holds_alternative<std::string>(v)) {
        std::string s = std::get<std::string>(v);
        std::string es = std::get<std::string>(ev);
        ctxt[name_info] = Valu {s + es};
    }
    return std::nullopt;
}

VOpt MnEq::exec(const Defs& defs, Ctxt& ctxt) const {
    // Add or update the context with this named variable's new value.
    Valu v = ctxt.at(name_info);
    Valu ev = expn->eval(defs, ctxt);
    if (std::holds_alternative<int>(v)) {
        int i = std::get<int>(v);
        int ei = std::get<int>(ev);
        ctxt[name_info] = Valu {i - ei};
    }
    return std::nullopt;
}

VOpt Pass::exec([[maybe_unused]]const Defs& defs, [[maybe_unused]]Ctxt& ctxt) const {
    // Do nothing!
    return std::nullopt;
}
  
VOpt Prnt::exec(const Defs& defs, Ctxt& ctxt) const {
    // Output the value of the expression to STDOUT.
    bool first = true;
    for (Expn_ptr e: exps) {
        if (!first) {
            std::cout << " ";
        }
        std::cout << to_string(e->eval(defs, ctxt));
        first = false;
    }
    std::cout << std::endl;
    return std::nullopt;
}

VOpt IfEl::exec(const Defs& defs, Ctxt& ctxt) const {
    Valu v = cndn->eval(defs, ctxt);
    bool bv = std::get<bool>(v);
    if (bv) {
        return then_blck->exec(defs, ctxt);
    } else {
        return else_blck->exec(defs, ctxt);
    }
}

VOpt Whle::exec(const Defs& defs, Ctxt& ctxt) const {
    Valu v = cndn->eval(defs, ctxt);
    bool bv = std::get<bool>(v);
    while (bv) {
        VOpt rt = blck->exec(defs, ctxt);
        if (rt.has_value()) {
            return rt;
        }
        v = cndn->eval(defs, ctxt);
        bv = std::get<bool>(v);
    }
    return std::nullopt;
}

VOpt PCll::exec(const Defs& defs, Ctxt& ctxt) const {
    if (defs.count(name) == 0) {
        throw SnekError {
            where(), "Run-time error: unknown procedure '" + name +"'."
        };
    }
    Valu v = defs.at(name)->call(defs, params, ctxt);
    if (std::holds_alternative<unit>(v)) {
        return std::nullopt;
    } else {
        throw SnekError {
            where(), "Run-time error: function '" + name +"' called as procedure."
        };
    }        
}

VOpt FRtn::exec(const Defs& defs, Ctxt& ctxt) const {
    Valu v = expn->eval(defs, ctxt);
    return VOpt {v};
}

VOpt PRtn::exec([[maybe_unused]]const Defs& defs, [[maybe_unused]]Ctxt& ctxt) const {
    return VOpt {Valu {None}};
}


//
// Expn:: eval
//
// - Evaluate a Snek expression within the runtime context to calculate
//   its resulting Snek value.

Valu FCll::eval(const Defs& defs, const Ctxt& ctxt) const {
    if (defs.count(name) == 0) {
        throw SnekError {
            where(), "Run-time error: unknown function '" + name +"'."
        };
    }
    return defs.at(name)->call(defs, params, ctxt);
}

//
// BiOp::eval relies heavily on...

// BiOp::addRule
//
// - This is the key helper method for defining binary operations.
//   You can specify a series of type-driven rules for caclulating
//   a binary operator's result from the values of its left and right
//   subtrees. It works by simply adding a hash table entry to the set
//   of "signatures" for that binary operation. It supports the 
//   `BiOp::eval` method's operator dispatch.
//
//  For example, we might define string repetition as taking an
//  integer and a string, and so we would add a rule for "int", "str",
//  returning "str, and the function attached to it would concatenate
//  a string several times.
//
void BiOp::addRule(std::string lt, 
                   std::string rt,
                   std::string ty,
                   std::function<Valu(Valu,Valu)> ef) {
    std::string lt_rt = lt + "*" + rt;
    sigs.insert({lt_rt, std::pair {ty, ef}});
}
//
Valu BiOp::eval(const Defs& defs, const Ctxt& ctxt) const {

    // Evaluate each subtree.
    Valu lv = left->eval(defs, ctxt);
    Valu rv = rght->eval(defs, ctxt);
    
    // Determine their resulting value's types.
    std::string lt = ty_string(lv);
    std::string rt = ty_string(rv);
    std::string lt_rt = lt + "*" + rt;

    // "Dispatch" the rule for evaluation according to the types.
    if (sigs.count(lt_rt) > 0) {
        return (sigs.at(lt_rt).second)(lv,rv);
    }

    throw SnekError {
        where(), "Run-time error: bad operand type(s) for '" + oper +"'."
    };
}

Valu Ltrl::eval([[maybe_unused]]const Defs& defs, [[maybe_unused]]const Ctxt& ctxt) const {
    // Just return the literal value.
    return valu;
}

Valu Lkup::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Look up the named variable within the current context.
    if (ctxt.count(name_info) > 0) {
        return ctxt.at(name_info);
    } else {
        throw SnekError { where(), "Run-time error: variable '" + name +"' not defined."};
    }
    
}

Valu Inpt::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Output the prompt. Await input from STDIN. Return its string.
    std::string vl;
    Valu valu = prpt->eval(defs, ctxt);
    std::cout << to_string(valu);
    std::cin >> vl;
    return Valu {vl};
}

Valu IntC::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Convert a Snek value to a Snek integer.
    int iv = 0;
    try {
        iv = to_int(expn->eval(defs, ctxt));
    }
    catch (const std::runtime_error& e) {
        throw SnekError { where(), 
            "Run-time error: cannot convert value to int."
        };
    }
    return Valu {iv};
}

Valu StrC::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Convert a Snek value to a Snek string.
    return Valu {to_string(expn->eval(defs, ctxt))};
}

Valu SLen::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Compute the length of a Snek string.
    Valu v = expn->eval(defs, ctxt);
    if (std::holds_alternative<std::string>(v)) {
        std::string s = std::get<std::string>(v);
        int sl = s.length();
        return Valu {sl};
    } else {
        throw SnekError { where(), 
            "Run-time error: attempted to take the length of a non-string."
        };
    }
}

Valu INeg::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    Valu v = expn->eval(defs, ctxt);
    int iv = std::get<int>(v);
    return Valu {-iv};
}

Valu LNeg::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    Valu v = expn->eval(defs, ctxt);
    bool bv = std::get<bool>(v);
    return Valu {!bv};
}

// * * * * *
//
// AST::output
//
// - Pretty printer for Snek code represented in an AST.
//
// The code below is an implementation of a pretty printer. For each case
// of an AST node (each subclass) the `output` method provides the means for
// printing the code of the Snek construct it represents.
//
//

void Prgm::output(std::ostream& os) const {
    for (std::pair<Name,Defn_ptr> dfpr : defs) {
        dfpr.second->output(os);
    }
    main->output(os, "");
}

void Defn::output(std::ostream& os) const {
    os << "def " << name << "(";
    bool first = true;
    for (unsigned int i = 0; i < arity(); i++) {
        if (!first) {
            os << ", ";
        }
        SymInfo_ptr xi = formal(i);
        os << xi->name << " : " << type_name(xi->type);
        first = false;
    }
    if (is_unit(returns())) {
        os << "):" << std::endl;
    } else {
        os << ") -> " << type_name(returns()) << ":" << std::endl;
    }
    body->output(os, "    ");
}

void Blck::output(std::ostream& os, std::string indent) const {
    for (Stmt_ptr s : stmts) {
        s->output(os,indent);
    }
}

void Ntro::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " : " << type_name(type) <<" = ";
    expn->output(os);
    os << std::endl;
}

void Asgn::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " = ";
    expn->output(os);
    os << std::endl;
}

void PlEq::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " += ";
    expn->output(os);
    os << std::endl;
}

void MnEq::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " -= ";
    expn->output(os);
    os << std::endl;
}

void Pass::output(std::ostream& os, std::string indent) const {
    os << indent << "pass" << std::endl;
}

void Prnt::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "print";
    os << "(";
    bool first = true;
    for (Expn_ptr e : exps) {
        if (!first) {
            os << ", ";
        }
        e->output(os);
        first = false;
    }
    os << ")";
    os << std::endl;
}

void IfEl::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "if ";
    cndn->output(os);
    os << ":" << std::endl;
    then_blck->output(os, indent + "    ");
    os << indent << "else:" << std::endl;
    else_blck->output(os, indent + "    ");
}

void Whle::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "while ";
    cndn->output(os);
    os << ":" << std::endl;
    blck->output(os, indent + "    ");
}

void PRtn::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "return";
    os << std::endl; 
}

void FRtn::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "return ";
    expn->output(os);
    os << std::endl; 
}

void PCll::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name;
    os << "(";
    bool first = true;
    for (Expn_ptr e : params) {
        if (!first) {
            os << ", ";
        }
        e->output(os);
        first = false;
    }
    os << ")";
    os << std::endl;
}

void FCll::output(std::ostream& os) const {
    os << name << "(";
    bool first = true;
    for (Expn_ptr e : params) {
        if (first) {
            os << ",";
        }
        e->output(os);
        first = false;
    }
    os << ")";
}

void BiOp::output(std::ostream& os) const {
    os << "(";
    left->output(os); 
    os << " ";
    os << oper;
    os << " ";
    rght->output(os);
    os << ")";
}

void Ltrl::output(std::ostream& os) const {
    os << to_repn(valu); 
}

void Lkup::output(std::ostream& os) const {
    os << name;
}

void Inpt::output(std::ostream& os) const {
    os << "input(";
    prpt->output(os);
    os << ")";
}

void IntC::output(std::ostream& os) const {
    os << "int(";
    expn->output(os);
    os << ")";
}

void StrC::output(std::ostream& os) const {
    os << "str(";
    expn->output(os);
    os << ")";
}

void SLen::output(std::ostream& os) const {
    os << "len(";
    expn->output(os);
    os << ")";
}

void LNeg::output(std::ostream& os) const {
    os << "(not ";
    expn->output(os);
    os << ")";
}

void INeg::output(std::ostream& os) const {
    os << "(- ";
    expn->output(os);
    os << ")";
}

// * * * * *
//
// AST::dump
//
// - outputs an AST.
//
// The code below prints the contents of an AST. For each case we output a
// header describing the node's type, and then we (with indents) dump each
// of the node's subtrees (if any).
//

void Prgm::dump(std::ostream& os) const {
    dump(os, "");
}

void Prgm::dump(std::ostream& os, std::string indent) const {
    // You should probably also dump the definitions. 
    os << indent << "Prgm" << std::endl;
    main->dump(os, indent+"    ");
}

void Defn::dump(std::ostream& os, std::string indent) const {
    os << indent << "DEFN" << std::endl;
    // Your code goes here.
}


void Blck::dump(std::ostream& os, std::string indent) const {
    os << indent << "Blck" << std::endl;
    for (Stmt_ptr s : stmts) {
        s->dump(os, indent+"    ");
    }
}

void Ntro::dump(std::ostream& os, std::string indent) const {
    os << indent << "Ntro" << std::endl;
    os << indent+"    " << name << std::endl;
    os << indent+"    " << type_name(type) << std::endl;
    expn->dump(os, indent+"    ");
}

void Asgn::dump(std::ostream& os, std::string indent) const {
    os << indent << "Asgn" << std::endl;
    os << indent+"    " << name << std::endl;
    expn->dump(os, indent+"    ");
}

void PlEq::dump(std::ostream& os, std::string indent) const {
    os << indent << "PlEq" << std::endl;
    os << indent+"    " << name << std::endl;
    expn->dump(os, indent+"    ");
}

void MnEq::dump(std::ostream& os, std::string indent) const {
    os << indent << "MnEq" << std::endl;
    os << indent+"    " << name << std::endl;
    expn->dump(os, indent+"    ");
}

void Pass::dump(std::ostream& os, std::string indent) const {
    os << indent << "Pass" << std::endl;
}

void Prnt::dump(std::ostream& os, std::string indent) const {
    os << indent << "Prnt" << std::endl;
    for (Expn_ptr e : exps) {
        e->dump(os, indent+"    ");
    }
}

void IfEl::dump(std::ostream& os, std::string indent) const {
    os << indent << "IfEl" << std::endl;
    cndn->dump(os, indent+"    ");
    then_blck->dump(os, indent+"    ");
    else_blck->dump(os, indent+"    ");
}

void Whle::dump(std::ostream& os, std::string indent) const {
    os << indent << "Whle" << std::endl;
    cndn->dump(os, indent+"    ");
    blck->dump(os, indent+"    ");
}

void PCll::dump(std::ostream& os, std::string indent) const {
    os << indent << "PCll" << std::endl;
    os << indent+"    " << name << std::endl;
    for (Expn_ptr e : params) {
        e->dump(os, indent+"    ");
    }
}

void PRtn::dump(std::ostream& os, std::string indent) const {
    os << indent << "PRtn" << std::endl;
}

void FRtn::dump(std::ostream& os, std::string indent) const {
    os << indent << "FRtn" << std::endl;
    expn->dump(os, indent+"    ");
}

void FCll::dump(std::ostream& os, std::string indent) const {
    os << indent << "FCll" << std::endl;
    os << indent+"    " << name << std::endl;
    for (Expn_ptr e : params) {
        e->dump(os, indent+"    ");
    }
}

void BiOp::dump(std::ostream& os, std::string indent) const {
    os << indent << "BiOp" << std::endl;
    os << indent << "    " << oper << std::endl;
    left->dump(os, indent+"    ");
    rght->dump(os, indent+"    ");
}

void Ltrl::dump(std::ostream& os, std::string indent) const {
    os << indent << "Ltrl" << std::endl;
    os << indent << "    " << to_repn(valu) << std::endl;
}

void Lkup::dump(std::ostream& os, std::string indent) const {
    os << indent << "Lkup" << std::endl;
    os << indent << "    " << name << std::endl;
}

void Inpt::dump(std::ostream& os, std::string indent) const {
    os << indent << "Inpt" << std::endl;
    prpt->dump(os, indent+"    ");
}

void IntC::dump(std::ostream& os, std::string indent) const {
    os << indent << "IntC" << std::endl;
    expn->dump(os, indent+"    ");
}

void StrC::dump(std::ostream& os, std::string indent) const {
    os << indent << "StrC" << std::endl;
    expn->dump(os, indent+"    ");
}

void SLen::dump(std::ostream& os, std::string indent) const {
    os << indent << "SLen" << std::endl;
    expn->dump(os, indent+"    ");
}

void LNeg::dump(std::ostream& os, std::string indent) const {
    os << indent << "LNeg" << std::endl;
    expn->dump(os, indent+"    ");
}

void INeg::dump(std::ostream& os, std::string indent) const {
    os << indent << "INeg" << std::endl;
    expn->dump(os, indent+"    ");
}
