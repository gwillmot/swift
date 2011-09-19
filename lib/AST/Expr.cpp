//===--- Expr.cpp - Swift Language Expression ASTs ------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file implements the Expr class and subclasses.
//
//===----------------------------------------------------------------------===//

#include "swift/AST/Expr.h"
#include "swift/AST/ASTVisitor.h"
#include "swift/AST/Types.h"
#include "swift/AST/ASTContext.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/raw_ostream.h"
using namespace swift;

//===----------------------------------------------------------------------===//
// Expr methods.
//===----------------------------------------------------------------------===//

// Only allow allocation of Stmts using the allocator in ASTContext.
void *Expr::operator new(size_t Bytes, ASTContext &C,
                         unsigned Alignment) throw() {
  return C.Allocate(Bytes, Alignment);
}

/// getLocStart - Return the location of the start of the expression.
/// FIXME: Need to extend this to do full source ranges like Clang.
SMLoc Expr::getStartLoc() const {
  switch (Kind) {
  case ExprKind::IntegerLiteral:
    return cast<IntegerLiteralExpr>(this)->getStartLoc();
  case ExprKind::FloatLiteral:
    return cast<FloatLiteralExpr>(this)->getStartLoc();
  case ExprKind::DeclRef:
    return cast<DeclRefExpr>(this)->getStartLoc();
  case ExprKind::OverloadSetRef:
    return cast<OverloadSetRefExpr>(this)->getStartLoc();
  case ExprKind::UnresolvedDeclRef:
    return cast<UnresolvedDeclRefExpr>(this)->getStartLoc();
  case ExprKind::UnresolvedMember:
    return cast<UnresolvedMemberExpr>(this)->getStartLoc();
  case ExprKind::UnresolvedScopedIdentifier:
    return cast<UnresolvedScopedIdentifierExpr>(this)->getStartLoc();
  case ExprKind::Tuple:
    return cast<TupleExpr>(this)->getStartLoc();
  case ExprKind::UnresolvedDot:
    return cast<UnresolvedDotExpr>(this)->getStartLoc();
  case ExprKind::TupleElement:
    return cast<TupleElementExpr>(this)->getStartLoc();
  case ExprKind::TupleShuffle:
    return cast<TupleShuffleExpr>(this)->getStartLoc();
  case ExprKind::Call:
    return cast<CallExpr>(this)->Fn->getStartLoc();
  case ExprKind::Sequence:
    return cast<SequenceExpr>(this)->Elements[0]->getStartLoc();
  case ExprKind::Func:
    return cast<FuncExpr>(this)->FuncLoc;
  case ExprKind::Closure:
    return cast<ClosureExpr>(this)->Input->getStartLoc();
  case ExprKind::AnonClosureArg:
    return cast<AnonClosureArgExpr>(this)->Loc;
  case ExprKind::Unary:
    return cast<UnaryExpr>(this)->Fn->getStartLoc();
  case ExprKind::Binary:
    return cast<BinaryExpr>(this)->Arg->getStartLoc();
  case ExprKind::ProtocolElement:
    return cast<ProtocolElementExpr>(this)->Arg->getStartLoc();
  }
  
  llvm_unreachable("expression type not handled!");
}

/// getLoc - Return the caret location of the expression.
SMLoc Expr::getLoc() const {
  switch (Kind) {
  case ExprKind::IntegerLiteral:
    return cast<IntegerLiteralExpr>(this)->getLoc();
  case ExprKind::FloatLiteral:
    return cast<FloatLiteralExpr>(this)->getLoc();
  case ExprKind::DeclRef:
    return cast<DeclRefExpr>(this)->getLoc();
  case ExprKind::OverloadSetRef:
    return cast<OverloadSetRefExpr>(this)->getLoc();
  case ExprKind::UnresolvedDeclRef:
    return cast<UnresolvedDeclRefExpr>(this)->getLoc();
  case ExprKind::UnresolvedMember:
    return cast<UnresolvedMemberExpr>(this)->getLoc();
  case ExprKind::UnresolvedScopedIdentifier:
    return cast<UnresolvedScopedIdentifierExpr>(this)->getLoc();
  case ExprKind::Tuple:
    return cast<TupleExpr>(this)->getLoc();
  case ExprKind::UnresolvedDot:
    return cast<UnresolvedDotExpr>(this)->getLoc();
  case ExprKind::TupleElement:
    return cast<TupleElementExpr>(this)->getLoc();
  case ExprKind::TupleShuffle:
    return cast<TupleShuffleExpr>(this)->getLoc();
  case ExprKind::Call:
    return cast<CallExpr>(this)->Arg->getStartLoc();
  case ExprKind::Sequence:
    return cast<SequenceExpr>(this)->Elements[0]->getLoc();
  case ExprKind::Func:
    return cast<FuncExpr>(this)->FuncLoc;
  case ExprKind::Closure:
    return cast<ClosureExpr>(this)->Input->getLoc();
  case ExprKind::AnonClosureArg:
    return cast<AnonClosureArgExpr>(this)->Loc;
  case ExprKind::Unary:
    return cast<UnaryExpr>(this)->Fn->getLoc();
  case ExprKind::Binary:
    return cast<BinaryExpr>(this)->Fn->getLoc();
  case ExprKind::ProtocolElement:
    return cast<ProtocolElementExpr>(this)->DotLoc;
  }
  
  llvm_unreachable("expression type not handled!");
}

