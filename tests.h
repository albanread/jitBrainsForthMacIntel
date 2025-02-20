//
// Created by oberon on 07/11/2024.
//

#ifndef TESTS_H
#define TESTS_H
#include <iostream>
#include <string>
#include "CompilerUtility.h"
#include "Compiler.h"

inline int total_tests = 0;
inline int passed_tests = 0;
inline int failed_tests = 0;

inline void run_word(const std::string& word)
{
    ForthWord* w = d.findWord(word.c_str());
    if (w == nullptr)
    {
        if (is_number(word))
        {
            uint64_t num = parseNumber(word);
            sm.pushDS(num);
        }
        else
        {
            std::cout << "Word not found " << word << std::endl;
            return;
        }
    }
    else
    {
        w->compiledFunc();
    }
}

void interpreter(const std::string& sourceCode);


inline void test_against_ds(const std::string& words, const uint64_t expected_top)
{
    try
    {
        sm.resetDS(); // to get a clean stack
        std::cout << "Running: " << words << std::endl;
        interpreter(words);

        uint64_t result = sm.popDS();
        total_tests++;
        if (result != expected_top)
        {
            failed_tests++;
            std::cout << "!!!! ---- Failed test: " << words << " Expected: " << expected_top << " but got: " << result
                << " <<<<< ---- Failed test !!!" << std::endl;
        }
        else
        {
            passed_tests++;
            std::cout << "Passed test: " << words << " = " << expected_top << std::endl;
        }
    }
    catch (const std::runtime_error& e)
    {
        failed_tests++;
        std::cout << "!!!! ---- Exception occurred: " << e.what() << " for test: " << words <<
            " <<<<< ---- Failed test !!!" << std::endl;
    }
}

inline void ftest_against_ds(const std::string& words, const float expected_top)
{
    const double tolerance = 1e-4;
    try
    {
        sm.resetDS(); // Reset the data stack to get a clean stack
        std::cout << "Running: " << words << std::endl;
        interpreter(words);

        float result = sm.popDSDouble();
        total_tests++;

        if (std::abs(result - expected_top) < tolerance)
        {
            passed_tests++; // Increment passed tests instead of failed
            std::cout << "Passed test: " << words << " = " << result << std::endl;
        }
        else
        {
            failed_tests++; // Correct the logical check
            std::cout << "!!!! ---- Failed test: " << words << " Expected: " << expected_top
                << " but got: " << result << " <<<<< ---- Failed test !!!" << std::endl;
        }
    }
    catch (const std::runtime_error& e)
    {
        failed_tests++;
        std::cout << "!!!! ---- Exception occurred: " << e.what() << " for test: " << words
            << " <<<<< ---- Failed test !!!" << std::endl;
    }
}


static void testCompileAndRun(const std::string& wordName,
                              const std::string& wordDefinition,
                              const std::string& testString, int expectedResult)
{
    try
    {
        // source code is : wordname wordDefinition ;
        std::string sourceCode = wordName + " " + wordDefinition + " ;";
        compileWord(wordName, wordDefinition, sourceCode);
        test_against_ds(testString, expectedResult);
        d.forgetLastWord();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        // Handle the error as needed (e.g., logging, cleanup)
    } catch (const std::exception& e)
    {
        std::cerr << "In :" << testString << std::endl;
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        // Handle other exceptions as needed
    }
}


inline void testInterpreter(const std::string& test_name, const std::string& testString,
                            const int expectedResult)
{
    uint64_t result = sm.popDS();
    interpreter(testString); // Interpret the test string input
    total_tests++;
    if (result != expectedResult)
    {
        failed_tests++;
        std::cout << "!! ---- Failed test: "
            << test_name << " Expected: "
            << expectedResult << " <---- Failed test !!!"
            << " but got: " << result <<
            std::endl;
    }
    else
    {
        passed_tests++;
        std::cout << "Passed test: " << test_name << " = " << expectedResult << std::endl;
    }
    d.forgetLastWord();
}


