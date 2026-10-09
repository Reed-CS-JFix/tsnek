#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "snek-check.hh"
#include "snek-ast.hh"
#include "snek-util.hh"

// ********
//
// helper functions for Type and Rtns
//

bool is_int(Type type) {
    return std::holds_alternative<IntTy>(type);
}

bool is_str(Type type) {
    return std::holds_alternative<StrTy>(type);
}

bool is_bool(Type type) {
    return std::holds_alternative<BoolTy>(type);
}

bool is_unit(Type type) {
    return std::holds_alternative<UnitTy>(type);
}

bool operator==(Type type1, Type type2) {
    if (is_int(type1) && is_int(type2)) {
        return true;
    }
    if (is_str(type1) && is_str(type2)) {
        return true;
    }
    if (is_bool(type1) && is_bool(type2)) {
        return true;
    }
    if (is_unit(type1) && is_unit(type2)) {
        return true;
    }
    return false;
}

bool operator!=(Type type1, Type type2) {
    return !(type1 == type2);
}

std::string type_name(Type type) {
    if (is_int(type)) {
        return "int";
    }
    if (is_str(type)) {
        return "str";
    }
    if (is_bool(type)) {
        return "bool";
    } 
    if (is_unit(type)) {
        return "unit";
    }
    return "wtf";
}

Type type_of_name(std::string ts) {
    if (ts == "int") {
        return IntTy {};
    }
    if (ts == "str") {
        return StrTy {};
    }
    if (ts == "bool") {
        return BoolTy {};
    } 
    if (ts == "unit") {
        return UnitTy {};
    }
    return UnitTy {};
}

Type type_of(Rtns rtns) {
    if (std::holds_alternative<VoidOr>(rtns)) {
        return std::get<VoidOr>(rtns).type;
    }
    if (std::holds_alternative<Type>(rtns)) {
        return std::get<Type>(rtns);
    }
    return Type {UnitTy {}}; // Should not happen.
}

// ********
//
// SymT - symbol table method defintions
//

SymInfo::SymInfo(std::string nm, Type ty, int id, SymKind kd) :
    name {nm}, identifier {id}, type {ty}, kind {kd} {}

SymT::SymT() : sym_table {}, formals {} { }

//
// push/pop and lookup in scope
//
// - allows us to treat the symbol table as a stack of variable contexts
//   as we perform semantic checks of a block of Snek statements
//
// push_scope - marks the stack so current scope can be restored later
// pop_scope - pops variables of stack whose scope is closing, restores prior
// lookup_in_scope - searches back for actively scoped variable by name;
//                   returns nullptr if no such name is active.
//
void SymT::push_scope(void) {
    unsigned int mark = in_scope.size();
    marks.push_back(mark);
}
//    
void SymT::pop_scope(void) {
    unsigned int mark = marks.back();
    marks.pop_back();
    while (in_scope.size() > mark) {
        in_scope.pop_back();
    }
}
//
SymInfo_ptr SymT::lookup_in_scope(std::string name) const {
    for (int i = in_scope.size() - 1; i >= 0; i--) {
        std::string x = in_scope[i].first;
        if (x == name) return in_scope[i].second;
    }
    return nullptr;
}                       
    
//
// add frml, locl, temp
//
// - add a new entry to the symbol table, either a formal parameter, a local
//   variable (at the current scope), or a temporary.
//
//  
SymInfo_ptr SymT::add_frml(std::string nm, Type ty) {
    SymInfo_ptr info { new SymInfo {nm, ty, 0, FRML} };
    sym_table[nm] = info;
    formals.push_back(nm);
    in_scope.push_back(std::pair {nm,info});
    return info;
}
//
SymInfo_ptr SymT::add_locl(std::string nm, Type ty) {
    SymInfo_ptr info { new SymInfo {nm, ty, sym_id++, FRML} };
    sym_table[nm] = info;
    in_scope.push_back( std::pair {nm,info});
    return info;
}
//
SymInfo_ptr SymT::add_temp(std::string nm, Type ty) {
    SymInfo_ptr info { new SymInfo {nm, ty, sym_id++, TEMP} };
    sym_table[nm] = info;
    return info;
}
//
bool SymT::has_info(std::string nm) const {
    return (sym_table.count(nm) > 0);
}
//
SymInfo_ptr SymT::get_frml(int i) const {
    return sym_table.at(formals[i]);
}
//
unsigned int SymT::get_frmls_size(void) const {
    return formals.size();
}

// ********
//
// class Defn typechecking
//
// Support in Defn for checking function and procedure calls.
// 

unsigned int Defn::arity(void) const {
    return symt.get_frmls_size();
}