//===----------------------------------------------------------------------===//
// Support methods for Exprs.
//===----------------------------------------------------------------------===//

/// getNumArgs - Return the number of arguments that this closure expr takes.
/// This is the length of the ArgList.
unsigned ClosureExpr::getNumArgs() const {
  Type Input = getType()->getAs<FunctionType>()->Input;
  
  if (TupleType *TT = Input->getAs<TupleType>())
    return TT->Fields.size();
  return 1;  
}

uint64_t IntegerLiteralExpr::getValue() const {
  unsigned long long IntVal;
  bool Error = Val.getAsInteger(0, IntVal);
  assert(!Error && "Invalid IntegerLiteral formed"); (void)Error;
  return IntVal;
}

static ValueDecl *getCalledValue(Expr *E) {
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E))
    return DRE->getDecl();

  if (TupleExpr *TE = dyn_cast<TupleExpr>(E))
    if (TE->isGroupingParen())
      return getCalledValue(TE->getElement(0));

  return nullptr;
}

ValueDecl *ApplyExpr::getCalledValue() const {
  return ::getCalledValue(Fn);
}

//===----------------------------------------------------------------------===//
//  Type Conversion Ranking
//===----------------------------------------------------------------------===//

/// convertTupleToTupleType - Given an expression that has tuple type, convert
/// it to have some other tuple type.
///
/// The caller gives us a list of the expressions named arguments and a count of
/// tuple elements for E in the IdentList+NumIdents array.  DestTy specifies the
/// type to convert to, which is known to be a TupleType.
static Expr::ConversionRank 
getTupleToTupleTypeConversionRank(const Expr *E, unsigned NumExprElements,
                                  TupleType *DestTy, ASTContext &Ctx) {
  // If the tuple expression or destination type have named elements, we
  // have to match them up to handle the swizzle case for when:
  //   (.y = 4, .x = 3)
  // is converted to type:
  //   (.x = int, .y = int)
  SmallVector<Identifier, 8> IdentList(NumExprElements);
  
  // Check to see if this conversion is ok by looping over all the destination
  // elements and seeing if they are provided by the input.
  
  // Keep track of which input elements are used.
  SmallVector<bool, 16> UsedElements(NumExprElements);
  SmallVector<int, 16>  DestElementSources(DestTy->Fields.size(), -1);

  if (TupleType *ETy = E->getType()->getAs<TupleType>()) {
    assert(ETy->Fields.size() == NumExprElements && "Expr #elements mismatch!");
    { unsigned i = 0;
    for (const TupleTypeElt &Elt : ETy->Fields)
      IdentList[i++] = Elt.Name;
    }
  
    // First off, see if we can resolve any named values from matching named
    // inputs.
    for (unsigned i = 0, e = DestTy->Fields.size(); i != e; ++i) {
      const TupleTypeElt &DestElt = DestTy->Fields[i];
      // If this destination field is named, first check for a matching named
      // element in the input, from any position.
      if (DestElt.Name.empty()) continue;
      
      int InputElement = -1;
      for (unsigned j = 0; j != NumExprElements; ++j)
        if (IdentList[j] == DestElt.Name) {
          InputElement = j;
          break;
        }
      if (InputElement == -1) continue;
      
      DestElementSources[i] = InputElement;
      UsedElements[InputElement] = true;
    }
  }
  
  // Next step, resolve (in order) unmatched named results and unnamed results
  // to any left-over unnamed input.
  unsigned NextInputValue = 0;
  for (unsigned i = 0, e = DestTy->Fields.size(); i != e; ++i) {
    // If we already found an input to satisfy this output, we're done.
    if (DestElementSources[i] != -1) continue;
    
    // Scan for an unmatched unnamed input value.
    while (1) {
      // If we didn't find any input values, we ran out of inputs to use.
      if (NextInputValue == NumExprElements)
        break;
      
      // If this input value is unnamed and unused, use it!
      if (!UsedElements[NextInputValue] && IdentList[NextInputValue].empty())
        break;
      
      ++NextInputValue;
    }
    
    // If we ran out of input values, we either don't have enough sources to
    // fill the dest (as in when assigning (1,2) to (int,int,int), or we ran out
    // and default values should be used.
    if (NextInputValue == NumExprElements) {
      if (DestTy->Fields[i].Init == 0)
        return Expr::CR_Invalid;
        
      // If the default initializer should be used, leave the
      // DestElementSources field set to -2.
      DestElementSources[i] = -2;
      continue;
    }
    
    // Okay, we found an input value to use.
    DestElementSources[i] = NextInputValue;
    UsedElements[NextInputValue] = true;
  }
  
  // If there were any unused input values, we fail.
  for (bool Elt : UsedElements)
    if (!Elt)
      return Expr::CR_Invalid;
  
  // It looks like the elements line up, walk through them and see if the types
  // either agree or can be converted.  If the expression is a TupleExpr, we do
  // this conversion in place.
  const TupleExpr *TE = dyn_cast<TupleExpr>(E);
  if (TE && TE->getNumElements() != 1 &&
      TE->getNumElements() == DestTy->Fields.size()) {
    Expr::ConversionRank CurRank = Expr::CR_Identity;
    
    // The conversion rank of the tuple is the worst case of the conversion rank
    // of each of its elements.
    for (unsigned i = 0, e = DestTy->Fields.size(); i != e; ++i) {
      // Extract the input element corresponding to this destination element.
      unsigned SrcField = DestElementSources[i];
      assert(SrcField != ~0U && "dest field not found?");
      
      // If SrcField is -2, then the destination element just uses its default
      // value.
      if (SrcField == -2U)
        continue;
     
      // Check to see if the src value can be converted to the destination
      // element type.
      Expr *Elt = TE->getElement(SrcField);
      CurRank = std::max(CurRank,
                         Elt->getRankOfConversionTo(DestTy->getElementType(i),
                                                    Ctx));
    }
    return CurRank;
  }
  
  // A tuple-to-tuple conversion of a non-parenthesized tuple is allowed to
  // permute the elements, but cannot perform conversions of each value.
  TupleType *ETy = E->getType()->getAs<TupleType>();
  for (unsigned i = 0, e = DestTy->Fields.size(); i != e; ++i) {
    // Extract the input element corresponding to this destination element.
    unsigned SrcField = DestElementSources[i];
    assert(SrcField != ~0U && "dest field not found?");

    // If SrcField is -2, then the destination element just uses its default
    // value.
    if (SrcField == -2U)
      continue;

    // The element types must match up exactly.
    if (ETy->getElementType(SrcField)->getCanonicalType(Ctx) !=
        DestTy->getElementType(i)->getCanonicalType(Ctx))
      return Expr::CR_Invalid;
  }

  return Expr::CR_Identity;
}


