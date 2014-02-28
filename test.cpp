#include "ginger.h"
#include <iostream>
#include <ios>
#include <vector>
#include <string>

std::string getStdin() {
    std::string output;
    std::string input;
    std::cin >> std::noskipws;
    while (std::getline(std::cin, input)) {
        output += input + '\n';
    }
    return output;
}

int failed = 0;

void print_error(int line, std::string input, std::string expected, std::string actual, std::string error = "") {
    std::cerr << "------------- TEST ERROR (" << line << ") ---------------" << std::endl;
    /*
    for (auto c: expected)
        std::cout << (int)c << std::endl;
    std::cout << "---" << std::endl;
    for (auto c: actual)
        std::cout << (int)c << std::endl;
    */
    std::cerr << "input: " << input << std::endl;
    std::cerr << "expected: " << expected << std::endl;
    std::cerr << "actual: " << actual << std::endl;
    std::cerr << "error: " <<  error << std::endl;
    ++failed;
}
void test_eq(std::string input, std::string expected, ginger::temple* p, int line) {
    try {
        std::stringstream ss;
        std::stringstream ess;
        ginger::parse(input, p ? *p : ginger::temple(), ginger::from_ios(ss), ginger::from_ios(ess));
        auto actual = ss.str();
        if (actual != expected) {
            auto error = ess.str();
            print_error(line, input, expected, actual, error);
        }
    } catch (int gline) {
        print_error(line, input, expected, "EXCEPTION (" + std::to_string(gline) + ")");
    } catch (...) {
        print_error(line, input, expected, "UNKNOWN EXCEPTION");
    }
}
#define TEST_EQ(input, expected) test_eq(input, expected, nullptr, __LINE__)
#define TEST_EQ_T(input, expected, t) test_eq(input, expected, &t, __LINE__)

int main() {
    TEST_EQ("Hello", "Hello");
    TEST_EQ("${{", "{{");
    TEST_EQ("$}}", "}}");
    TEST_EQ("$$", "$");
    TEST_EQ("$# comment", "");
    TEST_EQ("$# comment\n", "\n");
    // test ${variable}
    {
        ginger::temple t;
        t["value"] = 100;
        t["map"] = std::map<std::string, int>{ { "hoge", 1 }, { "fuga", 2 } };
        TEST_EQ_T("${value}", "100", t);
        TEST_EQ_T("${map.hoge}, ${map.fuga}", "1, 2", t);
    }
    // test $for
    {
        ginger::temple t;
        t["xs"] = std::vector<int>{ 1, 2 };
        t["ys"] = std::map<std::string, std::vector<int>>{ { "hoge", { 1, 2, 3 } } };
        TEST_EQ_T("$for x in xs{{test}}", "testtest", t);
        TEST_EQ_T("$for x in xs {{ test }}", " test  test ", t);
        TEST_EQ_T("$for x in xs{{$for x in xs{{test}}}}", "testtesttesttest", t);
        TEST_EQ_T("$for y in ys.hoge{{${y}}}", "123", t);
        TEST_EQ_T("$for y in  \t  ys.hoge   {{${y}\n}}", "1\n2\n3\n", t);
    }
    // test $if
    {
        ginger::temple t;
        t["true"] = true;
        t["false"] = false;

        // $if
        TEST_EQ_T("$if true {{hoge}}", "hoge", t);
        TEST_EQ_T("$if false{{hoge}}", "", t);

        // $elseif
        TEST_EQ_T("$if true{{hoge}}$elseif true{{fuga}}", "hoge", t);
        TEST_EQ_T("$if true{{hoge}}$elseif undefined{{${undefined}}}", "hoge", t);
        TEST_EQ_T("$if false{{hoge}}$elseif true{{fuga}}", "fuga", t);
        TEST_EQ_T("$if false{{hoge}}$elseif true{{fuga}} $elseif undefined {{ fuga2 }}", "fuga", t);
        TEST_EQ_T("$if false{{hoge}}$elseif false{{fuga}} $elseif true {{ fuga2 }}", " fuga2 ", t);

        // $else
        TEST_EQ_T("$if true {{hoge}}$else{{moke}}", "hoge", t);
        TEST_EQ_T("$if true {{hoge}} $else {{${undefined}}}", "hoge", t);
        TEST_EQ_T("$if false {{hoge}} $elseif true {{fuga}} $else{{moke}}", "fuga", t);
        TEST_EQ_T("$if false {{hoge}} $elseif false {{fuga}} $else{{moke}}", "moke", t);

        // confusing case
        TEST_EQ_T("$if true {{hoge}} ${true}", "hoge 1", t);
        TEST_EQ_T("$if true {{hoge}}${{", "hoge{{", t);
    }
    if (failed != 0) {
        std::cerr << "------- TEST FAILED --------" << std::endl;
        std::cerr << "FAILED COUNT: " << failed << std::endl;
        std::exit(1);
    }

    /*
    ginger::temple t;
    t["xs"] = std::vector<int>{ 1, 2, 3, 4 };
    t["value"] = "hoge-";
    t["x"] = true;
    t["p"] = false;
    t["q"] = false;
    t["ys"] = std::map<
                    std::string,
                    std::vector<
                        std::map<
                            std::string,
                            ginger::object>>> {
        { "values", {
            { { "test", true }, { "fuga", "aaaa" }, { "moke", "bbbb" } },
            { { "test", false }, { "fuga", "cccc" } },
        } },
    };

    std::string input = getStdin();
    try {
        ginger::parse(input, t);
    } catch (int line) {
        std::cerr << "error: " << line << std::endl;
    }
    */
}