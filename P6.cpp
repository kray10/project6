#include <iostream>
#include <cstdlib>
#include <cstring>

#include "err.hpp"
#include "lilc_compiler.hpp"
#include "ast.hpp"

using namespace LILC;

int 
main( const int argc, const char **argv )
{
	if (argc != 3){
		std::cout << "Usage: lilcc <infile> <outfile>" 
			<< std::endl;
		return 1;
	}

	LILC::LilC_Compiler compiler;
	try {
		if (compiler.codeGen(argv[1], argv[2])){
			return 0;
		}
	} catch (LILC::ToDoError& err){
		std::cerr << err.what() << std::endl;
	} catch (LILC::InternalError& err){
		std::cerr << err.what() << std::endl;
	} catch (std::runtime_error err){
		std::cerr << "runtime error" << std::endl;
		std::cerr << err.what() << std::endl;
	}
	return 1;
}