/// getConversionRank - Return the conversion rank for converting a value 'E' to
/// type 'ToTy'.
///
/// Note that this code needs to be kept carefully in synch with
/// SemaCoerceBottomUp::convertToType.
static Expr::ConversionRank 
getConversionRank(const Expr *E, Type DestTy, ASTContext &Ctx) {
  assert(!DestTy->is<DependentType>() &&
         "Result of conversion can't be dependent");

  // Exact matches are identity conversions.
  if (E->getType()->getCanonicalType(Ctx) == DestTy->getCanonicalType(Ctx))
    return Expr::CR_Identity;
  
  // If the expression is a grouping parenthesis, then it is an identity
  // conversion of the underlying expression.
  if (const TupleExpr *TE = dyn_cast<TupleExpr>(E))
    if (TE->isGroupingParen())
      return getConversionRank(TE->getElement(0), DestTy, Ctx);
  
  if (TupleType *TT = DestTy->getAs<TupleType>()) {
    if (const TupleExpr *TE = dyn_cast<TupleExpr>(E))
      return getTupleToTupleTypeConversionRank(TE, TE->getNumElements(),
                                               TT, Ctx);
    
    // If the is a scalar to tuple conversion, form the tuple and return it.
    int ScalarFieldNo = TT->getFieldForScalarInit();
    if (ScalarFieldNo != -1) {
      // If the destination is a tuple type with at most one element that has no
      // default value, see if the expression's type is convertable to the
      // element type.  This handles assigning 4 to "(a = 4, b : int)".
      return getConversionRank(E, TT->getElementType(ScalarFieldNo), Ctx);
    }
    
    // If the input is a tuple and the output is a tuple, see if we can convert
    // each element.
    if (TupleType *ETy = E->getType()->getAs<TupleType>())
      return getTupleToTupleTypeConversionRank(E, ETy->Fields.size(), TT, Ctx);
  }

  // Otherwise, check to see if this is an auto-closure case.  This case happens
  // when we convert an expression E to a function type whose result is E's
  // type.
  if (FunctionType *FT = DestTy->getAs<FunctionType>()) {
    if (getConversionRank(E, FT->Result, Ctx) == Expr::CR_Invalid)
      return Expr::CR_Invalid;
    
    return Expr::CR_AutoClosure;
  }

  // If the expression has a dependent type or we have some other case, we fail.
  return Expr::CR_Invalid;
}

