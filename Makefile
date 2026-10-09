UNAME := $(shell uname)
TARGET=snki
CXX=g++
YACCFLAGS=-d -v
ifeq ($(UNAME), Darwin)
	YACC=/opt/homebrew/opt/bison/bin/bison
	LEX=/opt/homebrew/opt/flex/bin/flex
	INCLUDES=-I/opt/homebrew/opt/flex/include
	LDFLAGS=-L/opt/homebrew/opt/flex/lib -L/opt/homebrew/opt/bison/lib -lfl -ly
else
	YACC=bison
	LEX=flex
	INCLUDES=
	LDFLAGS=
endif
CXXFLAGS=-Wall -Wextra -pedantic -Wno-c11-extensions -std=c++17 -g $(INCLUDES)
YACC_YACC=snek-bison.tab.hh location.hh position.hh stack.hh snek-bison.tab.cc snek-bison.output
OBJ=$(SRC:.cc=.o)

all:  $(TARGET)

snki: snek-flex.o snek-bison.tab.o snki.o snek-ast.o snek-check.o snek-util.o 
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

lexer: snek-flex.cc snek-util.hh snek-flex.hh

snek-flex.cc: snek-flex.ll snek-flex.hh snek-util.hh parser
		$(LEX) -o $@ snek-flex.ll

parser: snek-bison.tab.cc snek-bison.tab.hh snek-ast.hh snek-util.hh snek-check.hh snki.hh

snek-bison.tab.cc: snek-bison.yy snek-ast.hh snek-util.hh snek-check.hh snki.hh
		$(YACC) $(YACCFLAGS) snek-bison.yy

%.o: %.cc %.hh
		$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
		touch $(YACC_YACC) snek-flex.cc foo.o foo~ $(TARGET)
		rm -f *~ *.o $(YACC_YACC) snek-flex.cc $(TARGET)