Type Defn::returns(void) const {
    return rety;
}

SymInfo_ptr Defn::formal(int i) const {
    return symt.get_frml(i);
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

// ********
//
// AST::chck - check Snek syntax to see if it is "type copacetic"
//

//
// Prgm::chck method
//
// - This implements the "|- d1 ... dk M ok" relation.
//   It checks each function definition di and also the main block M.
//
void Prgm::chck(void) {
    
    // Check each function definition. 
    for (std::pair<Name,Defn_ptr> dfpr : defs) {
        dfpr.second->chck(defs);
    }
    
    // Check the main block.
    Rtns rtns = main->chck(Rtns{Void {}},defs,main_symt);

    // Complain that there are return statements.
    if (!std::holds_alternative<Void>(rtns)) {
        SnekError(main->where(), "Main script should not return.");
    }
    
}

//
// Defn::chck method
// 
// - This implements the "D |- d ok" relation.
//   It checks that a function definition uses its formal parameters
//   and local variables correctly, and that it properly returns the
//   correct type of value in all cases.
//
// defs: the collection of function definition (signatures) i.e. Delta
//
void Defn::chck(Defs& defs) {

    // Check the body.
    Rtns rtns = body->chck(Rtns{rety}, defs, symt);

    // Make sure it returns in all cases.
    if (std::holds_alternative<Void>(rtns) && rety != UnitTy {}) {
        throw SnekError(body->where(), "Function body never returns.");
    }
    if (std::holds_alternative<VoidOr>(rtns)) {
        if (rety == UnitTy {}) {
            throw SnekError(body->where(), "Procedure body might return a value.");
        } else {
            throw SnekError(body->where(), "Function body might not return in some cases.");
        }
    }
}

//
// Blck::chck and Stmt::chck methods
//
// - These implement the "D;G |- s1...sk A r" relation.
//   They each check that Snek statements type well.
//   It also determines their return behavior.
//
// expd: the return behavior expected within this block
// defs: the collection of function definition (signatures) i.e. Delta
// symt: the stack of variables active (within scope) i.e. Gamma
// return value: the actual return behavior i.e. rho
//

Rtns Blck::chck(Rtns expd, Defs& defs, SymT& symt) {

    // Add a new "scope level" to the symbol table's stack.
    symt.push_scope();

    // ***
    // FIX THIS!
    
    // Scan through the statements and check their return behavior.
    for (Stmt_ptr stmt : stmts) {

        // Check this statement.
        [[maybe_unused]] Rtns stmt_rtns = stmt->chck(expd, defs, symt);
    }
    
    // FIX THIS!
    // *** 
    
    // Okay, now deactivate this block's level of scope.
    symt.pop_scope();

    return Rtns {Void {}}; // FIX THIS!
}

Rtns Asgn::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    
    // ***
    // FIX THIS!
    // ***
    (void)defs;
    (void)symt;
    
    return Rtns {Void {}}; 
}

Rtns Pass::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs,
                [[maybe_unused]] SymT& symt) {

    // Nothing to check. Falls through.
    return Rtns {Void {}};
}

Rtns Prnt::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {

    for (Expn_ptr e : exps) {
        // Check the sub-expression.
        [[maybe_unused]] Type t = e->chck(defs,symt);
    }

    // Falls through.
    return Rtns {Void {}};
}

Rtns Ntro::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs, SymT& symt) {
    Type t = expn->chck(defs, symt);
    name_info = symt.add_locl(name, t);
    if (t != type) {
        throw SnekError {where(), "Introduced variable's type does not match its initialization."};
    }
    return Rtns {Void {}}; 
}

Rtns PlEq::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs, SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)symt;

    return Rtns{Void {}};
}

Rtns MnEq::chck([[maybe_unused]] Rtns expd,
                [[maybe_unused]] Defs& defs, SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)symt;

    return Rtns{Void {}};
}

Rtns FRtn::chck(Rtns expd,
                [[maybe_unused]]Defs& defs, SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)symt;
    (void)expd;

    // Returns the expected type.
    return expd;
}

Rtns PRtn::chck(Rtns expd,
                [[maybe_unused]] Defs& defs, [[maybe_unused]] SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)expd;

    // Returns None.
    return Rtns {Type {UnitTy {}}};
}

Rtns PCll::chck([[maybe_unused]] Rtns expd, Defs& defs, SymT& symt) {
    if (defs.count(name) == 0) {
        throw SnekError {
            where(), "Unknown procedure '" + name +"'."
        };
    }
    Defn_ptr def = defs.at(name);

    // ***
    // FIX THIS!
    // ***
    (void)symt;
    
    return Rtns {Void {}};
}