/// getRankOfConversionTo - Return the rank of a conversion from the current
/// type to the specified type.
Expr::ConversionRank 
Expr::getRankOfConversionTo(Type DestTy, ASTContext &Ctx) const {
  return getConversionRank(this, DestTy, Ctx);
}



//===----------------------------------------------------------------------===//
// Expression Walking
//===----------------------------------------------------------------------===//

namespace {
  /// ExprWalker - This class implements a simple expression walker which
  /// invokes a function pointer on every expression in an AST.  If the function
  /// pointer returns true the walk is terminated.
  class ExprWalker : public ASTVisitor<ExprWalker, Expr*, Stmt*> {
    friend class ASTVisitor<ExprWalker, Expr*, Stmt*>;
    Expr *(^ExprFn)(Expr *E, WalkOrder Order);
    Stmt *(^StmtFn)(Stmt *S, WalkOrder Order);
    
    
    Expr *visitIntegerLiteralExpr(IntegerLiteralExpr *E) { return E; }
    Expr *visitFloatLiteralExpr(FloatLiteralExpr *E) { return E; }
    Expr *visitDeclRefExpr(DeclRefExpr *E) { return E; }
    Expr *visitOverloadSetRefExpr(OverloadSetRefExpr *E) { return E; }
    Expr *visitUnresolvedDeclRefExpr(UnresolvedDeclRefExpr *E) { return E; }
    Expr *visitUnresolvedMemberExpr(UnresolvedMemberExpr *E) { return E; }
    Expr *visitUnresolvedScopedIdentifierExpr(UnresolvedScopedIdentifierExpr*E){
      return E;
    }
    
    Expr *visitTupleExpr(TupleExpr *E) {
      for (unsigned i = 0, e = E->getNumElements(); i != e; ++i)
        if (E->getElement(i)) {
          if (Expr *Elt = doIt(E->getElement(i)))
            E->setElement(i, Elt);
          else
            return 0;
        }
      return E;
    }
    Expr *visitUnresolvedDotExpr(UnresolvedDotExpr *E) {
      if (!E->getBase())
        return E;
      
      if (Expr *E2 = doIt(E->getBase())) {
        E->setBase(E2);
        return E;
      }
      return 0;
    }
    
    Expr *visitTupleElementExpr(TupleElementExpr *E) {
      if (Expr *E2 = doIt(E->getBase())) {
        E->setBase(E2);
        return E;
      }
      return 0;
    }
    
    Expr *visitTupleShuffleExpr(TupleShuffleExpr *E) {
      if (Expr *E2 = doIt(E->getSubExpr())) {
        E->setSubExpr(E2);
        return E;
      }
      return 0;
    }
    
    Expr *visitSequenceExpr(SequenceExpr *E) {
      for (unsigned i = 0, e = E->NumElements; i != e; ++i)
        if (Expr *Elt = doIt(E->Elements[i]))
          E->Elements[i] = Elt;
        else
          return 0;
      return E;
    }
    
