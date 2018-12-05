#include "err.hpp"
#include "ast.hpp"
#include "symbol_table.hpp"
#include "lilc_compiler.hpp"
#include <fstream>

namespace LILC{

bool LilC_Compiler::codeGen(
	const char * const inFile,
	const char * const outFile
){
	if (!this->typeAnalysis(inFile)){ return false; }
	std::ofstream out(outFile);
	LilC_Backend* backend = new LilC_Backend(out);
	bool valid = this->astRoot->codeGen(backend);
	out.close();
	return valid;
}

bool ASTNode::codeGen(LilC_Backend* backend){
	throw LILC::InternalError(
		__FILE__ ": "
		"We should never see this, as it\n"
		"is supposed to be overridden in any\n"
		"subclass at which it is encountered");
	return false;
}

bool ProgramNode::codeGen(LilC_Backend* backend){
	return myDeclList->codeGen(backend);
}

bool DeclListNode::codeGen(LilC_Backend* backend){
	bool valid = true;
	for (DeclNode * decl : *myDecls) {
			valid = decl->globalCodeGen(backend) && valid;
	}
	return valid;
}

bool VarDeclNode::globalCodeGen(LilC_Backend* backend){
	if (mySize == -1) {
		backend->genGlobalVar(getName(), 4);
	}
	return true;
}

bool FnDeclNode::globalCodeGen(LilC_Backend* backend){
	std::string entrance = "_" + getName();
	std::string exit = "_" + getName() + "_Exit";

	if (getName() == "main") {
		backend->generate(".text");
		backend->generate(".globl main");
		backend->genLabel(getName(), "Method entry");
		backend->genLabel("_start", "add __start for main only");
	} else {
		backend->generate(".text");
		backend->genLabel(entrance, getName() + " function entry");
	}

	backend->genPush(LilC_Backend::RA);
	backend->genPush(LilC_Backend::FP);
	backend->generate("addu", LilC_Backend::FP, LilC_Backend::SP, std::to_string(myFormals->offsetSize() + 8));
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myBody->getLocalsSize()));

	myBody->codeGenWithExit(backend, exit);

	backend->generateWithComment("","#FUNCTION EXIT");
	backend->genLabel(exit);
	backend->generateIndexed("lw", LilC_Backend::RA, LilC_Backend::FP, myFormals->offsetSize() * -1, "load return address");
	backend->generateWithComment("move", "save control link", LilC_Backend::T0, LilC_Backend::FP);
	backend->generateIndexed("lw", LilC_Backend::FP, LilC_Backend::FP, (myFormals->offsetSize() + 4) * -1, "restore FP");
	backend->generateWithComment("move", "restore SP", LilC_Backend::SP, LilC_Backend::T0);

	if (getName() == "main") {
		backend->generateWithComment("li", "load exit code for syscall", LilC_Backend::V0, "10");
		backend->generateWithComment("syscall", "only do this for main", "", "");
	} else {
		backend->generateWithComment("jr", "return", LilC_Backend::RA, "");
	}
	return true;
}

bool FormalDeclNode::globalCodeGen(LilC_Backend* backend){
	throw runtime_error("Not implemented: FormalDeclNode");
}

bool StructDeclNode::globalCodeGen(LilC_Backend* backend){
	throw runtime_error("Not implemented: StructDeclNode");
}

bool FnBodyNode::codeGen(LilC_Backend* backend) {
	throw runtime_error("Not implement: FnBodyNode");
}

bool FnBodyNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	return myStmtList->codeGenWithExit(backend, exitLabel);
}

bool StmtListNode::codeGen(LilC_Backend* backend) {
	bool valid = true;
	for (StmtNode* stmt : *myStmts) {
		valid = stmt->codeGen(backend) && valid;
	}
	return valid;
}

bool StmtListNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	bool valid = true;
	for (StmtNode* stmt : *myStmts) {
		valid = stmt->codeGenWithExit(backend, exitLabel) && valid;
	}
	return valid;
}

bool StrLitNode::codeGen(LilC_Backend* backend) {
	backend->genStringLit(myString);
	return true;
}