Rtns IfEl::chck(Rtns expd, Defs& defs, SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)symt;
    (void)defs;
    (void)expd;

    return Rtns {Void {}}; // FIX THIS!
}

Rtns Whle::chck(Rtns expd, Defs& defs, SymT& symt) {

    // ***
    // FIX THIS!
    // ***
    (void)symt;
    (void)defs;
    (void)expd;

    return Rtns {Void {}}; // FIX THIS!
}

//
// Expn::chck methods
//
// - These implement the "D;G |- e : t" relation.
//   They each check that Snek expressions type well.
//   They return the type of their value.
//
// defs: the collection of function definition (signatures) i.e. Delta
// symt: the stack of variables active (within scope) i.e. Gamma
// return value: the expression's value type i.e. tau
//

Type FCll::chck(const Defs& defs, const SymT& symt) {
    if (defs.count(name) == 0) {
        throw SnekError {
            where(), "Unknown function '" + name +"'."
        };
    }
    Defn_ptr def = defs.at(name);

    // ***
    // FIX THIS!
    // ***
    (void)symt;

    return def->returns();
        
}

Type BiOp::chck(const Defs& defs, const SymT& symt) {

    // Check the left and the right expressions.
    Type left_ty = left->chck(defs,symt);
    Type rght_ty = rght->chck(defs,symt);
    
    // Look up the signature tau1 x tau2 -> tau.
    std::string t1 = type_name(left_ty);
    std::string t2 = type_name(rght_ty);
    std::string t1_t2 = t1 + "*" + t2;
    if (sigs.count(t1_t2) == 0) {
        std::string msg = "Wrong operand types for `" + oper + "` operation.";
        throw SnekError { where(), msg };
    }
    std::string t = sigs.at(t1_t2).first;

    // Return the type.
    return type_of_name(t);

}

Type Ltrl::chck([[maybe_unused]] const Defs& defs, [[maybe_unused]] const SymT& symt) {

    // Look up the literal's type. Return it.
    //
    if (std::holds_alternative<int>(valu)) {
        return Type {IntTy {}};
    } else if (std::holds_alternative<std::string>(valu)) {
        return Type {StrTy {}};
    } if (std::holds_alternative<bool>(valu)) {
        return Type {BoolTy {}};
    } else {
        return Type {UnitTy {}};
    }
    
}

Type Lkup::chck([[maybe_unused]] const Defs& defs, const SymT& symt) {

    // Lookup the name, see if it is in scope.
    SymInfo_ptr info = symt.lookup_in_scope(name);

    // If not in scope, issue the appropriate error.
    if (info == nullptr) {
        
        // Check if defined in the block but no longer in scope. 
        if (symt.has_info(name)) {
            throw SnekError(where(),
                            "Variable '" + name + "' no longer in scope.");
        } else {
            throw SnekError(where(),
                            "Variable '" + name + "' never introduced.");
        }
    }

    // Set its SymT data for future checks.
    name_info = info;
    
    // Return its type.
    return info->type;
    
}

Type Inpt::chck(const Defs& defs, const SymT& symt) {
    [[maybe_unused]] Type ty = prpt->chck(defs, symt);
    return Type {StrTy {}}; 
}

Type IntC::chck(const Defs& defs, const SymT& symt) {
    [[maybe_unused]] Type ty = expn->chck(defs, symt);
    return Type {IntTy {}}; 
}

Type StrC::chck(const Defs& defs, const SymT& symt) {
    [[maybe_unused]] Type ty = expn->chck(defs, symt);
    return Type {StrTy {}}; 
}

Type SLen::chck(const Defs& defs, const SymT& symt) {
    Type ty = expn->chck(defs, symt);
    if (!is_str(ty)) {
        throw SnekError {where(), "Type mismatch for string length. Expected `str` but given `" + type_name(ty) + "`."};
    }
    return Type {IntTy {}};
}

Type LNeg::chck(const Defs& defs, const SymT& symt) {
    Type ty = expn->chck(defs, symt);
    if (!is_bool(ty)) {
        throw SnekError {where(), "Type mismatch for `not`. Expected `bool` but given `" + type_name(ty) + "`."};
    }
    return Type {BoolTy {}};
}

Type INeg::chck(const Defs& defs, const SymT& symt) {
    Type ty = expn->chck(defs, symt);
    if (!is_int(ty)) {
        throw SnekError {where(), "Type mismatch for unary `-`. Expected `int` but given `" + type_name(ty) + "`."};
    }
    return Type {IntTy {}};
}
