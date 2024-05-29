//////////////////////////////////////////////////////////////////////
//
//    CodeGenVisitor - Walk the parser tree to do
//                     the generation of code
//
//    Copyright (C) 2020-2030  Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 3
//    of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: José Miguel Rivero (rivero@cs.upc.edu)
//             Computer Science Department
//             Universitat Politecnica de Catalunya
//             despatx Omega.110 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
//////////////////////////////////////////////////////////////////////

#include "CodeGenVisitor.h"
#include "antlr4-runtime.h"

#include "../common/TypesMgr.h"
#include "../common/SymTable.h"
#include "../common/TreeDecoration.h"
#include "../common/code.h"

#include <string>
#include <cstddef>    // std::size_t

// uncomment the following line to enable debugging messages with DEBUG*
// #define DEBUG_BUILD
#include "../common/debug.h"

// using namespace std;

#define RESULT_ADDRESS "_result"

// Constructor
CodeGenVisitor::CodeGenVisitor(TypesMgr       & Types,
                               SymTable       & Symbols,
                               TreeDecoration & Decorations) :
  Types{Types},
  Symbols{Symbols},
  Decorations{Decorations} {
}

// Accessor/Mutator to the attribute currFunctionType
TypesMgr::TypeId CodeGenVisitor::getCurrentFunctionTy() const {
  return currFunctionType;
}

void CodeGenVisitor::setCurrentFunctionTy(TypesMgr::TypeId type) {
  currFunctionType = type;
}

// Methods to visit each kind of node:
//
antlrcpp::Any CodeGenVisitor::visitProgram(AslParser::ProgramContext *ctx) {
  DEBUG_ENTER();
  code my_code;
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  for (auto ctxFunc : ctx->function()) { 
    subroutine subr = visit(ctxFunc);
    my_code.add_subroutine(subr);
  }
  Symbols.popScope();
  DEBUG_EXIT();
  return my_code;
}

antlrcpp::Any CodeGenVisitor::visitFunction(AslParser::FunctionContext *ctx) {
  DEBUG_ENTER();
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  subroutine subr(ctx->ID()->getText());
  codeCounters.reset();

  //Set current function Type
  TypesMgr::TypeId funcType = getTypeDecor(ctx);
  setCurrentFunctionTy(funcType);
  
  //If its return type isn't void, add return parameter
  TypesMgr::TypeId retType = Types.getFuncReturnType(funcType);
  if (not Types.isVoidTy(retType)) {
    bool isArray = Types.isArrayTy(retType);
    subr.add_param(RESULT_ADDRESS, Types.to_string(retType), isArray);
  }

  //Add each parameter
  int nParams = ctx->parameter_decl()->ID().size();
  for (int i = 0; i < nParams; ++i) {
    TypesMgr::TypeId paramTy = getTypeDecor(ctx->parameter_decl()->type(i));
    bool isArray = Types.isArrayTy(paramTy);
    if (isArray) {
      TypesMgr::TypeId paramElemTy = Types.getArrayElemType(paramTy);
      subr.add_param(ctx->parameter_decl()->ID(i)->getText(), Types.to_string(paramElemTy), isArray);
    }
    else subr.add_param(ctx->parameter_decl()->ID(i)->getText(), Types.to_string(paramTy), isArray);
  }

  //Add each declared variable
  std::vector<var> && lvars = visit(ctx->declarations());
  for (auto & onevar : lvars) {
    subr.add_var(onevar);
  }

  //Generate function's instructions
  instructionList && code = visit(ctx->statements());
  code = code || instruction(instruction::RETURN());
  subr.set_instructions(code);
  Symbols.popScope();
  DEBUG_EXIT();
  return subr;
}

