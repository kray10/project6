#include <string>
#include "lilc_mips.hpp"

namespace LILC{

const std::string LilC_Backend::TRUE = "1";
const std::string LilC_Backend::FALSE = "0";

const std::string LilC_Backend::FP = "$fp";
const std::string LilC_Backend::SP = "$sp";
const std::string LilC_Backend::RA = "$ra";
const std::string LilC_Backend::V0 = "$v0";
const std::string LilC_Backend::V1 = "$v1";
const std::string LilC_Backend::A0 = "$a0";
const std::string LilC_Backend::T0 = "$t0";
const std::string LilC_Backend::T1 = "$t1";

void LilC_Backend::generateWithComment(
	std::string opcode,
	std::string comment,
	std::string arg1,
	std::string arg2,
	std::string arg3
) {
        int space = MAXLEN - opcode.length() + 2;

        out << "\t" + opcode;
        if (arg1 != "") {
            for (int k = 1; k <= space; k++)
                out << " ";
            out << arg1;
            if (arg2 != "") {
                out << ", " + arg2;
                if (arg3 != "")
                    out << ", " + arg3;
            }
        }
        if (comment != "")
            out << "\t\t#" + comment;
	out << std::endl;
}

void LilC_Backend::generate(
	const std::string opcode,
	const std::string arg1,
	const std::string arg2,
	const std::string arg3) {
        int space = MAXLEN - opcode.length() + 2;

        out << "\t" + opcode;
        if (arg1 != "") {
            for (int k = 1; k <= space; k++)
                out << " ";
            out << arg1;
            if (arg2 != "") {
                out << ", " + arg2;
                if (arg3 != "")
                    out << ", " + arg3;
            }
        }
	out << std::endl;
}

void LilC_Backend::generateIndexed(
	std::string opcode,
	std::string arg1,
	std::string arg2,
	int arg3,
	std::string comment=""
) {
        int space = MAXLEN - opcode.length() + 2;

	out << "\t" << opcode;
	for (int k = 1; k <= space; k++) {
		out << " ";
	}
	out << arg1 << ", " << arg3 << "(" << arg2 << ")";
	if (comment != "")
		out << "\t#" + comment;
	out << std::endl;
}

void LilC_Backend::generateLabeled(
	std::string label,
	std::string opcode,
        std::string comment,
	std::string arg1
) {
	int space = MAXLEN - opcode.length() + 2;

	out << label << ":";
	out << "\t" << opcode;
	if (arg1 != "") {
		for (int k = 1; k <= space; k++) {
			out << " ";
		}
		out << arg1;
        }
        if (comment != "") { out << "\t# " << comment; }
	out << std::endl;
}

void LilC_Backend::genPush(std::string s) {
	generateIndexed("sw", s, SP, 0, "PUSH");
	generate("subu", SP, SP, "4");
}

void LilC_Backend::genPop(std::string s) {
	generateIndexed("lw", s, SP, 4, "POP");
	generate("addu", SP, SP, "4");
}

void LilC_Backend::genLabel(std::string label, std::string comment) {
	out << label << ":";
	if (comment != "")
		out << "\t\t" << "# " << comment;
	out << std::endl;
}

std::string LilC_Backend::nextLabel() {
	std::string tmp = ".L" + std::to_string(currLabel);
	currLabel++;
	return(tmp);
}

void LilC_Backend::genGlobalVar(std::string name, int size) {
	out << "\t.data\n\t.align 2\n_" << name
	    << ": .space " <<size << std::endl;
}

void LilC_Backend::genWrite(std::string type) {
	genPop(A0);
	if (type == "int" || type == "bool") {
		generate("li", V0, "1");
	} else if (type == "string") {
		generate("li", V0, "4");
	}
	generate("syscall");
}

void LilC_Backend::genStringLit(std::string value) {
	out << "\t.data" << std::endl;
	std::string label = nextLabel();
	generateLabeled(label, ".asciiz " + value, "");
	out << "\t.text" << std::endl;
	generate("la", T0, label);
	genPush(T0);
}

void LilC_Backend::genIntLit(int value) {
	generate("li", T0, std::to_string(value));
	genPush(T0);
}

void LilC_Backend::genBoolLit(bool value) {
	std::string boolVal = value ? TRUE : FALSE;
	generate("li", T0, boolVal);
	genPush(T0);
}

void LilC_Backend::genAddr(std::string id, bool isGlobal, int offset) {
	if (isGlobal) {
		generate("la", T0, "_" + id);
	} else {
		generateIndexed("la", T0, FP, offset);
	}
	genPush(T0);
}

void LilC_Backend::genAssign() {
	genPop(T1);
	genPop(T0);
	generateIndexed("sw", T0, T1, 0);
	genPush(T0);
}

void LilC_Backend::genLoadId(std::string id, bool isGlobal, int offset) {
	if (isGlobal) {
		generate("lw", T0, "_" + id);
	} else {
		generateIndexed("lw", T0, FP, offset);
	}
	genPush(T0);
}

void LilC_Backend::genNegativeNum() {
	generateWithComment("li", "UnaryMinusNode", T0, "-1");
	genPop(T1);
	genMult(T0, T1, T0);
	genPush(T0);
}

void LilC_Backend::genMult(std::string arg1, std::string arg2, std::string result) {
	generate("mult", arg1, arg2);
	generate("mflo", result, "");
}

void LilC_Backend::genNot() {
	genPop(T0);
	generateWithComment("xori", "Not Operation", T0, T0, "1");
	genPush(T0);
}

void LilC_Backend::genDiv(std::string arg1, std::string arg2, std::string result) {
	generate("div", arg1, arg2);
	generate("mflo", result, "");
}

} // End namespace LILC