void run_basic_tests()
{
    test_against_ds(" 0b10000000  ", 128);
    test_against_ds(" 0x64  ", 100);
    test_against_ds(" 16 ", 16);
    test_against_ds(" 16 16 + ", 32);
    test_against_ds(" 1 2 3 + + ", 6);
    test_against_ds(" 8 8* ", 64);
    test_against_ds(" 5 DUP * ", 25);
    test_against_ds(" 5 SQ ", 25);
    test_against_ds(" 1 2 3 OVER ", 2);
    test_against_ds(" 1 2 3 SWAP ", 2);
    test_against_ds(" 1 2 3 4 5 DEPTH ", 5);
    test_against_ds(" 1987 ", 1987);
    test_against_ds(" 1987 1+", 1988);
    test_against_ds(" 1987 1-", 1986);
    test_against_ds(" 1987 1 +", 1988);
    test_against_ds(" 1987 1 -", 1986);
    test_against_ds(" 3 4 +", 7);
    test_against_ds(" 10 2 -", 8);
    test_against_ds(" 6 3 *", 18);
    test_against_ds(" 8 2 /", 4);
    test_against_ds(" 5 DUP +", 10); // DUP duplicates 5, resulting in 5 5, then + adds them to get 10
    test_against_ds(" 1 2 SWAP", 1); // SWAP 1 2 results in 2 1, top of stack should be 1
    test_against_ds(" 1 2 OVER", 1);
    test_against_ds(" 3 4 SWAP 5", 5); // SWAP 3 4 results in 4 3, then push 5, top of stack is 5
    test_against_ds(" 2 3 4 + *", 14); // 3 + 4 = 7, 2 * 7 = 14
    test_against_ds(" 6 2 / 3 *", 9); // 6 / 2 = 3, 3 * 3 = 9
    test_against_ds(" 9 2 + 3 -", 8); // 9 + 2 = 11, 11 - 3 = 8
    test_against_ds(" 7 8 DUP + +", 23); // 8 DUP results in 7 8 8, 8 + 8 = 16, 7 + 16 = 23
    test_against_ds(" 6 4 3 OVER * +", 16); //
    test_against_ds(" 1 2 3 DROP + ", 3); //
    test_against_ds(" 0 0 +", 0);
    test_against_ds(" -1 1 +", 0);
    test_against_ds(" 1 0 -", 1);
    test_against_ds(" 1 1+ ", 2);
    test_against_ds(" 1 1- ", 0);
    test_against_ds(" 10 2 - 3 +", 11); // 10-2=8, 8+3=11
    test_against_ds("8 8/", 1); // 8 >> 3 = 1
    test_against_ds("64 8/", 8); // 64 >> 3 = 8
    test_against_ds("16 8/", 2); // 16 >> 3 = 2
    test_against_ds("7 8/", 0); // 7 >> 3 = 0
    test_against_ds("128 8/", 16); // 128 >> 3 = 16
    test_against_ds("8 4/", 2); // 8 >> 2 = 2
    test_against_ds("16 4/", 4); // 16 >> 2 = 4
    test_against_ds("64 4/", 16); // 64 >> 2 = 16
    test_against_ds("3 4/", 0); // 3 >> 2 = 0
    test_against_ds("128 4/", 32); // 128 >> 2 = 32
    test_against_ds("8 2/", 4); // 8 >> 1 = 4
    test_against_ds("16 2/", 8); // 16 >> 1 = 8
    test_against_ds("64 2/", 32); // 64 >> 1 = 32
    test_against_ds("1 2/", 0); // 1 >> 1 = 0
    test_against_ds("128 2/", 64); // 128 >> 1 = 64
    test_against_ds("1 8*", 8); // 1 << 3 = 8
    test_against_ds("2 8*", 16); // 2 << 3 = 16
    test_against_ds("4 8*", 32); // 4 << 3 = 32
    test_against_ds("8 8*", 64); // 8 << 3 = 64
    test_against_ds("16 8*", 128); // 16 << 3 = 128


    test_against_ds(" 0 invert ", -1);
    test_against_ds(" 1 invert ", -2);
    test_against_ds(" 2 invert ", -3);


    // Test if 3 is less than 5 (should be -1, which represents true in Forth)
    test_against_ds("3 5 <", -1);

    // Test if 5 is less than 3 (should be 0, which represents false in Forth)
    test_against_ds("5 3 <", 0);

    // Test if 5 is greater than 3 (should be -1, which represents true in Forth)
    test_against_ds("5 3 >", -1);

    // Test if 3 is greater than 5 (should be 0, which represents false in Forth)
    test_against_ds("3 5 >", 0);

    // Test if 5 is equal to 5 (should be -1, which represents true in Forth)
    test_against_ds("5 5 =", -1);

    // Test if 5 is equal to 3 (should be 0, which represents false in Forth)
    test_against_ds("5 3 =", 0);

    test_against_ds("-1 0=", 0);

    test_against_ds("0 0=", -1);

    // Tests for 0<
    test_against_ds("0 0<", 0); // 0 is not less than 0, hence false (0)
    test_against_ds("1 0<", 0); // 1 is not less than 0, hence false (0)
    test_against_ds("-1 0<", -1); // -1 is less than 0, hence true (-1)
    test_against_ds("10 0<", 0); // 10 is not less than 0, hence false (0)
    test_against_ds("-10 0<", -1); // -10 is less than 0, hence true (-1)

    // Tests for 0>
    test_against_ds("0 0>", 0); // 0 is not greater than 0, hence false (0)
    test_against_ds("1 0>", -1); // 1 is greater than 0, hence true (-1)
    test_against_ds("-1 0>", 0); // -1 is not greater than 0, hence false (0)
    test_against_ds("10 0>", -1); // 10 is greater than 0, hence true (-1)
    test_against_ds("-10 0>", 0); // -10 is not greater than 0, hence false (0)

    // Test >R followed by R> (should push 5 to RS and then pop it back to DS, so DS ends with 5)
    test_against_ds("5 >R R>", 5);

    // Test >R followed by R@ and R> (should push 5 to RS, duplicate it on DS, then pop it back to DS; result should be 5 on DS)
    test_against_ds("5 >R R@ R>", 5);

    // Test >R followed by R@ and another R@ (should push 5 to RS, then copy it to DS twice, then we only care about the first value; result should be 5 on DS)
    test_against_ds("5 >R R@ R@ + ", 10);

    // value and variable tests
    test_against_ds(" variable fred 110 fred ! fred @ forget ", 110);

    test_against_ds(" variable fred 120 to fred fred @   ", 120);

    test_against_ds(" 77 value testval testval forget ", 77);

    test_against_ds(" 77 value testval 99 to testval testval forget ", 99);


    // compiled word tests
    testCompileAndRun("testWord",
                      " 100 + ",
                      "1 testWord ",
                      101);

    testCompileAndRun("testWord",
                      " 0 11 1 do I + LOOP ",
                      " testWord ",
                      55);

    testCompileAndRun("testBeginAgain",
                      " 0 BEGIN DUP 10 < WHILE 1+ AGAIN  ",
                      " testBeginAgain ",
                      10); //

    testCompileAndRun("testBeginWhileRepeat",
                      " BEGIN DUP 10 < WHILE 1+ REPEAT ",
                      " 0 testBeginWhileRepeat ",
                      10); // Expected result when while condition fails

    testCompileAndRun("testBeginUntil",
                      " 0 BEGIN 1+ DUP 10 = UNTIL ",
                      " 10 testBeginUntil ",
                      10); // Expected result after the loop is terminated


    testCompileAndRun("testNestedIfElse",
                      " IF IF 1 ELSE 2 THEN ELSE 3 THEN ",
                      " -1 0 testNestedIfElse ",
                      3);

    testCompileAndRun("testNestedIfElse",
                      " IF IF 1 ELSE 2 THEN ELSE 3 THEN ",
                      " 0 0 testNestedIfElse ",
                      3);

    testCompileAndRun("testNestedIfElse",
                      " IF IF 1 ELSE 2 THEN ELSE 3 THEN ",
                      " 0 -1 testNestedIfElse ",
                      2);

    testCompileAndRun("testIfElse",
                      " IF 1 ELSE 2 THEN ",
                      " 0 testIfElse ",
                      2);

    testCompileAndRun("testIfElse",
                      " IF 1 ELSE 2 THEN ",
                      " -1 testIfElse ",
                      1);


    // test loop containing if then
    testCompileAndRun("testBeginUntilNestedIF",
                      " 0 BEGIN 1+ DUP 5 > IF 65 emit THEN DUP 10 = UNTIL ",
                      " 8 testBeginUntilNestedIF ",
                      10);


    // this is using "LEAVE" properly
    testCompileAndRun("testBeginUntilEarlyLeave",
                      " 0 BEGIN 1+ DUP 5 > IF LEAVE THEN DUP 10 = UNTIL ",
                      " 0 testBeginUntilEarlyLeave ",
                      6);


    // this is using "LEAVE" properly
    testCompileAndRun("testBeginAGAINLeave",
                      " 0 BEGIN 1+ DUP 5 > IF LEAVE THEN AGAIN ",
                      " 0 testBeginAGAINLeave ",
                      6);


    testCompileAndRun("testDoLoop",
                      " DO I LOOP ",
                      " 10 1 testDoLoop ",
                      9);

    testCompileAndRun("testDoPlusLoop",
                      " DO I 2 +LOOP ",
                      " 10 1 testDoPlusLoop ",
                      9);


    testCompileAndRun("testDoPlusLoop",
                      " DO I 2 +LOOP ",
                      " 10 1 testDoPlusLoop ",
                      9);


    testCompileAndRun("testThreeLevelDeepLoop",
                      " 3 0 DO  2 0  DO  1 0 DO I J K + + LOOP LOOP LOOP ",
                      " testThreeLevelDeepLoop ",
                      5);


    test_against_ds(" 20 value test test ", 20);

    testCompileAndRun("testValues",
                      " test   ",
                      " testValues ",
                      20);

    testCompileAndRun("testValues",
                      " 30 to test test  ",
                      " testValues ",
                      30);

    test_against_ds(" variable tim 10 tim ! tim @ ", 10);

    testCompileAndRun("testVariables",
                      " 30 to tim tim @ ",
                      "  testVariables ",
                      30);

    testCompileAndRun("testLocals",
                      " { a b } a b + ",
                      " 10 1 testLocals ",
                      11);

    testCompileAndRun("testLocals2",
                      " { a b | c } a b + to c c ",

                      " 10 6 testLocals2 ",
                      16);

    testCompileAndRun("testLocals3",
                      " { a b | c -- d } a b + to c c 2* to d ",

                      " 9 10 6 testLocals3 ",
                      32);


    test_against_ds(" TRUE ", -1);
    test_against_ds(" FALSE ", 0);


    test_against_ds(" 10 3 MOD ", 1); // 10 % 3 = 1
    test_against_ds(" 5 NEGATE ", -5); // -5
    test_against_ds(" -7 ABS ", 7); // | -7 | = 7
    test_against_ds(" 4 9 MIN ", 4); // min(4, 9) = 4
    test_against_ds(" 15 5 MAX ", 15); // max(15, 5) = 15

    test_against_ds(" 5 1 10 WITHIN ", -1); // 5 is within the range [1, 10), should return -1 (true)
    test_against_ds(" 0 1 10 WITHIN ", 0); // 0 is not within the range [1, 10), should return 0 (false)
    test_against_ds(" 10 1 10 WITHIN ", 0); // 10 is not within the range [1, 10), should return 0 (false)
    test_against_ds(" 15 1 10 WITHIN ", 0); // 15 is not within the range [1, 10), should return 0 (false)
    test_against_ds(" 5 5 10 WITHIN ", -1); // 5 is within the range [5, 10), should return -1 (true)

    test_against_ds(" CHAR a  ", 97);

    testCompileAndRun("testChar",
                      " CHAR A",

                      " testChar ",
                      65);


    test_against_ds(" 7 constant daysInWeek daysInWeek ", 7);

    // test_against_ds(" 12 to daysInWeek daysInWeek ", 7);

    // tidy
    test_against_ds(" forget forget forget forget 10 ", 10);

    // fact - crashy
    testCompileAndRun("factTest",
                      "dup 2 < if drop 1 exit then dup begin dup 2 > while 1- swap over * swap repeat drop ",
                      " 5 factTest",
                      120);

    testCompileAndRun("rfactTest",
                      "DUP 2 < IF DROP 1 EXIT THEN  DUP 1- RECURSE * ",
                      " 5 rfactTest",
                      120);


    testCompileAndRun("testcase",
                      R"(
                      CASE
                        1 OF
                          10
                        ENDOF
                        2 OF
                          20
                        ENDOF
                        3 OF
                          30
                        ENDOF
                        DEFAULT
                          40
                      ENDCASE )",
                      " 3 testcase",
                      30);




    testCompileAndRun("testcase",
                    R"(
                      CASE
                        1 OF
                          10
                        ENDOF
                        2 OF
                          20
                        ENDOF
                        3 OF
                          30
                        ENDOF
                        DEFAULT
                          40
                      ENDCASE )",
                    " 2 testcase",
                    20);


    testCompileAndRun("testcase",
                R"(
                      CASE
                        1 OF
                          10
                        ENDOF
                        2 OF
                          20
                        ENDOF
                        3 OF
                          30
                        ENDOF
                        DEFAULT
                          40
                      ENDCASE )",
                " 3 testcase",
                30);



    testCompileAndRun("testcase",
            R"(
                      CASE
                        1 OF
                          10
                        ENDOF
                        2 OF
                          20
                        ENDOF
                        3 OF
                          30
                        ENDOF
                        DEFAULT
                          40
                      ENDCASE )",
            " 99 testcase",
            40);



    testCompileAndRun("nestedcase",
        R"(
                      CASE
                        1 OF
                          10
                        ENDOF
                        2 OF
                         CASE
                            1 OF
                              10
                            ENDOF
                            2 OF
                              200
                            ENDOF
                            3 OF
                              30
                            ENDOF
                            DEFAULT
                              40
                         ENDCASE
                        ENDOF
                        3 OF
                          30
                        ENDOF
                        DEFAULT
                          40
                      ENDCASE )",
        " 2 2 nestedcase",
        200);


    ftest_against_ds("3.14159", 3.14159); // Single float value
    ftest_against_ds("2.0 2.0 f+", 4.0); // Addition resulting in a float
    ftest_against_ds("5.0 1.0 f-", 4.0); // Subtraction resulting in a float

    ftest_against_ds("10.0 2.0 f/", 5.0); // Division resulting in a float
    ftest_against_ds("3.0 2.0 f*", 6.0); // Multiplication resulting in a float
    ftest_against_ds("-3.0 fabs", 3.0);
    ftest_against_ds("5.5 2.0 fmod", 1.5);


    // ### Cosine Function Test Cases
    //
    //     | Input (Radians) | Expected Result (Decimal) |
    //     |-----------------|---------------------------|
    //     | 0.0             | 1.0                       |
    //     | 1.5708          | 0.0                       |
    //     | 3.1416          | -1.0                      |
    //     | 4.7124          | 0.0                       |
    //     | 6.2832          | 1.0                       |
    //     | 1.0472          | 0.5                       |
    //     | 0.7854          | 0.7071                    |
    //     | 0.5236          | 0.8660                    |


    // ftest_against_ds("0.0 fcos", 1.0);
    // ftest_against_ds("1.5708 fcos", 0.0);
    // ftest_against_ds("3.1416 fcos", -1.0);
    // ftest_against_ds("4.7124 fcos", 0.0);
    // ftest_against_ds("6.2832 fcos", 1.0);
    // ftest_against_ds("1.0472 fcos", 0.5);
    // ftest_against_ds("0.7854 fcos", 0.7071);
    // ftest_against_ds("0.5236 fcos", 0.8660);
    //
    // ### Sine Function Test Cases
    //
    //     | Input (Radians) | Expected Result (Decimal) |
    //     |-----------------|---------------------------|
    //     | 0.0             | 0.0                       |
    //     | 1.5708          | 1.0                       |
    //     | 3.1416          | 0.0                       |
    //     | 4.7124          | -1.0                      |
    //     | 6.2832          | 0.0                       |
    //     | 1.0472          | 0.8660                    |
    //     | 0.7854          | 0.7071                    |
    //     | 0.5236          | 0.5                       |

    // ftest_against_ds("0.0 fsin", 0.0);
    // ftest_against_ds("1.5708 fsin", 1.0);
    // ftest_against_ds("3.1416 fsin", 0.0);
    // ftest_against_ds("4.7124 fsin", -1.0);
    // ftest_against_ds("6.2832 fsin", 0.0);
    // ftest_against_ds("1.0472 fsin", 0.8660);
    // ftest_against_ds("0.7854 fsin", 0.7071);
    // ftest_against_ds("0.5236 fsin", 0.5);

    // f<
    test_against_ds("1.0 2.0 f<", -1); // 1.0 < 2.0 is true
    test_against_ds("2.0 1.0 f<", 0); // 2.0 < 1.0 is false
    test_against_ds("1.0 1.0 f<", 0); // 1.0 < 1.0 is false

    // f>
    test_against_ds("2.0 1.0 f>", -1); // 2.0 > 1.0 is true
    test_against_ds("1.0 2.0 f>", 0); // 1.0 > 2.0 is false
    test_against_ds("1.0 1.0 f>", 0); // 1.0 > 1.0 is false

    // f=
    test_against_ds("1.0 1.0 f=", -1); // 1.0 = 1.0 is true
    test_against_ds("1.0 2.0 f=", 0); // 1.0 = 2.0 is false

    // f<>
    test_against_ds("1.0 2.0 f<>", -1); // 1.0 <> 2.0 is true
    test_against_ds("2.0 1.0 f<>", -1); // 2.0 <> 1.0 is true
    test_against_ds("1.0 1.0 f<>", 0); // 1.0 <> 1.0 is false


    // Print summary after running tests
    std::cout << "\nTest results:" << std::endl;
    std::cout << "Total tests run: " << total_tests << std::endl;
    std::cout << "Passed tests: " << passed_tests << std::endl;
    std::cout << "Failed tests: " << failed_tests << std::endl;
}
#endif //TESTS_H