bool WriteStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " WRITE");
	myExp->codeGen(backend);
	backend->genWrite(typeToWrite);
	return true;
}

bool IntLitNode::codeGen(LilC_Backend* backend) {
	backend->genIntLit(myInt);
	return true;
}

bool TrueNode::codeGen(LilC_Backend* backend) {
	backend->genBoolLit(true);
	return true;
}

bool FalseNode::codeGen(LilC_Backend* backend) {
	backend->genBoolLit(false);
	return true;
}

bool IdNode::genAddr(LilC_Backend* backend) {
	backend->genAddr(myStrVal, mySymbol->isGlobal(), mySymbol->getOffset());
	return true;
}

bool IdNode::codeGen(LilC_Backend* backend) {
	backend->genLoadId(myStrVal, mySymbol->isGlobal(), mySymbol->getOffset());
	return true;
}

bool IdNode::genJumpAndLink(LilC_Backend* backend) {
	std::string label = myStrVal == "main" ? myStrVal : "_" + myStrVal;
	backend->generate("jal", label);
	return true;
}

bool AssignStmtNode::codeGen(LilC_Backend* backend) {
	myAssign->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	return true;
}

bool AssignNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " Assign");
	myExpRHS->codeGen(backend);
	myExpLHS->genAddr(backend);
	backend->genAssign();
	return true;
}

bool UnaryMinusNode::codeGen(LilC_Backend* backend) {
	myExp->codeGen(backend);
	backend->genNegativeNum();
	return true;
}

bool NotNode::codeGen(LilC_Backend* backend) {
	myExp->codeGen(backend);
	backend->genNot();
	return true;
}

bool PlusNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " PLUS");
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("add", LilC_Backend::T0, LilC_Backend::T0, LilC_Backend::T1);
	backend->genPush(LilC_Backend::T0);
	return true;
}

bool MinusNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " MINUS");
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("sub", LilC_Backend::T0, LilC_Backend::T0, LilC_Backend::T1);
	backend->genPush(LilC_Backend::T0);
	return true;
}

bool TimesNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " TIMES");
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->genMult(LilC_Backend::T0, LilC_Backend::T1, LilC_Backend::T0);
	backend->genPush(LilC_Backend::T0);
	return true;
}

bool DivideNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " DIVIDE");
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->genDiv(LilC_Backend::T0, LilC_Backend::T1, LilC_Backend::T0);
	backend->genPush(LilC_Backend::T0);
	return true;
}

bool AndNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " AND");
	std::string pushFalse = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::TRUE, pushFalse);
	myExp2->codeGen(backend);
	backend->generateWithComment("b", "Exit and exp", exit, "");
	backend->genLabel(pushFalse, "return false");
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, "Exit And expression");
	return true;
}

bool OrNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " OR");
	std::string pushTrue= backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::FALSE, pushTrue);
	myExp2->codeGen(backend);
	backend->generateWithComment("b", "Exit or exp", exit, "");
	backend->genLabel(pushTrue, "return true");
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, "Exit or expression");
	return true;
}

bool EqualsNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " EQUALS");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("beq", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit equals exp");
	return true;
}

bool NotEqualsNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " NOT EQUALS");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit not equals exp");
	return true;
}

bool LessNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " LESS THAN");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("blt", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit less than exp");
	return true;
}

bool GreaterNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " GREATER THAN");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("bgt", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit greater than exp");
	return true;
}

bool LessEqNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " LESS THAN OR EQUAL");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("ble", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit greater than exp");
	return true;
}

bool GreaterEqNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " GREATER THAN OR EQUAL");
	std::string pushTrue = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp1->codeGen(backend);
	myExp2->codeGen(backend);
	backend->genPop(LilC_Backend::T1);
	backend->genPop(LilC_Backend::T0);
	backend->generate("bge", LilC_Backend::T0, LilC_Backend::T1, pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::FALSE);
	backend->genPush(LilC_Backend::T0);
	backend->generate("j", exit);
	backend->genLabel(pushTrue);
	backend->generate("li", LilC_Backend::T0, LilC_Backend::TRUE);
	backend->genPush(LilC_Backend::T0);
	backend->genLabel(exit, " exit greater than exp");
	return true;
}