antlrcpp::Any CodeGenVisitor::visitReturnStmt(AslParser::ReturnStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  if (ctx->expr()) {
    CodeAttribs  && codAtsE = visit(ctx->expr());
    std::string       addrE = codAtsE.addr;
    instructionList & codeP = codAtsE.code;
    code = codeP;
    TypesMgr::TypeId exprTy = getTypeDecor(ctx->expr());
    TypesMgr::TypeId funcTy = getCurrentFunctionTy();
    TypesMgr::TypeId retTy  = Types.getFuncReturnType(funcTy);

    std::string temp = coerceType(code, retTy, exprTy, addrE);
    code = code || instruction::LOAD(RESULT_ADDRESS, temp);
  }
  code = code || instruction::RETURN();
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(AslParser::FuncCallContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->call());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitProcCall(AslParser::ProcCallContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs  && codAts = visit(ctx->call());
  instructionList & code = codAts.code;
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitCall(AslParser::CallContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  TypesMgr::TypeId funcType = getTypeDecor(ctx->ident());
  TypesMgr::TypeId retType = Types.getFuncReturnType(funcType);
  bool nonVoid = not Types.isVoidTy(retType);

  //Push return (if non-void function)
  if (nonVoid) code = code || instruction::PUSH();

  //For each parameter, add the code generated for it and push the parameter
  int nExpr = ctx->expr().size();
  for (int i = 0; i < nExpr; ++i) {
    CodeAttribs    && codAtsP = visit(ctx->expr(i));
    std::string         addrP = codAtsP.addr;
    instructionList &   codeP = codAtsP.code;

    TypesMgr::TypeId paramTy = Types.getParameterType(funcType, i);
    TypesMgr::TypeId exprTy = getTypeDecor(ctx->expr(i));
    addrP = coerceType(codeP, paramTy, exprTy, addrP);
    addrP = reference(codeP, paramTy, addrP);
    code = code || codeP || instruction::PUSH(addrP);
  }

  CodeAttribs    && codAtsFunc = visit(ctx->ident());
  std::string         addrFunc = codAtsFunc.addr;
  code = code || instruction::CALL(addrFunc);

  //Pop each parameter
  for (int i = 0; i < (int)ctx->expr().size(); ++i) {
    code = code || instruction::POP();
  }

  //Pop return (if non-void function)
  std::string temp = "%"+codeCounters.newTEMP();
  if (nonVoid) code = code || instruction::POP(temp);

  CodeAttribs codAts = CodeAttribs(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitDeclarations(AslParser::DeclarationsContext *ctx) {
  DEBUG_ENTER();
  std::vector<var> lvars;
  for (auto & varDeclCtx : ctx->variable_decl()) {
    std::vector<var> varList = visit(varDeclCtx);
    for (auto & oneVar : varList) {
      lvars.push_back(oneVar);
    }
  }
  DEBUG_EXIT();
  return lvars;
}

antlrcpp::Any CodeGenVisitor::visitVariable_decl(AslParser::Variable_declContext *ctx) {
  DEBUG_ENTER();
  TypesMgr::TypeId   t1 = getTypeDecor(ctx->type());
  std::size_t      size = Types.getSizeOfType(t1);
  std::vector<var> varList;
  for (auto idCtx : ctx->ID()) {
    varList.push_back(var{idCtx->getText(), Types.to_string_basic(t1), size});
  }
  DEBUG_EXIT();
  return varList;
}

antlrcpp::Any CodeGenVisitor::visitStatements(AslParser::StatementsContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  for (auto stCtx : ctx->statement()) {
    instructionList && codeS = visit(stCtx);
    code = code || codeS;
  }
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitAssignStmt(AslParser::AssignStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAtsE1 = visit(ctx->left_expr());
  std::string           addr1 = codAtsE1.addr;
  std::string           offs1 = codAtsE1.offs;
  instructionList &     code1 = codAtsE1.code;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());

  CodeAttribs     && codAtsE2 = visit(ctx->expr());
  std::string           addr2 = codAtsE2.addr;
  std::string           offs2 = codAtsE2.offs;
  instructionList &     code2 = codAtsE2.code;
  TypesMgr::TypeId tid2 = getTypeDecor(ctx->expr());

  instructionList &&     code = code1 || code2;

  //Assignment between arrays (copy all the values from right to left array)
  if (Types.isArrayTy(tid1) and Types.isArrayTy(tid2)) {
    //Get array size and array element size
    int arrSizeVal  = (int)Types.getArraySize(tid1);
    TypesMgr::TypeId elemType = Types.getArrayElemType(tid1);
    int elemSizeVal = (int)Types.getSizeOfType(elemType);

    //Temporal registers for loop
    std::string iterator      = "%"+codeCounters.newTEMP();
    std::string elemSize      = "%"+codeCounters.newTEMP();
    std::string arrSize       = "%"+codeCounters.newTEMP();
    std::string condAddr      = "%"+codeCounters.newTEMP();
    std::string temp          = "%"+codeCounters.newTEMP();

    //instruction lists for conditional expression and body statements
    instructionList condCode  = instruction::LT(condAddr, iterator, arrSize);
    instructionList bodyStmts = instruction::LOADX(temp, addr2, iterator)
                             || instruction::XLOAD(addr1, iterator, temp)
                             || instruction::ADD(iterator, iterator, elemSize);

    //Assemble the entire loop code
    code = code || instruction::ILOAD(iterator, "0")
                || instruction::ILOAD(elemSize, std::to_string(elemSizeVal))
                || instruction::ILOAD(arrSize, std::to_string(arrSizeVal))
                || instruction_LOOP(condCode, condAddr, bodyStmts);
  }
  //Otherwise it's a single value
  else {
    std::string srcAddr;  //Address that will contain the value to assign to the left expression

    //Coerce the type of the right expression to that of the left expression
    if (codAtsE2.offs != "") {
      //The right expression has an offset
      std::string temp = "%"+codeCounters.newTEMP();
      code = code || instruction::LOADX(temp, addr2, offs2);
      srcAddr = coerceType(code, tid1, tid2, temp);
    }
    else srcAddr = coerceType(code, tid1, tid2, addr2);

    //Store the value in the left expression
    if (codAtsE1.offs != "") code = code || instruction::XLOAD(addr1, offs1, srcAddr);
    else                     code = code || instruction::LOAD(addr1, srcAddr);
  }
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitIfStmt(AslParser::IfStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  CodeAttribs     && codAtsExpr = visit(ctx->expr());
  std::string          addrExpr = codAtsExpr.addr;
  instructionList &    codeExpr = codAtsExpr.code;

  //If statements
  instructionList &&   codeStmtsIf = visit(ctx->statements(0));
  std::string label = codeCounters.newLabelIF();
  std::string labelEndIf = "endif"+label;
  
  if (ctx->statements(1)) {
    //There's an "else" too
    instructionList &&   codeStmtsElse = visit(ctx->statements(1));
    std::string labelElse = "else"+label;

    code = codeExpr    || instruction::FJUMP(addrExpr, labelElse) 
        || codeStmtsIf || instruction::UJUMP(labelEndIf) 
        || instruction::LABEL(labelElse) || codeStmtsElse
        || instruction::LABEL(labelEndIf);
  } 
  else {
    //There's just an "if"
    code = codeExpr    || instruction::FJUMP(addrExpr, labelEndIf) ||
           codeStmtsIf || instruction::LABEL(labelEndIf);
  }
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWhileStmt(AslParser::WhileStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     &&  codAtE = visit(ctx->expr());
  std::string          addrE = codAtE.addr;
  instructionList &    codeE = codAtE.code;
  instructionList &&   codeS = visit(ctx->statements());

  instructionList code = instruction_LOOP(codeE, addrE, codeS);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitReadStmt(AslParser::ReadStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAtsE = visit(ctx->left_expr());
  std::string          addrE = codAtsE.addr;
  std::string          offsE = codAtsE.offs;
  instructionList &    codeE = codAtsE.code;
  instructionList &     code = codeE;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());

  //Read input value and save it in temp
  std::string temp = "%"+codeCounters.newTEMP();
  if      (Types.isFloatTy(tid1))     code = codeE || instruction::READF(temp);
  else if (Types.isCharacterTy(tid1)) code = codeE || instruction::READC(temp);
  else                                code = codeE || instruction::READI(temp);

  //Store temp value in variable
  if (codAtsE.offs != "") code = code || instruction::XLOAD(addrE, offsE, temp);
  else                    code = code || instruction::LOAD(addrE, temp);

  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteExpr(AslParser::WriteExprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr());
  std::string         addr1 = codAt1.addr;
  // std::string         offs1 = codAt1.offs;
  instructionList &   code1 = codAt1.code;
  instructionList &    code = code1;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->expr());

  if      (Types.isFloatTy(tid1))     code = code || instruction::WRITEF(addr1);
  else if (Types.isCharacterTy(tid1)) code = code || instruction::WRITEC(addr1);
  else                                code = code || instruction::WRITEI(addr1);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteString(AslParser::WriteStringContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  std::string s = ctx->STRING()->getText();
  code = code || instruction::WRITES(s);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitArithmetic(AslParser::ArithmeticContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;

  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  // TypesMgr::TypeId  t = getTypeDecor(ctx);

  std::string temp = "%"+codeCounters.newTEMP();
  if (ctx->MOD())
    code = code || instruction_MOD(temp, addr1, addr2);
  else if (Types.isFloatTy(t1) or Types.isFloatTy(t2)) {
    //Must coerce to float type
    std::string param1 = coerceType(code, t2, t1, addr1);
    std::string param2 = coerceType(code, t1, t2, addr2); 
    if      (ctx->MUL())   code = code || instruction::FMUL(temp, param1, param2);
    else if (ctx->DIV())   code = code || instruction::FDIV(temp, param1, param2);
    else if (ctx->MINUS()) code = code || instruction::FSUB(temp, param1, param2);
    else                   code = code || instruction::FADD(temp, param1, param2);
  }
  else { //Both t1 and t2 must be integer
    if      (ctx->MUL())   code = code || instruction::MUL(temp, addr1, addr2);
    else if (ctx->DIV())   code = code || instruction::DIV(temp, addr1, addr2);
    else if (ctx->MINUS()) code = code || instruction::SUB(temp, addr1, addr2);
    else                   code = code || instruction::ADD(temp, addr1, addr2);  //(ctx->PLUS())
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitLogical(AslParser::LogicalContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  
  std::string temp = "%"+codeCounters.newTEMP();
  if (ctx->AND()) code = code || instruction::AND(temp, addr1, addr2);
  else            code = code || instruction::OR (temp, addr1, addr2);
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitUnary(AslParser::UnaryContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs      && codAt = visit(ctx->expr());
  std::string          addr = codAt.addr;
  instructionList &    code = codAt.code;
  TypesMgr::TypeId t = getTypeDecor(ctx->expr());
  
  std::string temp = "%"+codeCounters.newTEMP();
  if      (Types.isBooleanTy(t)) code = code || instruction::NOT(temp, addr);
  else if (Types.isFloatTy(t)) {
    if (ctx->MINUS()) code = code || instruction::FNEG(temp, addr);
    else              code = code || instruction::FLOAD(temp, addr);
  }
  else { //Otherwise it's an Integer
    if (ctx->MINUS()) code = code || instruction::NEG(temp, addr);
    else              code = code || instruction::ILOAD(temp, addr);
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitRelational(AslParser::RelationalContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;

  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  // TypesMgr::TypeId  t = getTypeDecor(ctx);

  std::string temp = "%"+codeCounters.newTEMP();
  if (Types.isFloatTy(t1) or Types.isFloatTy(t2)) {
    //Must coerce to float type
    std::string param1 = coerceType(code, t2, t1, addr1);
    std::string param2 = coerceType(code, t1, t2, addr2);
    if      (ctx->LT())  code = code || instruction::FLT(temp, param1, param2);
    else if (ctx->LE())  code = code || instruction::FLE(temp, param1, param2);
    else if (ctx->GT())  code = code || instruction::FLT(temp, param2, param1);
    else if (ctx->GE())  code = code || instruction::FLE(temp, param2, param1);
    else if (ctx->NEQ()) code = code || instruction_FNE(temp, param1, param2);
    else                 code = code || instruction::FEQ(temp, param1, param2);
  }
  else { //Both t1 and t2 must be integer
    if      (ctx->LT())  code = code || instruction::LT(temp, addr1, addr2);
    else if (ctx->LE())  code = code || instruction::LE(temp, addr1, addr2);
    else if (ctx->GT())  code = code || instruction::LT(temp, addr2, addr1);
    else if (ctx->GE())  code = code || instruction::LE(temp, addr2, addr1);
    else if (ctx->NEQ()) code = code || instruction_NE(temp, addr1, addr2);
    else                 code = code || instruction::EQ(temp, addr1, addr2);
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitValue(AslParser::ValueContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  const std::string &value = ctx->getText();
  std::string temp = "%"+codeCounters.newTEMP();
  if      (ctx->BOOLVAL())  code = instruction::ILOAD(temp, value == "true" ? "1" : "0");
  else if (ctx->FLOATVAL()) code = instruction::FLOAD(temp, value);
  else if (ctx->CHARVAL())  code = instruction::CHLOAD(temp, value.substr(1,value.length()-2));
  else                      code = instruction::ILOAD(temp, value);
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitLeftExpr(AslParser::LeftExprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAtsLE = visit(ctx->left_expr());
  std::string         addr = codAtsLE.addr;
  std::string         offs = codAtsLE.offs;
  instructionList & codeLE = codAtsLE.code;
  instructionList code = codeLE;

  std::string temp = addr;
  if (offs != "") {
		temp = "%" + codeCounters.newTEMP();
		code = code || instruction::LOADX(temp, addr, offs);
	}

  CodeAttribs codAts = CodeAttribs(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitExprIdent(AslParser::ExprIdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->ident());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitArrLeftExpr(AslParser::ArrLeftExprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAtsId = visit(ctx->ident());
  std::string         addrId = codAtsId.addr;
  instructionList &   codeId = codAtsId.code;

  CodeAttribs && codAtsEx = visit(ctx->expr());
  std::string         addrEx = codAtsEx.addr;
  instructionList &   codeEx = codAtsEx.code;
  
  std::string temp1 = "%"+codeCounters.newTEMP();
  std::string temp2 = "%"+codeCounters.newTEMP();

  //Get element size to calculate real offset
  TypesMgr::TypeId  elemType = getTypeDecor(ctx);
  std::size_t       elemSize = Types.getSizeOfType(elemType);

  instructionList code = codeId || codeEx
                    || instruction::ILOAD(temp1, std::to_string(elemSize))
                    || instruction::MUL(temp2, addrEx, temp1);
  CodeAttribs codAts = CodeAttribs(addrId, temp2, code);

  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitIdent(AslParser::IdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs codAtsId(ctx->ID()->getText(), "", instructionList());
  instructionList code = codAtsId.code;
  std::string addr = dereference(code, codAtsId.addr);
  CodeAttribs codAts(addr, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitParenthesis(AslParser::ParenthesisContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->expr());
  DEBUG_EXIT();
  return codAts;
}


// Getters for the necessary tree node atributes:
//   Scope and Type
SymTable::ScopeId CodeGenVisitor::getScopeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getScope(ctx);
}
TypesMgr::TypeId CodeGenVisitor::getTypeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getType(ctx);
}


//DEFINITION OF CUSTOM MACROS
std::string CodeGenVisitor::coerceType(instructionList &code, 
                                       TypesMgr::TypeId destType, 
                                       TypesMgr::TypeId srcType,
                                       const std::string &srcAddr) {
  if (Types.isIntegerTy(srcType) and Types.isFloatTy(destType)) {
    std::string destAddr = "%" + codeCounters.newTEMP();
    code = code || instruction::FLOAT(destAddr, srcAddr);
    return destAddr;
  }
  return srcAddr;
}

std::string CodeGenVisitor::reference(instructionList &code,
                                      TypesMgr::TypeId srcType,
                                      const std::string &srcAddr) {
  if (Symbols.isLocalVarClass(srcAddr) and Types.isArrayTy(srcType)) {
    std::string destAddr = "%"+codeCounters.newTEMP();
    code = code || instruction::ALOAD(destAddr, srcAddr);
    return destAddr;
  }
  return srcAddr;
}

std::string CodeGenVisitor::dereference(instructionList &code,
                                        std::string &srcAddr) {
  TypesMgr::TypeId srcType = Symbols.getType(srcAddr);
  if (Symbols.isParameterClass(srcAddr) and Types.isArrayTy(srcType)) {
    std::string destAddr = "%"+codeCounters.newTEMP();
    code = code || instruction::LOAD(destAddr, srcAddr);
    return destAddr;
  }
  return srcAddr;
}

//Modulo
instructionList CodeGenVisitor::instruction_MOD(const std::string &dest, 
                                                const std::string &param1, 
                                                const std::string &param2) {
  std::string temp1 = "%" + codeCounters.newTEMP();
  std::string temp2 = "%" + codeCounters.newTEMP();
  return instruction::DIV(temp1, param1, param2)
      || instruction::MUL(temp2, param2, temp1)
      || instruction::SUB(dest,  param1, temp2);
}

//Not Equal
instructionList CodeGenVisitor::instruction_NE(const std::string &dest,
                                               const std::string &addr1,
                                               const std::string &addr2) {
  std::string temp = "%"+codeCounters.newTEMP();
  return instruction::EQ(temp, addr1, addr2)
      || instruction::NOT(dest, temp);
}

//Float Not Equal
instructionList CodeGenVisitor::instruction_FNE(const std::string &dest,
                                                const std::string &addr1,
                                                const std::string &addr2) {
  std::string temp = "%"+codeCounters.newTEMP();
  return instruction::FEQ(temp, addr1, addr2)
      || instruction::NOT(dest, temp);
}

//General loops
instructionList CodeGenVisitor::instruction_LOOP(instructionList &condCode,
                                                 const std::string &condAddr,
                                                 instructionList &bodyStmts) {
  std::string label = codeCounters.newLabelWHILE();
  std::string labelWhile    = "while" + label;
  std::string labelEndWhile = "endwhile" + label;
  return instruction::LABEL(labelWhile) 
      || condCode 
      || instruction::FJUMP(condAddr, labelEndWhile) 
      || bodyStmts 
      || instruction::UJUMP(labelWhile) 
      || instruction::LABEL(labelEndWhile);
}

// Constructors of the class CodeAttribs:
//
CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList & code) :
  addr{addr}, offs{offs}, code{code} {
}

CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList && code) :
  addr{addr}, offs{offs}, code{code} {
}