    Expr *visitFuncExpr(FuncExpr *E) {
      if (BraceStmt *S = cast_or_null<BraceStmt>(doIt(E->Body))) {
        E->Body = S;
        return E;
      }
      return 0;
    }
    
    Expr *visitClosureExpr(ClosureExpr *E) {
      if (Expr *E2 = doIt(E->Input)) {
        E->Input = E2;
        return E;
      }
      return 0;
    }
    
    Expr *visitAnonClosureArgExpr(AnonClosureArgExpr *E) { return E; }

    Expr *visitApplyExpr(ApplyExpr *E) {
      Expr *E2 = doIt(E->Fn);
      if (E2 == 0) return 0;
      E->Fn = E2;
      
      E2 = doIt(E->Arg);
      if (E2 == 0) return 0;
      E->Arg = E2;
      return E;      
    }

    Expr *visitCallExpr(CallExpr *E) {
      return visitApplyExpr(E);
    }

    Expr *visitUnaryExpr(UnaryExpr *E) {
      return visitApplyExpr(E);
    }

    Expr *visitBinaryExpr(BinaryExpr *E) {
      // Visit the arguments to the tuple.
      TupleExpr *Arg = E->getArgTuple();
      assert(Arg->getNumElements() == 2);
      Expr *E2 = doIt(Arg->getElement(0));
      if (E2 == 0) return 0;
      Arg->setElement(0, E2);
      
      E2 = doIt(Arg->getElement(1));
      if (E2 == 0) return 0;
      Arg->setElement(1, E2);
      return E;
    }
    
    Expr *visitProtocolElementExpr(ProtocolElementExpr *E) {
      return visitApplyExpr(E);
    }
    

    Stmt *visitSemiStmt(SemiStmt *SS) {
      return SS;
    }
    
    Stmt *visitAssignStmt(AssignStmt *AS) {
      if (Expr *E = doIt(AS->Dest))
        AS->Dest = E;
      else
        return 0;

      if (Expr *E = doIt(AS->Src))
        AS->Src = E;
      else
        return 0;
      return AS;
    }
    
    Stmt *visitBraceStmt(BraceStmt *BS) {
      for (unsigned i = 0, e = BS->NumElements; i != e; ++i) {
        if (Expr *SubExpr = BS->Elements[i].dyn_cast<Expr*>()) {
          if (Expr *E2 = doIt(SubExpr))
            BS->Elements[i] = E2;
          else
            return 0;
          continue;
        }
        
        if (Stmt *S = BS->Elements[i].dyn_cast<Stmt*>()) {
          if (Stmt *S2 = doIt(S))
            BS->Elements[i] = S2;
          else
            return 0;
          continue;
        }
        Decl *D = BS->Elements[i].get<Decl*>();
        if (ValueDecl *VD = dyn_cast<ValueDecl>(D))
          if (Expr *Init = VD->Init) {
            if (Expr *E2 = doIt(Init))
              VD->Init = E2;
            else
              return 0;
          }
      }
      
      return BS;
    }

    Stmt *visitReturnStmt(ReturnStmt *RS) {
      if (Expr *E = doIt(RS->Result))
        RS->Result = E;
      else
        return 0;
      return RS;
    }
    
    Stmt *visitIfStmt(IfStmt *IS) {
      if (Expr *E2 = doIt(IS->Cond))
        IS->Cond = E2;
      else
        return 0;
      
      if (Stmt *S2 = doIt(IS->Then))
        IS->Then = S2;
      else
        return 0;
      
      if (IS->Else) {
        if (Stmt *S2 = doIt(IS->Else))
          IS->Else = S2;
        else
          return 0;
      }
      return IS;
    }
    
    Stmt *visitWhileStmt(WhileStmt *WS) {
      if (Expr *E2 = doIt(WS->Cond))
        WS->Cond = E2;
      else
        return 0;
      
      if (Stmt *S2 = doIt(WS->Body))
        WS->Body = S2;
      else
        return 0;
      return WS;
    }
       