bool PostIncStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " POSTINC");
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("addi", LilC_Backend::T0, "1");
	backend->genPush(LilC_Backend::T0);
	myExp->genAddr(backend);
	backend->genAssign();
	return true;
}

bool PostDecStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " POSTDEC");
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("addi", LilC_Backend::T0, "-1");
	backend->genPush(LilC_Backend::T0);
	myExp->genAddr(backend);
	backend->genAssign();
	return true;
}

bool IfStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " If statement");
	std::string exit = backend->nextLabel();
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, exit);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	myStmts->codeGen(backend);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	backend->genLabel(exit, " Skip if statment");
	return true;
}

bool IfStmtNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	backend->generateWithComment("", " If statement");
	std::string exit = backend->nextLabel();
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, exit);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	myStmts->codeGenWithExit(backend, exitLabel);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	backend->genLabel(exit, " Skip if statment");
	return true;
}

bool IfElseStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " If else statement");
	std::string elseB = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, elseB);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	myStmtsT->codeGen(backend);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	backend->generate("j", exit);
	backend->genLabel(elseB, " else portion statment");
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsF->sizeOfDecls()));
	myStmtsF->codeGen(backend);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	backend->genLabel(exit);
	return true;
}

bool IfElseStmtNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	backend->generateWithComment("", " If else statement");
	std::string elseB = backend->nextLabel();
	std::string exit = backend->nextLabel();
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, elseB);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	myStmtsT->codeGenWithExit(backend, exitLabel);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	backend->generate("j", exit);
	backend->genLabel(elseB, " else portion statment");
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsF->sizeOfDecls()));
	myStmtsF->codeGenWithExit(backend, exitLabel);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDeclsT->sizeOfDecls()));
	backend->genLabel(exit);
	return true;
}

bool WhileStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " while statement");
	std::string start = backend->nextLabel();
	std::string exit = backend->nextLabel();
	backend->genLabel(start, " Beginning of while loop");
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, exit);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	myStmts->codeGen(backend);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	backend->generate("j", start);
	backend->genLabel(exit, " exit for while loop");
	return true;
}

bool WhileStmtNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	backend->generateWithComment("", " while statement");
	std::string start = backend->nextLabel();
	std::string exit = backend->nextLabel();
	backend->genLabel(start, " Beginning of while loop");
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::T1, LilC_Backend::TRUE);
	backend->generate("bne", LilC_Backend::T0, LilC_Backend::T1, exit);
	backend->generate("subu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	myStmts->codeGenWithExit(backend, exitLabel);
	backend->generate("addu", LilC_Backend::SP, LilC_Backend::SP, std::to_string(myDecls->sizeOfDecls()));
	backend->generate("j", start);
	backend->genLabel(exit, " exit for while loop");
	return true;
}

bool ReadStmtNode::codeGen(LilC_Backend* backend) {
	backend->generateWithComment("", " READ");
	myExp->genAddr(backend);
	backend->genPop(LilC_Backend::T0);
	backend->generate("li", LilC_Backend::V0, "5");
	backend->generate("syscall");
	backend->generateIndexed("sw", LilC_Backend::V0, LilC_Backend::T0, 0, "Store value read");
	return true;
}

bool CallStmtNode::codeGen(LilC_Backend* backend) {
	myCallExp->codeGen(backend);
	backend->genPop(LilC_Backend::T0);
	return true;
}

bool CallExpNode::codeGen(LilC_Backend* backend) {
	myExpList->codeGen(backend);
	myId->genJumpAndLink(backend);
	backend->genPush(LilC_Backend::V0);
	return true;
}

bool ExpListNode::codeGen(LilC_Backend* backend) {
	for (ExpNode * exp : myExps) {
		exp->codeGen(backend);
	}
	return true;
}

bool ReturnStmtNode::codeGenWithExit(LilC_Backend* backend, std::string exitLabel) {
	myExp->codeGen(backend);
	backend->genPop(LilC_Backend::V0);
	backend->generate("j", exitLabel);
	return true;
}

} // End namespace LILC
