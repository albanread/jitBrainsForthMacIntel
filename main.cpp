#include "JitContext.h"
#include "ForthDictionary.h"
#include "JitGenerator.h"
#include "JitGenerator.h"
#include "quit.h"

JitGenerator& gen = JitGenerator::getInstance();


// start to test some code generation
void add_words()
{
    d.addConstant("1", JitGenerator::push1, JitGenerator::build_forth(JitGenerator::push1), nullptr, nullptr);
    d.addConstant("2", JitGenerator::push2, JitGenerator::build_forth(JitGenerator::push2), nullptr, nullptr);
    d.addConstant("3", JitGenerator::push3, JitGenerator::build_forth(JitGenerator::push3), nullptr, nullptr);
    d.addConstant("4", JitGenerator::push4, JitGenerator::build_forth(JitGenerator::push4), nullptr, nullptr);
    d.addConstant("8", JitGenerator::push8, JitGenerator::build_forth(JitGenerator::push8), nullptr, nullptr);

    // d.addWord("16", JitGenerator::push16, JitGenerator::build_forth(JitGenerator::push16), nullptr, nullptr);

    d.addConstant("32", JitGenerator::push32, JitGenerator::build_forth(JitGenerator::push32), nullptr, nullptr);
    d.addConstant("64", JitGenerator::push64, JitGenerator::build_forth(JitGenerator::push64), nullptr, nullptr);
    d.addConstant("-1", JitGenerator::pushNeg1, JitGenerator::build_forth(JitGenerator::pushNeg1), nullptr, nullptr);

    d.addWord("2*", JitGenerator::gen2mul, JitGenerator::build_forth(JitGenerator::gen2mul), nullptr, nullptr);
    d.addWord("4*", JitGenerator::gen4mul, JitGenerator::build_forth(JitGenerator::gen4mul), nullptr, nullptr);
    d.addWord("8*", JitGenerator::gen8mul, JitGenerator::build_forth(JitGenerator::gen8mul), nullptr, nullptr);
    d.addWord("10*", JitGenerator::genMulBy10, JitGenerator::build_forth(JitGenerator::genMulBy10), nullptr, nullptr);

    d.addWord("16*", JitGenerator::gen16mul, JitGenerator::build_forth(JitGenerator::gen16mul), nullptr, nullptr);

    d.addWord("2/", JitGenerator::gen2Div, JitGenerator::build_forth(JitGenerator::gen2Div), nullptr, nullptr);
    d.addWord("4/", JitGenerator::gen4Div, JitGenerator::build_forth(JitGenerator::gen4Div), nullptr, nullptr);
    d.addWord("8/", JitGenerator::gen8Div, JitGenerator::build_forth(JitGenerator::gen8Div), nullptr, nullptr);

    d.addWord("1+", JitGenerator::gen1Inc, JitGenerator::build_forth(JitGenerator::gen1Inc), nullptr, nullptr);
    d.addWord("2+", JitGenerator::gen2Inc, JitGenerator::build_forth(JitGenerator::gen2Inc), nullptr, nullptr);
    d.addWord("16+", JitGenerator::gen16Inc, JitGenerator::build_forth(JitGenerator::gen16Inc), nullptr, nullptr);

    d.addWord("1-", JitGenerator::gen1Dec, JitGenerator::build_forth(JitGenerator::gen1Dec), nullptr, nullptr);
    d.addWord("2-", JitGenerator::gen2Dec, JitGenerator::build_forth(JitGenerator::gen2Dec), nullptr, nullptr);
    d.addWord("16-", JitGenerator::gen16Dec, JitGenerator::build_forth(JitGenerator::gen16Dec), nullptr, nullptr);

    d.addWord("CHAR", nullptr, nullptr, JitGenerator::genImmediateChar, JitGenerator::genTerpImmediateChar);


    d.addWord("0=", JitGenerator::genZeroEquals, JitGenerator::build_forth(JitGenerator::genZeroEquals), nullptr,
              nullptr);

    d.addWord("0<", JitGenerator::genZeroLessThan, JitGenerator::build_forth(JitGenerator::genZeroLessThan), nullptr,
              nullptr);
    d.addWord("0>", JitGenerator::genZeroGreaterThan, JitGenerator::build_forth(JitGenerator::genZeroGreaterThan),
              nullptr, nullptr);

    // Add the < comparison word
    d.addWord("<", JitGenerator::genLt, JitGenerator::build_forth(JitGenerator::genLt), nullptr, nullptr);

    // Add the = comparison word
    d.addWord("=", JitGenerator::genEq, JitGenerator::build_forth(JitGenerator::genEq), nullptr, nullptr);

    // Add the > comparison word
    d.addWord(">", JitGenerator::genGt, JitGenerator::build_forth(JitGenerator::genGt), nullptr, nullptr);


    d.addWord("+", JitGenerator::genPlus, JitGenerator::build_forth(JitGenerator::genPlus), nullptr, nullptr);
    d.addWord("-", JitGenerator::genSub, JitGenerator::build_forth(JitGenerator::genSub), nullptr, nullptr);
    d.addWord("*", JitGenerator::genMul, JitGenerator::build_forth(JitGenerator::genMul), nullptr, nullptr);
    d.addWord("/", JitGenerator::genDiv, JitGenerator::build_forth(JitGenerator::genDiv), nullptr, nullptr);
    d.addWord("sqrt", JitGenerator::genIntSqrt, JitGenerator::build_forth(JitGenerator::genIntSqrt), nullptr, nullptr);
    d.addWord("gcd", JitGenerator::genGcd, JitGenerator::build_forth(JitGenerator::genGcd), nullptr, nullptr);

    d.addWord("f+", JitGenerator::genFPlus, JitGenerator::build_forth(JitGenerator::genFPlus), nullptr, nullptr);
    d.addWord("f-", JitGenerator::genFSub, JitGenerator::build_forth(JitGenerator::genFSub), nullptr, nullptr);
    d.addWord("f*", JitGenerator::genFMul, JitGenerator::build_forth(JitGenerator::genFMul), nullptr, nullptr);
    d.addWord("f/", JitGenerator::genFDiv, JitGenerator::build_forth(JitGenerator::genFDiv), nullptr, nullptr);
    d.addWord("fmod", JitGenerator::genFMod, JitGenerator::build_forth(JitGenerator::genFMod), nullptr, nullptr);    d.addWord("fsqrt", JitGenerator::genSqrt, JitGenerator::build_forth(JitGenerator::genSqrt), nullptr, nullptr);
    d.addWord("fsqrt", JitGenerator::genSqrt, JitGenerator::build_forth(JitGenerator::genSqrt), nullptr, nullptr);
    d.addWord("fabs", JitGenerator::genFAbs, JitGenerator::build_forth(JitGenerator::genFAbs), nullptr, nullptr);

    // floats conversions
    d.addWord("FLOAT", JitGenerator::genIntToFloat, JitGenerator::build_forth(JitGenerator::genIntToFloat), nullptr, nullptr);
    d.addWord("INTEGER", JitGenerator::genFloatToInt, JitGenerator::build_forth(JitGenerator::genFloatToInt), nullptr, nullptr);

    // Floating point maximum and minimum
    d.addWord("fmax", JitGenerator::genFMax, JitGenerator::build_forth(JitGenerator::genFMax), nullptr, nullptr);
    d.addWord("fmin", JitGenerator::genFMin, JitGenerator::build_forth(JitGenerator::genFMin), nullptr, nullptr);


    // floating comparisons
    d.addWord("f<", JitGenerator::genFLess, JitGenerator::build_forth(JitGenerator::genFLess), nullptr, nullptr);
    d.addWord("f>", JitGenerator::genFGreater, JitGenerator::build_forth(JitGenerator::genFGreater), nullptr, nullptr);
    d.addWord("f=", JitGenerator::genFApproxEquals, JitGenerator::build_forth(JitGenerator::genFApproxEquals), nullptr, nullptr);
    d.addWord("f<>", JitGenerator::genFApproxNotEquals, JitGenerator::build_forth(JitGenerator::genFApproxNotEquals), nullptr, nullptr);


    // print float
    d.addWord("f.", JitGenerator::genFDot, JitGenerator::build_forth(JitGenerator::genFDot), nullptr, nullptr);


    d.addWord("MOD", JitGenerator::genMod, JitGenerator::build_forth(JitGenerator::genMod), nullptr, nullptr);
    d.addWord("NEGATE", JitGenerator::genNegate, JitGenerator::build_forth(JitGenerator::genNegate), nullptr, nullptr);
    d.addWord("INVERT", JitGenerator::genInvert, JitGenerator::build_forth(JitGenerator::genInvert), nullptr, nullptr);

    d.addWord("ABS", JitGenerator::genAbs, JitGenerator::build_forth(JitGenerator::genAbs), nullptr, nullptr);
    d.addWord("MIN", JitGenerator::genMin, JitGenerator::build_forth(JitGenerator::genMin), nullptr, nullptr);
    d.addWord("MAX", JitGenerator::genMax, JitGenerator::build_forth(JitGenerator::genMax), nullptr, nullptr);
    d.addWord("WITHIN", JitGenerator::genWithin, JitGenerator::build_forth(JitGenerator::genWithin), nullptr, nullptr);

    d.addWord("DUP", JitGenerator::genDup, JitGenerator::build_forth(JitGenerator::genDup), nullptr, nullptr);
    d.addWord("DROP", JitGenerator::genDrop, JitGenerator::build_forth(JitGenerator::genDrop), nullptr, nullptr);
    d.addWord("SWAP", JitGenerator::genSwap, JitGenerator::build_forth(JitGenerator::genSwap), nullptr, nullptr);
    d.addWord("OVER", JitGenerator::genOver, JitGenerator::build_forth(JitGenerator::genOver), nullptr, nullptr);
    d.addWord("ROT", JitGenerator::genRot, JitGenerator::build_forth(JitGenerator::genRot), nullptr, nullptr);

    // add depth
    // add nip and tuck words
    d.addWord("NIP", JitGenerator::genNip, JitGenerator::build_forth(JitGenerator::genNip), nullptr, nullptr);
    d.addWord("TUCK", JitGenerator::genTuck, JitGenerator::build_forth(JitGenerator::genTuck), nullptr, nullptr);

    // or, xor, and, not
    d.addWord("OR", JitGenerator::genOR, JitGenerator::build_forth(JitGenerator::genOR), nullptr, nullptr);
    d.addWord("XOR", JitGenerator::genXOR, JitGenerator::build_forth(JitGenerator::genXOR), nullptr, nullptr);
    d.addWord("AND", JitGenerator::genAnd, JitGenerator::build_forth(JitGenerator::genAnd), nullptr, nullptr);
    d.addWord("NOT", JitGenerator::genNot, JitGenerator::build_forth(JitGenerator::genNot), nullptr, nullptr);


    d.addWord(">R", JitGenerator::genToR, JitGenerator::build_forth(JitGenerator::genToR), nullptr, nullptr);
    d.addWord("R>", JitGenerator::genRFrom, JitGenerator::build_forth(JitGenerator::genRFrom), nullptr, nullptr);
    d.addWord("R@", JitGenerator::genRFetch, JitGenerator::build_forth(JitGenerator::genRFetch), nullptr, nullptr);
    d.addWord("RP@", JitGenerator::genRPFetch, JitGenerator::build_forth(JitGenerator::genRPFetch), nullptr, nullptr);
    d.addWord("SP", JitGenerator::genDSAT, JitGenerator::build_forth(JitGenerator::genDSAT), nullptr, nullptr);

    d.addWord("SP@", JitGenerator::genSPFetch, JitGenerator::build_forth(JitGenerator::genSPFetch), nullptr, nullptr);
    d.addWord("SP!", JitGenerator::genSPStore, JitGenerator::build_forth(JitGenerator::genSPStore), nullptr, nullptr);
    d.addWord("RP!", JitGenerator::genRPStore, JitGenerator::build_forth(JitGenerator::genRPStore), nullptr, nullptr);
    d.addWord("@", JitGenerator::genAT, JitGenerator::build_forth(JitGenerator::genAT), nullptr, nullptr);
    d.addWord("!", JitGenerator::genStore, JitGenerator::build_forth(JitGenerator::genStore), nullptr, nullptr);


    // Add immediate functions for control flow words
    d.addCompileOnlyImmediate("IF", nullptr, nullptr, JitGenerator::genIf, nullptr);
    d.addCompileOnlyImmediate("THEN", nullptr, nullptr, JitGenerator::genThen, nullptr);
    d.addCompileOnlyImmediate("ELSE", nullptr, nullptr, JitGenerator::genElse, nullptr);
    d.addCompileOnlyImmediate("BEGIN", nullptr, nullptr, JitGenerator::genBegin, nullptr);
    d.addCompileOnlyImmediate("UNTIL", nullptr, nullptr, JitGenerator::genUntil, nullptr);
    d.addCompileOnlyImmediate("WHILE", nullptr, nullptr, JitGenerator::genWhile, nullptr);
    d.addCompileOnlyImmediate("REPEAT", nullptr, nullptr, JitGenerator::genRepeat, nullptr);
    d.addCompileOnlyImmediate("AGAIN", nullptr, nullptr, JitGenerator::genAgain, nullptr);
    d.addCompileOnlyImmediate("RECURSE", nullptr, nullptr, JitGenerator::genRecurse, nullptr);
    d.addCompileOnlyImmediate("DO", nullptr, nullptr, JitGenerator::genDo, nullptr);
    d.addCompileOnlyImmediate("LOOP", nullptr, nullptr, JitGenerator::genLoop, nullptr);
    d.addCompileOnlyImmediate("+LOOP", nullptr, nullptr, JitGenerator::genPlusLoop, nullptr);
    d.addCompileOnlyImmediate("I", nullptr, nullptr, JitGenerator::genI, nullptr);
    d.addCompileOnlyImmediate("J", nullptr, nullptr, JitGenerator::genJ, nullptr);
    d.addCompileOnlyImmediate("K", nullptr, nullptr, JitGenerator::genK, nullptr);
    d.addCompileOnlyImmediate("EXIT", nullptr, nullptr, JitGenerator::genExit, nullptr);
    d.addCompileOnlyImmediate("LEAVE", nullptr, nullptr, JitGenerator::genLeave, nullptr);

    d.addCompileOnlyImmediate("CASE", nullptr, nullptr, JitGenerator::genCase, nullptr);
    d.addCompileOnlyImmediate("OF", nullptr, nullptr, JitGenerator::genOf, nullptr);
    d.addCompileOnlyImmediate("ENDOF", nullptr, nullptr, JitGenerator::genEndOf, nullptr);
    d.addCompileOnlyImmediate("DEFAULT", nullptr, nullptr, JitGenerator::genDefault, nullptr);

    d.addCompileOnlyImmediate("ENDCASE", nullptr, nullptr, JitGenerator::genEndCase, nullptr);


    d.addCompileOnlyImmediate("{", nullptr, nullptr, JitGenerator::gen_leftBrace, nullptr);

    d.addWord("to", nullptr, nullptr, JitGenerator::genTO, JitGenerator::execTO);


    //d.addWord("s.", JitGenerator::genPrint, JitGenerator::build_forth(JitGenerator::genPrint), nullptr, nullptr);

    // immediate words that create variables
    d.addInterpretOnlyImmediate("value", JitGenerator::genImmediateValue);
    d.addInterpretOnlyImmediate("fvalue", JitGenerator::genImmediateFvalue);
    d.addInterpretOnlyImmediate("array", JitGenerator::genImmediateArray);

    d.addInterpretOnlyImmediate("string", JitGenerator::genImmediateStringValue);
    d.addInterpretOnlyImmediate("constant", JitGenerator::genImmediateConstant);
    d.addInterpretOnlyImmediate("variable", JitGenerator::genImmediateVariable);

    d.addInterpretOnlyImmediate("fconstant", JitGenerator::genImmediateConstant);



    d.addWord("DEPTH", JitGenerator::genDepth2, JitGenerator::build_forth(JitGenerator::genDepth2), nullptr, nullptr);
    d.addWord("FORGET", JitGenerator::genForget, JitGenerator::build_forth(JitGenerator::genForget), nullptr, nullptr);
    d.addWord(".", JitGenerator::genDot, JitGenerator::build_forth(JitGenerator::genDot), nullptr, nullptr);
    d.addWord("h.", JitGenerator::genHDot, JitGenerator::build_forth(JitGenerator::genHDot), nullptr, nullptr);


    d.addWord("emit", JitGenerator::genEmit, JitGenerator::build_forth(JitGenerator::genEmit), nullptr, nullptr);
    d.addWord(".s", nullptr, JitGenerator::dotS, nullptr, nullptr);
    d.addWord("words", nullptr, JitGenerator::words, nullptr, nullptr);
    d.addWord("see", nullptr, nullptr, nullptr, JitGenerator::see);


    d.addWord(".\"", nullptr, nullptr, JitGenerator::genImmediateDotQuote, JitGenerator::doDotQuote);
    d.addWord("s\"", nullptr, nullptr, JitGenerator::genImmediateSQuote, JitGenerator::doSQuote);
    d.addWord("s.", JitGenerator::genPrint, JitGenerator::build_forth(JitGenerator::genPrint), nullptr, JitGenerator::genPrint);





}



int main(int argc, char* argv[]){

    jc.loggingOFF();
    add_words();
    Quit();
    return 0;
}