  public:
    ExprWalker(Expr *(^ExprFn)(Expr *E, WalkOrder Order),
               Stmt *(^StmtFn)(Stmt *S, WalkOrder Order))
      : ExprFn(ExprFn), StmtFn(StmtFn) {
    }
    Expr *doIt(Expr *E) {
      // If no visitor function wants to get called before/after the node, just
      // walk into it.
      if (ExprFn == 0)
        return visit(E);
      
      // Try the preorder visitation.  If it returns null, we just skip entering
      // subnodes of this tree.
      Expr *E2 = ExprFn(E, WalkOrder::PreOrder);
      if (E2 == 0) return E;
      
      if (E) E = visit(E);
      if (E) E = ExprFn(E, WalkOrder::PostOrder);
      return E;
    }
    Stmt *doIt(Stmt *S) {
      // If no visitor function wants to get called before/after the node, just
      // walk into it.
      if (StmtFn == 0)
        return visit(S);
      
      // Try the preorder visitation.  If it returns null, we just skip entering
      // subnodes of this tree.
      Stmt *S2 = StmtFn(S, WalkOrder::PreOrder);
      if (S2 == 0) return S;
      
      if (S) S = visit(S);
      if (S) S = StmtFn(S, WalkOrder::PostOrder);
      return S;
    }
  };
} // end anonymous namespace.

Expr *Expr::walk(Expr *(^ExprFn)(Expr *E, WalkOrder Order),
                 Stmt *(^StmtFn)(Stmt *S, WalkOrder Order)) {
  return ExprWalker(ExprFn, StmtFn).doIt(this);  
}

Stmt *Stmt::walk(Expr *(^ExprFn)(Expr *E, WalkOrder Order),
                 Stmt *(^StmtFn)(Stmt *S, WalkOrder Order)) {
  return ExprWalker(ExprFn, StmtFn).doIt(this);
}



//===----------------------------------------------------------------------===//
// Printing for Expr and all subclasses.
//===----------------------------------------------------------------------===//

namespace {
/// PrintExpr - Visitor implementation of Expr::print.
class PrintExpr : public ExprVisitor<PrintExpr> {
public:
  raw_ostream &OS;
  unsigned Indent;
  
  PrintExpr(raw_ostream &os, unsigned indent) : OS(os), Indent(indent) {
  }
  
  void printRec(Expr *E) {
    Indent += 2;
    if (E)
      visit(E);
    else
      OS.indent(Indent) << "(**NULL EXPRESSION**)";
    Indent -= 2;
  }
  
  /// FIXME: This should use ExprWalker to print children.
  
  void printRec(Decl *D) { D->print(OS, Indent+2); }
  void printRec(Stmt *S) { S->print(OS, Indent+2); }

  void visitIntegerLiteralExpr(IntegerLiteralExpr *E) {
    OS.indent(Indent) << "(integer_literal_expr type='" << E->getType();
    OS << "' value=" << E->getValue() << ')';
  }
  void visitFloatLiteralExpr(FloatLiteralExpr *E) {
    OS.indent(Indent) << "(float_literal_expr type='" << E->getType();
    OS << "' value=" << E->getValue() << ')';
  }
  void visitDeclRefExpr(DeclRefExpr *E) {
    OS.indent(Indent) << "(declref_expr type='" << E->getType();
    OS << "' decl=" << E->getDecl()->Name << ')';
  }
  void visitOverloadSetRefExpr(OverloadSetRefExpr *E) {
    OS.indent(Indent) << "(overloadsetref_expr type='" << E->getType();
    OS << "' decl=" << E->getDecls()[0]->Name << ')';
  }
  void visitUnresolvedDeclRefExpr(UnresolvedDeclRefExpr *E) {
    OS.indent(Indent) << "(unresolved_decl_ref_expr type='" << E->getType();
    OS << "' name=" << E->getName() << ')';
  }
  void visitUnresolvedMemberExpr(UnresolvedMemberExpr *E) {
    OS.indent(Indent) << "(unresolved_member_expr type='" << E->getType();
    OS << "\' name='" << E->getName() << "')";
  }
  void visitUnresolvedScopedIdentifierExpr(UnresolvedScopedIdentifierExpr *E) {
    OS.indent(Indent) << "(unresolved_scoped_identifier_expr base='"
      << E->getBaseTypeFromScope()->Name << "\' name='" << E->getName() << "')";
  }
  void visitTupleExpr(TupleExpr *E) {
    OS.indent(Indent) << "(tuple_expr type='" << E->getType() << '\'';
    for (unsigned i = 0, e = E->getNumElements(); i != e; ++i) {
      OS << '\n';
      if (E->getElement(i))
        printRec(E->getElement(i));
      else
        OS.indent(Indent+2) << "<<tuple element default value>>";
    }
    OS << ')';
  }
  void visitUnresolvedDotExpr(UnresolvedDotExpr *E) {
    OS.indent(Indent) << "(unresolved_dot_expr type='" << E->getType();
    OS << "\' field '" << E->getName().str() << "'";
    if (!E->getResolvedDecls().empty())
      OS << " decl resolved to " << E->getResolvedDecls().size()
         << " candidate(s)!";
    if (E->getBase()) {
      OS << '\n';
      printRec(E->getBase());
    }
    OS << ')';
  }
  void visitTupleElementExpr(TupleElementExpr *E) {
    OS.indent(Indent) << "(tuple_element_expr type='" << E->getType();
    OS << "\' field #" << E->getFieldNumber() << "\n";
    printRec(E->getBase());
    OS << ')';
  }
  void visitTupleShuffleExpr(TupleShuffleExpr *E) {
    OS.indent(Indent) << "(tuple_shuffle type='" << E->getType();
    OS << "' Elements=[";
    for (unsigned i = 0, e = E->getElementMapping().size(); i != e; ++i) {
      if (i) OS << ", ";
      OS << E->getElementMapping()[i];
    }
    OS << "]\n";
    printRec(E->getSubExpr());
    OS << ')';
  }

