%skeleton "lalr1.cc"
%require  "2.3"
%debug 
%defines 
%define api.namespace {Snek}
%define api.parser.class {Parser}
%define parse.error verbose
    
%code requires{
    
    #include "snek-ast.hh"
    #include "snek-check.hh"

    namespace Snek {
        class Driver;
        class Lexer;
    }

    #define YY_NULLPTR nullptr

}

%parse-param { Lexer  &lexer }
%parse-param { Driver &main  }
    
%code{

    #include <sstream>
    #include "snek-util.hh"    
    #include "snki.hh"

    #undef yylex
    #define yylex lexer.yylex
    
}


%define api.value.type variant
%define parse.assert
%define api.token.prefix {Token_}

%locations

%token               EOFL  0  
%token               EOLN
%token               INDT
%token               DEDT
%token               DEFN "def"
%token               PASS "pass"
%token               PRNT "print"
%token               IFTN "if"
%token               ELSE "else"
%token               WHLE "while"
%token               RTRN "return"
%token               INPT "input"
%token               SLEN "len"
%token               BOOL "bool"
%token               UNIT "unit"
%token               INTC "int"
%token               STRC "str"
%token               ASGN "="
%token               PLEQ "+="
%token               MNEQ "-="
%token               PLUS "+"
%token               MNUS "-"
%token               TMES "*"
%token               IDIV "//"
%token               IMOD "%"
%token               CONJ "and"
%token               DISJ "or"
%token               LNEG "not"
%token               LESS "<"
%token               LSEQ "<="
%token               EQUL "=="
%token               LPAR "(" 
%token               RPAR ")"
%token               CMMA ","
%token               COLN ":"
%token               ARRW "->"
%token               NONE "None"
%token               TRUE "True"
%token               FALS "False"
%token <int>         NMBR
%token <std::string> NAME
%token <std::string> STRG

%type <Prgm_ptr> prgm
%type <Blck_ptr> blck
%type <Blck_ptr> nest
%type <Stmt_vec> stms
%type <Stmt_ptr> stmt
%type <Expn_vec> exps
%type <Expn_ptr> expn
%type <Defs>     defs
%type <Defn_ptr> defn
%type <SymT>     fmls
%type <Type>     type

%%

%start main;

%left DISJ;
%left CONJ;
%precedence LNEG;
%nonassoc LESS LSEQ EQUL;
%left PLUS MNUS;
%left TMES IMOD IDIV;
%precedence INEG;
    
main:
  prgm {
      main.set($1);
  }
;

prgm:
blck {
                      // Hack:
      (void)yynerrs_; // This is to get rid of the "variable set but not used"
                      // warning from the compiler.
      Defs ds { };
      Blck_ptr b = $1; 
      $$ = Prgm_ptr { new Prgm {ds, b, b->where()} };
  }   
| defs blck {
                      // Hack:
      (void)yynerrs_; // This is to get rid of the "variable set but not used"
                      // warning from the compiler.
      Defs     ds = $1;
      Blck_ptr b  = $2; 
      $$ = Prgm_ptr { new Prgm {ds, b, b->where()} };
  }
;

defs: defn {
      Defs ds { };
      Defn_ptr d = $1;
      ds[d->name] = d;
      $$ = ds;
  }
| defs defn {
      Defs ds = $1;
      Defn_ptr d = $2;
      ds[d->name] = d;
      $$ = ds;
  }
;

defn:
  DEFN NAME LPAR RPAR ARRW type COLN EOLN nest {
    SymT ps { };
    $$ = Defn_ptr { new Defn {$2, ps, $6, $9, lexer.locate(@1)} };
  }    
| DEFN NAME LPAR RPAR COLN EOLN nest {
    SymT ps { };
    $$ = Defn_ptr { new Defn {$2, ps, UnitTy {}, $7, lexer.locate(@1)} };
  }    
| DEFN NAME LPAR fmls RPAR ARRW type COLN EOLN nest {
    $$ = Defn_ptr { new Defn {$2, $4, $7, $10, lexer.locate(@1)} };
  }
| DEFN NAME LPAR fmls RPAR COLN EOLN nest {
    $$ = Defn_ptr { new Defn {$2, $4, UnitTy {}, $8, lexer.locate(@1)} };
  }
;

fmls:
  NAME COLN type {
    SymT ps { };
    ps.add_frml($1,$3);
    $$ = ps;
  }
| fmls CMMA NAME COLN type {
    SymT ps = $1;
    ps.add_frml($3,$5);
    $$ = ps;
  }
;

type:
  INTC {
    $$ = IntTy {};
  }
| STRC {
    $$ = StrTy {};
  }
| BOOL {
    $$ = BoolTy {};
  }
| UNIT {
    $$ = UnitTy {};
  }
;
        
nest:
  INDT blck DEDT {
    $$ = $2;
  }
;

blck:
  stms {
      Stmt_vec ss = $1;
      $$ = Blck_ptr { new Blck {ss, ss[0]->where()} };
  }
;

stms:
  stms stmt {
      Stmt_vec ss = $1;
      ss.push_back($2);
      $$ = ss;
  }
| stmt {
      Stmt_vec ss { };
      ss.push_back($1);
      $$ = ss;
  }
;
  
