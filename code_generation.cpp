#include "err.hpp"
#include "ast.hpp"
#include "symbol_table.hpp"
#include "lilc_compiler.hpp"
#include "lilc_mips.hpp"

namespace LILC{

bool LilC_Compiler::codeGen(
	const char * const inFile,
	const char * const outFile
){
	if (!this->typeAnalysis(inFile)){ return false; }
	return this->astRoot->codeGen();
}

bool ASTNode::codeGen(){
	throw LILC::InternalError(
		__FILE__ ": "
		"We should never see this, as it\n"
		"is supposed to be overridden in any\n"
		"subclass at which it is encountered");
	return false;
}

bool ProgramNode::codeGen(){
	throw LILC::ToDoError();
	return false;
}

} // End namespace LILC 