  void visitSequenceExpr(SequenceExpr *E) {
    OS.indent(Indent) << "(sequence_expr type='" << E->getType() << '\'';
    for (unsigned i = 0, e = E->NumElements; i != e; ++i) {
      OS << '\n';
      printRec(E->Elements[i]);
    }
    OS << ')';
  }
  void visitFuncExpr(FuncExpr *E) {
    OS.indent(Indent) << "(func_expr type='" << E->getType() << "'\n";
    printRec(E->Body);
    OS << ')';
  }
  void visitClosureExpr(ClosureExpr *E) {
    OS.indent(Indent) << "(closure_expr type='" << E->getType() << "'\n";
    printRec(E->Input);
    OS << ')';
  }
  
  void visitAnonClosureArgExpr(AnonClosureArgExpr *E) {
    OS.indent(Indent) << "(anon_closure_arg_expr type='" << E->getType();
    OS << "' ArgNo=" << E->ArgNo << ')';
  }
  
  void visitCallExpr(CallExpr *E) {
    OS.indent(Indent) << "(call_expr type='" << E->getType() << "'\n";
    printRec(E->Fn);
    OS << '\n';
    printRec(E->Arg);
    OS << ')';
  }
  void visitUnaryExpr(UnaryExpr *E) {
    OS.indent(Indent) << "(unary_expr '";
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E->Fn))
      OS << DRE->getDecl()->Name;
    else if (OverloadSetRefExpr *OO = dyn_cast<OverloadSetRefExpr>(E->Fn))
      OS << OO->getDecls()[0]->Name;
    else
      OS << "***UNKNOWN***";
    OS << "' type='" << E->getType() << "'\n";
    printRec(E->Arg);
    OS << ')';
  }
  void visitBinaryExpr(BinaryExpr *E) {
    OS.indent(Indent) << "(binary_expr '";
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E->Fn))
      OS << DRE->getDecl()->Name;
    else if (OverloadSetRefExpr *OO = dyn_cast<OverloadSetRefExpr>(E->Fn))
      OS << OO->getDecls()[0]->Name;
    else
      OS << "***UNKNOWN***";
    OS << "' type='" << E->getType() << "'\n";
    printRec(E->getArgTuple()->getElement(0));
    OS << '\n';
    printRec(E->getArgTuple()->getElement(1));
    OS << ')';
  }
  
  void visitProtocolElementExpr(ProtocolElementExpr *E) {
    OS.indent(Indent) << "(protocol_element_expr type='"
                      << E->getType() << "'\n";
    printRec(E->Fn);
    OS << '\n';
    printRec(E->Arg);
    OS << ')';
  }
};

} // end anonymous namespace.


void Expr::dump() const {
  print(llvm::errs());
  llvm::errs() << '\n';
}

void Expr::print(raw_ostream &OS, unsigned Indent) const {
  PrintExpr(OS, Indent).visit(const_cast<Expr*>(this));
}