stmt: 
  NAME ASGN expn EOLN {
      $$ = std::shared_ptr<Asgn> { new Asgn {$1,$3,lexer.locate(@2)} };
  }
| NAME COLN type ASGN expn EOLN {
      $$ = std::shared_ptr<Ntro> { new Ntro {$1,$3,$5,lexer.locate(@4)} };
  }
| NAME PLEQ expn EOLN {
      $$ = std::shared_ptr<PlEq> { new PlEq {$1,$3,lexer.locate(@2)} };
  }
| NAME MNEQ expn EOLN {
      $$ = std::shared_ptr<MnEq> { new MnEq {$1,$3,lexer.locate(@2)} };
  }
| PASS EOLN {
      $$ = std::shared_ptr<Pass> { new Pass {lexer.locate(@1)} };
  }
| PRNT LPAR exps RPAR EOLN {
      $$ = std::shared_ptr<Prnt> { new Prnt {$3,lexer.locate(@1)} };
  }
| IFTN expn COLN EOLN nest ELSE COLN EOLN nest {
      $$ = std::shared_ptr<IfEl> { new IfEl {$2,$5,$9,lexer.locate(@1)} };
  }
| WHLE expn COLN EOLN nest {
      $$ = std::shared_ptr<Whle> { new Whle {$2,$5,lexer.locate(@1)} };
  }
| NAME LPAR exps RPAR EOLN {
      $$ = std::shared_ptr<PCll> { new PCll {$1,$3,lexer.locate(@1)} };
  }
| RTRN expn EOLN {
      $$ = std::shared_ptr<FRtn> { new FRtn {$2,lexer.locate(@1)} };
  }
| RTRN EOLN {
      $$ = std::shared_ptr<PRtn> { new PRtn {lexer.locate(@1)} };
  }
;

expn:
  expn PLUS expn {
      $$ = std::shared_ptr<Plus> { new Plus {$1,$3,lexer.locate(@2)} };
  }
| expn MNUS expn {
      $$ = std::shared_ptr<Mnus> { new Mnus {$1,$3,lexer.locate(@2)} };
  }
| expn TMES expn {
      $$ = std::shared_ptr<Tmes> { new Tmes {$1,$3,lexer.locate(@2)} };
  }
| expn IDIV expn {
      $$ = std::shared_ptr<IDiv> { new IDiv {$1,$3,lexer.locate(@2)} };
  }
| expn IMOD expn {
      $$ = std::shared_ptr<IMod> { new IMod {$1,$3,lexer.locate(@2)} };
  }
| expn DISJ expn {
      $$ = std::shared_ptr<Disj> { new Disj {$1,$3,lexer.locate(@2)} };
  }
| expn CONJ expn {
      $$ = std::shared_ptr<Conj> { new Conj {$1,$3,lexer.locate(@2)} };
  }
| expn LESS expn {
      $$ = std::shared_ptr<Less> { new Less {$1,$3,lexer.locate(@2)} };
  }
| expn LSEQ expn {
      $$ = std::shared_ptr<LsEq> { new LsEq {$1,$3,lexer.locate(@2)} };
  }
| expn EQUL expn {
      $$ = std::shared_ptr<Equl> { new Equl {$1,$3,lexer.locate(@2)} };
  }
| INPT LPAR expn RPAR {
      $$ = std::shared_ptr<Inpt> { new Inpt {$3,lexer.locate(@1)} };
  }
| INTC LPAR expn RPAR {
      $$ = std::shared_ptr<IntC> { new IntC {$3,lexer.locate(@1)} };
  }
| STRC LPAR expn RPAR {
      $$ = std::shared_ptr<StrC> { new StrC {$3,lexer.locate(@1)} };
  }
| SLEN LPAR expn RPAR {
      $$ = std::shared_ptr<SLen> { new SLen {$3,lexer.locate(@1)} };
  }
| LNEG expn {
      $$ = std::shared_ptr<LNeg> { new LNeg {$2,lexer.locate(@1)} };
  }
| MNUS expn %prec INEG {
      $$ = std::shared_ptr<INeg> { new INeg {$2,lexer.locate(@1)} };
  }
| NAME LPAR exps RPAR {
      $$ = std::shared_ptr<FCll> { new FCll {$1,$3,lexer.locate(@1)} };
  }
| NAME {
      $$ = std::shared_ptr<Lkup> { new Lkup {$1,lexer.locate(@1)} };
  }
| NMBR {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {$1},lexer.locate(@1)} };
  }
| STRG {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {de_escape($1)},lexer.locate(@1)} };
  }
| TRUE {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {true},lexer.locate(@1)} };
  }
| FALS {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {false},lexer.locate(@1)} };
  }
| NONE {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {None},lexer.locate(@1)} };
  }    
| LPAR expn RPAR {
      $$ = $2;
  }
;

exps:
  %empty {
    Expn_vec ps { };
    $$ = ps;
  }
|
  expn {
    Expn_vec ps { };
    ps.push_back($1);
    $$ = ps;
  }
| exps CMMA expn {
    Expn_vec ps = $1;
    ps.push_back($3);
    $$ = ps;
  }
;

%%
       
void Snek::Parser::error(const location_type &loc, const std::string &msg) {
    throw SnekError { lexer.locate(loc), msg };
}
