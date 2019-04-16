#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;
using namespace std;

#pragma region Range Struct

struct Range
{
    int left;
    int right;

    bool divisionByZero;

    bool impossibleRange;

    Range()
    {
        left = 0;
        right = 0;
        divisionByZero = false;
        impossibleRange = false;
    }

    Range(int v)
    {
        left = v;
        right = v;
        divisionByZero = false;
        impossibleRange = false;
        range_sanity();
    }

    Range(int a, int b)
    {
        if (a > b)
        {
            right = a;
            left = b;
        }
        else
        {
            right = b;
            left = a;
        }
        divisionByZero = false;
        impossibleRange = false;
        range_sanity();
    }

    Range(int a, int b, bool dz)
    {
        if (a > b)
        {
            right = a;
            left = b;
        }
        else
        {
            right = b;
            left = a;
        }
        divisionByZero = dz;
        impossibleRange = false;
        range_sanity();
    }

    Range(int a, int b, bool dz, bool ir)
    {
        if (a > b)
        {
            right = a;
            left = b;
        }
        else
        {
            right = b;
            left = a;
        }
        divisionByZero = dz;
        impossibleRange = ir;
        range_sanity();
    }

    Range(Range const &r)
    {
        left = r.left;
        right = r.right;
        divisionByZero = r.divisionByZero;
        impossibleRange = r.impossibleRange;
        range_sanity();
    }

    bool operator==(Range r)
    {
        return (left == r.left) && (right == r.right);
    }

    bool operator!=(Range r)
    {
        return (left != r.left) || (right != r.right);
    }

    void range_sanity()
    {
        if (left < -1000)
        {
            left = -1000;
        }

        if (right > 1000)
        {
            right = 1000;
        }
    }
};

bool operator==(const Range l, const Range r)
{
    return (l.left == r.left) && (l.right == r.right);
}

#pragma endregion

#pragma region Global Variables

// Variable / register name to range
typedef std::map<string, Range> ValueAnalysis;

// Basic block to value analysis
// Widening
map<string, ValueAnalysis> wideValueAnalysisMap;
map<string, bool> wideImpossibleBlock;
// Narrowing
map<string, ValueAnalysis> narrowValueAnalysisMap;
map<string, bool> narrowImpossibleBlock;

// Basic block to range for diff analysis
map<string, map<string, int>> diffAnalysisMap;

int narrowing_count = 0;
int widening_count = 0;

bool print_steps = true;

bool print_infinity = false;

#pragma endregion

#pragma region Narrowing

// All implementations are simple approximations using range computation

Range narrow_all()
{
    return Range(-1000, 1000);
}

Range narrow_str(int v)
{
    return Range(v);
}

Range narrow_str(Range r)
{
    return Range(r);
}

Range narrow_add(Range l, Range r)
{
    set<int> potentialValues;

    potentialValues.insert(l.left + r.left);
    potentialValues.insert(l.left + r.right);
    potentialValues.insert(l.right + r.left);
    potentialValues.insert(l.right + r.right);

    if (l.left == -1000 || r.left == -1000)
    {
        potentialValues.insert(-1000);
    }
    if (l.left == 1000 || r.left == 1000)
    {
        potentialValues.insert(1000);
    }
    if (l.right == -1000 || r.right == -1000)
    {
        potentialValues.insert(-1000);
    }
    if (l.right == 1000 || r.right == 1000)
    {
        potentialValues.insert(1000);
    }

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR);
}

Range narrow_sub(Range l, Range r)
{
    set<int> potentialValues;

    potentialValues.insert(l.left - r.left);
    potentialValues.insert(l.left - r.right);
    potentialValues.insert(l.right - r.left);
    potentialValues.insert(l.right - r.right);

    if (l.left == -1000 || r.left == 1000)
    {
        potentialValues.insert(-1000);
    }
    if (l.left == 1000 || r.left == -1000)
    {
        potentialValues.insert(1000);
    }
    if (l.right == 1000 || r.right == -1000)
    {
        potentialValues.insert(1000);
    }
    if (l.right == -1000 || r.right == 1000)
    {
        potentialValues.insert(-1000);
    }

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR);
}

Range narrow_mul(Range l, Range r)
{
    set<int> potentialValues;

    potentialValues.insert(l.left * r.left);
    potentialValues.insert(l.left * r.right);
    potentialValues.insert(l.right * r.left);
    potentialValues.insert(l.right * r.right);

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR);
}

Range narrow_div(Range l, Range r)
{
    if (r.left == 0 && r.right == 0)
    {
        // Broken analysis
        return Range(0, 0, true);
    }

    bool divisionByZero = false;
    if (r.left == 0 || r.right == 0 || (r.left < 0 && r.right > 0))
    {
        // Potential of division by zero, not useful for the assignment analysis but need to be taken care of
        divisionByZero = true;
    }

    set<int> potentialValues;

    if (r.left == 0)
    {
        // Take the values by itself as if divided by 1
        potentialValues.insert(l.left);
        potentialValues.insert(l.right);
    }
    else
    {
        potentialValues.insert(l.left / r.left);
        potentialValues.insert(l.right / r.left);
    }

    if (r.right == 0)
    {
        // Take the values by itself as if divided by -1
        potentialValues.insert(-1 * l.left);
        potentialValues.insert(-1 * l.right);
    }
    else
    {
        potentialValues.insert(l.left * r.right);
        potentialValues.insert(l.right * r.right);
    }

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR, divisionByZero);
}

Range narrow_rem(Range l, Range r)
{
    if (r.left == 0 && r.right == 0)
    {
        // Broken analysis
        return Range(0, 0, true);
    }

    bool divisionByZero = false;
    if (r.left == 0 || r.right == 0 || (r.left < 0 && r.right > 0))
    {
        // Potential of division by zero, not useful for the assignment analysis but need to be taken care of
        divisionByZero = true;
    }

    // For remainder values, it will always only depend on r values, values are from 0 to r.right
    // If there are negative values at either side, insert from -r.right to r.right
    // If all values are negative then rem is only positive

    int newL = 0;
    int newR = 0;

    if (r.right >= 0)
    {
        newL = 0;
        newR = r.right - 1;

        if (l.left < 0)
        {
            newL = -r.right + 1;
        }
    }
    else
    {
        newL = r.right + 1;
        newR = 0;

        if (l.right > 0)
        {
            newR = (-r.right) - 1;
        }
    }

    return Range(newL, newR, divisionByZero);
}

Range narrow_combine(Range l, Range r)
{
    set<int> potentialValues;

    potentialValues.insert(l.left);
    potentialValues.insert(l.right);
    potentialValues.insert(r.left);
    potentialValues.insert(r.right);

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR);
}

// Retrieves the range for l in case of true = l gt r
Range narrow_gt(Range l, Range r)
{
    int newL;

    if (r.left == l.left)
    {
        if (r.left >= l.right)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = r.left + 1;
        }
    }
    else if (r.left < l.left)
    {
        newL = l.left;
    }
    else
    {
        if (r.left >= l.right)
        {
            //impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = r.left + 1;
        }
    }

    int newR;

    if (l.right == r.right)
    {
        if (l.right <= r.left)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newR = l.right;
        }
    }
    else if (l.right > r.right)
    {
        newR = l.right;
    }
    else
    {
        if (l.right <= r.left)
        {
            return Range(0, 0, false, true);
        }
        else
        {
            newR = l.right;
        }
    }
    return Range(newL, newR);
}

Range narrow_ge(Range l, Range r)
{
    int newL;

    if (r.left == l.left)
    {
        newL = l.left;
    }
    else if (r.left < l.left)
    {
        newL = l.left;
    }
    else
    {
        if (r.left > l.right)
        {
            //impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = r.left;
        }
    }

    int newR;

    if (l.right == r.right)
    {
        newR = l.right;
    }
    else if (l.right > r.right)
    {
        newR = l.right;
    }
    else
    {
        if (l.right < r.left)
        {
            return Range(0, 0, false, true);
        }
        else
        {
            newR = l.right;
        }
    }
    return Range(newL, newR);
}

Range narrow_lt(Range l, Range r)
{
    int newL;

    if (r.left == l.left)
    {
        if (l.left >= r.right)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = l.left;
        }
    }
    else if (r.left > l.left)
    {
        newL = l.left;
    }
    else
    {
        if (l.left >= r.right)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = l.left;
        }
    }

    int newR;

    if (l.right == r.right)
    {
        if (r.right <= l.left)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newR = l.right - 1;
        }
    }
    else if (l.right < r.right)
    {
        newR = l.right;
    }
    else
    {
        if (r.right <= l.left)
        {
            return Range(0, 0, false, true);
        }
        else
        {
            newR = r.right - 1;
        }
    }
    return Range(newL, newR);
}

Range narrow_le(Range l, Range r)
{
    int newL;

    if (r.left == l.left)
    {
        newL = l.left;
    }
    else if (r.left > l.left)
    {
        newL = l.left;
    }
    else
    {
        if (l.left > r.right)
        {
            // impossible case
            return Range(0, 0, false, true);
        }
        else
        {
            newL = l.left;
        }
    }

    int newR;

    if (l.right == r.right)
    {
        newR = l.right;
    }
    else if (l.right < r.right)
    {
        newR = l.right;
    }
    else
    {
        if (r.right < l.left)
        {
            return Range(0, 0, false, true);
        }
        else
        {
            newR = r.right;
        }
    }
    return Range(newL, newR);
}

Range narrow_eq(Range l, Range r)
{
    int newL;

    if (l.left <= r.left)
    {
        newL = r.left;
    }
    else
    {
        newL = l.left;
    }

    int newR;

    if (l.right <= r.right)
    {
        newR = l.right;
    }
    else
    {
        newR = r.right;
    }

    if (newL > newR)
    {
        // impossible case
        return Range(0, 0, false, true);
    }

    return Range(newL, newR);
}

ValueAnalysis narrow_combine(ValueAnalysis left, ValueAnalysis right)
{
    ValueAnalysis combined;

    set<string> keys;

    for (auto it = left.begin(); it != left.end(); ++it)
    {
        keys.insert(it->first);
    }

    for (auto it = right.begin(); it != right.end(); ++it)
    {
        keys.insert(it->first);
    }

    for (string key : keys)
    {
        bool leftExist = left.find(key) != left.end();
        bool rightExist = right.find(key) != right.end();

        if (leftExist && rightExist)
        {
            combined[key] = narrow_combine(left[key], right[key]);
        }
        else if (leftExist && (!rightExist))
        {
            combined[key] = Range(left[key]);
        }
        else if ((!leftExist) && rightExist)
        {
            combined[key] = Range(right[key]);
        }
    }

    return combined;
}

#pragma endregion

#pragma region Widening

int widen_left(int v)
{
    if (v < -100)
    {
        return -1000;
    }

    if (v < -10)
    {
        return -100;
    }

    if (v < -1)
    {
        return -10;
    }

    if (v == -1 || v == 0 || v == 1)
    {
        return v;
    }

    if (v < 10)
    {
        return 1;
    }

    if (v < 100)
    {
        return 10;
    }

    if (v < 1000)
    {
        return 100;
    }

    return 1000;
}

int widen_right(int v)
{
    if (v > 100)
    {
        return 1000;
    }

    if (v > 10)
    {
        return 100;
    }

    if (v > 1)
    {
        return 10;
    }

    if (v == -1 || v == 0 || v == 1)
    {
        return v;
    }

    if (v > -10)
    {
        return -1;
    }

    if (v > -100)
    {
        return -10;
    }

    if (v > -1000)
    {
        return -100;
    }

    return -1000;
}

Range widen_all()
{
    return Range(-1000, 1000);
}

Range widen_str(int v)
{
    return Range(widen_left(v), widen_right(v));
}

Range widen_str(Range r)
{
    int newL = widen_left(r.left);
    int newR = widen_right(r.right);

    return Range(newL, newR);
}

Range widen_add(Range l, Range r)
{
    return widen_str(narrow_add(l, r));
}

Range widen_sub(Range l, Range r)
{
    return widen_str(narrow_sub(l, r));
}

Range widen_mul(Range l, Range r)
{
    return widen_str(narrow_mul(l, r));
}

Range widen_div(Range l, Range r)
{
    return widen_str(narrow_div(l, r));
}

Range widen_rem(Range l, Range r)
{
    return widen_str(narrow_rem(l, r));
}

Range widen_combine(Range l, Range r)
{
    return widen_str(narrow_combine(l, r));
}

Range widen_gt(Range l, Range r)
{
    return widen_str(narrow_gt(l, r));
}

Range widen_ge(Range l, Range r)
{
    return widen_str(narrow_ge(l, r));
}

Range widen_lt(Range l, Range r)
{
    return widen_str(narrow_lt(l, r));
}

Range widen_le(Range l, Range r)
{
    return widen_str(narrow_le(l, r));
}

Range widen_eq(Range l, Range r)
{
    return widen_str(narrow_eq(l, r));
}

ValueAnalysis widen_combine(ValueAnalysis left, ValueAnalysis right)
{
    ValueAnalysis combined;

    set<string> keys;

    for (auto it = left.begin(); it != left.end(); ++it)
    {
        keys.insert(it->first);
    }

    for (auto it = right.begin(); it != right.end(); ++it)
    {
        keys.insert(it->first);
    }

    for (string key : keys)
    {
        bool leftExist = left.find(key) != left.end();
        bool rightExist = right.find(key) != right.end();

        if (leftExist && rightExist)
        {
            combined[key] = widen_combine(left[key], right[key]);
        }
        else if (leftExist && (!rightExist))
        {
            combined[key] = Range(left[key]);
        }
        else if ((!leftExist) && rightExist)
        {
            combined[key] = Range(right[key]);
        }
    }

    return combined;
}

#pragma endregion

#pragma region Labels

string label(Range r)
{
    string divByZero = r.divisionByZero ? "true" : "false";
    string impossibleRange = r.impossibleRange ? "true" : "false";
    return "[" + to_string(r.left) + ", " + to_string(r.right) + "] - Division By Zero: " + divByZero + " - Impossible Range: " + impossibleRange;
}

string label(const Value *Node)
{
    if (!Node->getName().empty())
        return Node->getName().str();

    string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}

#pragma endregion

#pragma region Printing

void print(const ValueAnalysis valueAnalysis)
{
    for (auto &row : valueAnalysis)
    {
        string variable = row.first;
        Range variableRange = row.second;
        outs() << "\t" << variable << "\t : " << label(variableRange) << "\n";
    }
}

void print(const map<string, int> valueAnalysis)
{
    for (auto &row : valueAnalysis)
    {
        string variable = row.first;
        int variableRange = row.second;
        if ((!print_infinity) && variableRange == 1000)
        {
            continue;
        }
        outs() << "\t" << variable << "\t : " << to_string(variableRange) << "\n";
    }
}

void print(const map<string, map<string, int>> analysisMap)
{
    for (auto &row : analysisMap)
    {
        string blockLabel = row.first;
        map<string, int> maxDiffAnalysis = row.second;
        outs() << blockLabel << " : \n";
        print(maxDiffAnalysis);
    }
}

void print(const map<string, ValueAnalysis> analysisMap)
{
    for (auto &row : analysisMap)
    {
        string blockLabel = row.first;
        ValueAnalysis valueAnalysis = row.second;
        outs() << blockLabel << " : \n";
        print(valueAnalysis);
    }
}

void print(const set<string> stringSet)
{
    outs() << "Set \t: {";
    bool first = true;
    for (string var : stringSet)
    {
        if (!first)
        {
            outs() << ", ";
        }
        outs() << var;
        first = false;
    }
    outs() << "} \n";
}

void print(const map<string, set<string>> analysisMap)
{
    for (auto &row : analysisMap)
    {
        string BBLabel = row.first;

        outs() << BBLabel << "\t: {";
        bool first = true;
        for (string var : row.second)
        {
            if (!first)
            {
                outs() << ", ";
            }
            outs() << var;
            first = false;
        }
        outs() << "} \n";
    }
}

void print(const Value *bb)
{
    outs() << "Instruction:";
    bb->print(outs());
    outs() << "\n";
}

void print(const Value *bb, string label)
{
    outs() << label + ":";
    bb->print(outs());
    outs() << "\n";
}

#pragma endregion

#pragma region Helper

bool impossible_path(const ValueAnalysis valueAnalysis)
{
    for (auto &row : valueAnalysis)
    {
        if (row.second.impossibleRange)
        {
            return true;
        }
    }
    return false;
}

string get_load_store_label(Value &v)
{
    Instruction &I = cast<Instruction>(v);
    if (isa<StoreInst>(I))
    {
        return label(I.getOperand(1));
    }
    else
    {
        return label(I.getOperand(0));
    }
}

int get_const_value(Value &v)
{
    llvm::ConstantInt *CI = dyn_cast<ConstantInt>(&v);
    return CI->getSExtValue();
}

Function *init(unique_ptr<Module> *m)
{
    Function *F = (*m)->getFunction("main");

    for (auto &BB : *F)
    {
        ValueAnalysis widenEmpty, narrowEmpty;
        wideValueAnalysisMap[label(&BB)] = widenEmpty;
        narrowValueAnalysisMap[label(&BB)] = narrowEmpty;
    }

    return F;
}

BasicBlock *findMain(unique_ptr<Module> *m)
{
    for (auto &F : **m)
    {
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0)
        {
            BasicBlock *BB = dyn_cast<BasicBlock>(F.begin());
            return BB;
        }
    }
    return NULL;
}

#pragma endregion

#pragma region Widen Flow

ValueAnalysis widen_pred_cond_branch(ValueAnalysis predAnalysis, Instruction &inst, BasicBlock *current)
{
    ValueAnalysis result = ValueAnalysis(predAnalysis);

    BranchInst *br = dyn_cast<BranchInst>(&inst);

    int numOperands = br->getNumOperands();
    if (numOperands == 1)
    {
        // if end case
        return result;
    }

    // other cases will have 3 operands
    bool cmpResult = false;
    if (current == br->getOperand(2))
    {
        // if then
        cmpResult = true;
    }
    if (current == br->getOperand(1))
    {
        // if else
        cmpResult = false;
    }

    CmpInst &compareInst = cast<CmpInst>(*(br->getOperand(0)));
    Value *leftOperand = compareInst.getOperand(0);
    Value *rightOperand = compareInst.getOperand(1);

    // For use in switch cases
    string lLabel;
    Range lRange;
    Range rRange;

    // Assumption here is either left or right operand must be a constant
    bool reverseOperation = false;
    if (isa<Constant>(rightOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(rightOperand);
        int rConstant = CI->getSExtValue();
        rRange = widen_str(rConstant);

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Right side constant, false result
            reverseOperation = true;
        }
    }
    else if (isa<Constant>(leftOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(leftOperand);
        int rConstant = CI->getSExtValue();
        rRange = widen_str(rConstant);

        lLabel = get_load_store_label(*rightOperand);
        lRange = predAnalysis[lLabel];

        if (cmpResult)
        {
            // Left side constant, true result
            reverseOperation = true;
        }
    }
    else
    {
        string rLabel = get_load_store_label(*rightOperand);
        rRange = predAnalysis[rLabel];

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Test against left side
            reverseOperation = true;
        }
    }

    // NE not handled as it is complex condition for interval analysis
    // NE is equal to SGT + SLT
    switch (compareInst.getPredicate())
    {
    case CmpInst::ICMP_SGE:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_lt(lRange, rRange);
        }
        else
        {
            result[lLabel] = widen_ge(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SGT:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_le(lRange, rRange);
        }
        else
        {
            result[lLabel] = widen_gt(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SLE:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_gt(lRange, rRange);
        }
        else
        {
            result[lLabel] = widen_le(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SLT:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_ge(lRange, rRange);
        }
        else
        {
            result[lLabel] = widen_lt(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_EQ:
    {
        result[lLabel] = widen_eq(lRange, rRange);
        break;
    }
    default:
    {
        break;
    }
    }
    return result;
}

ValueAnalysis widen_pred_cond(ValueAnalysis predAnalysis, BasicBlock *predecessor, BasicBlock *current)
{
    ValueAnalysis result;
    for (auto rit = predecessor->rbegin(), ritend = predecessor->rend(); rit != ritend; ++rit)
    {
        Instruction &inst = *rit;

        if (isa<BranchInst>(inst))
        {
            result = widen_pred_cond_branch(predAnalysis, inst, current);
            break;
        }
    }
    return result;
}

map<string, Range> widen_get_operand_ranges(Instruction &I, ValueAnalysis temp)
{

    Value *op1 = I.getOperand(0);
    Value *op2 = I.getOperand(1);

    Range lRange, rRange;

    if (isa<ConstantInt>(op1))
    {
        lRange = widen_str(get_const_value(*op1));
    }
    else if (temp.find(label(op1)) != temp.end())
    {
        lRange = widen_str(temp[label(op1)]);
    }
    else
    {
        lRange = widen_all();
    }

    if (isa<ConstantInt>(op2))
    {
        rRange = widen_str(get_const_value(*op2));
    }
    else if (temp.find(label(op2)) != temp.end())
    {
        rRange = widen_str(temp[label(op2)]);
    }
    else
    {
        rRange = widen_all();
    }

    map<string, Range> result;

    result["left"] = lRange;
    result["right"] = rRange;

    return result;
}

ValueAnalysis widen_generate(BasicBlock *BB, ValueAnalysis predecessorAnalysis, ValueAnalysis previousRoundAnalysis)
{
    ValueAnalysis result = ValueAnalysis(predecessorAnalysis);
    ValueAnalysis temp = ValueAnalysis(predecessorAnalysis); // Storing % stuff

    for (auto &I : *BB)
    {
        if (isa<AllocaInst>(I))
        {
            if (previousRoundAnalysis.find(label(&I)) != previousRoundAnalysis.end())
            {

                result[label(&I)] = previousRoundAnalysis[label(&I)];
                temp[label(&I)] = previousRoundAnalysis[label(&I)];
            }
            else
            {
                result[label(&I)] = widen_all();
                temp[label(&I)] = widen_all();
            }
        }
        else if (isa<StoreInst>(I))
        {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);

            Range lRange;

            if (isa<ConstantInt>(op1))
            {
                lRange = widen_str(get_const_value(*op1));
            }
            else if (temp.find(label(op1)) != temp.end())
            {
                lRange = widen_str(temp[label(op1)]);
            }
            else
            {
                lRange = widen_all();
            }

            result[label(op2)] = lRange;
            temp[label(op2)] = lRange;
        }
        else if (isa<LoadInst>(I))
        {
            Value *op1 = I.getOperand(0);

            Range lRange;

            if (isa<ConstantInt>(op1))
            {
                lRange = widen_str(get_const_value(*op1));
            }
            else if (temp.find(label(op1)) != temp.end())
            {
                lRange = widen_str(temp[label(op1)]);
            }
            else
            {
                lRange = widen_all();
            }
            temp[label(&I)] = lRange;
        }
        else if (I.getOpcode() == BinaryOperator::SDiv)
        {
            map<string, Range> ops = widen_get_operand_ranges(I, temp);
            temp[label(&I)] = widen_div(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Mul)
        {
            map<string, Range> ops = widen_get_operand_ranges(I, temp);
            temp[label(&I)] = widen_mul(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Add)
        {
            map<string, Range> ops = widen_get_operand_ranges(I, temp);
            temp[label(&I)] = widen_add(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Sub)
        {
            map<string, Range> ops = widen_get_operand_ranges(I, temp);
            temp[label(&I)] = widen_sub(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::SRem)
        {
            map<string, Range> ops = widen_get_operand_ranges(I, temp);
            temp[label(&I)] = widen_rem(ops["left"], ops["right"]);
        }
    }

    return result;
}

void widen_generate(Function *F)
{
    for (auto &BB : *F)
    {
        if (wideImpossibleBlock[BB.getName()])
        {
            wideValueAnalysisMap.erase(BB.getName());
            continue;
        }
        ValueAnalysis predUnion;
        // Load the current stored analysis for all predecessor nodes
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            if (wideImpossibleBlock[predecessor->getName()])
            {
                continue;
            }
            ValueAnalysis predSet = widen_pred_cond(wideValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            predUnion = widen_combine(predUnion, predSet);
        }

        ValueAnalysis OldBBAnalysis = wideValueAnalysisMap[BB.getName()];
        ValueAnalysis BBAnalysis = widen_generate(&BB, predUnion, OldBBAnalysis);
        if (OldBBAnalysis != BBAnalysis)
        {
            wideValueAnalysisMap[BB.getName()] = BBAnalysis;
        }
    }
}

void widen_check_block(Function *F)
{
    for (auto &BB : *F)
    {
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            ValueAnalysis predSet = widen_pred_cond(wideValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            if (impossible_path(predSet))
            {
                wideImpossibleBlock[BB.getName()] = true;
            }
        }
    }
}

bool widen_fixed_point(map<string, ValueAnalysis> oldAnalysisMap)
{
    if (oldAnalysisMap.empty())
        return false;

    for (auto it = wideValueAnalysisMap.begin(); it != wideValueAnalysisMap.end(); ++it)
    {
        ValueAnalysis oldAnalysis = oldAnalysisMap[it->first];
        ValueAnalysis newAnalysis = it->second;
        if (oldAnalysis.size() != newAnalysis.size())
            return false;

        for (auto varRangeAnalysis = newAnalysis.begin(); varRangeAnalysis != newAnalysis.end(); ++varRangeAnalysis)
        {
            if (oldAnalysis.find(varRangeAnalysis->first) == oldAnalysis.end())
            {
                return false;
            }

            Range oldRange = oldAnalysis[varRangeAnalysis->first];
            Range newRange = varRangeAnalysis->second;

            if (oldRange != newRange)
            {
                return false;
            }
        }
    }
    return true;
}

void widen(Function *F)
{
    map<string, ValueAnalysis> oldAnalysisMap;

    // Fixpoint Loop
    while (!widen_fixed_point(oldAnalysisMap))
    {
        oldAnalysisMap.clear();
        oldAnalysisMap.insert(wideValueAnalysisMap.begin(), wideValueAnalysisMap.end());

        widen_check_block(F);
        widen_generate(F);

        if (print_steps)
        {
            outs() << "Round:" << widening_count++ << "\n";
            print(wideValueAnalysisMap);
            llvm::errs() << "\n";
        }
    }
}

#pragma endregion

#pragma region Narrow Flow

ValueAnalysis narrow_pred_cond_branch(ValueAnalysis predAnalysis, Instruction &inst, BasicBlock *current)
{
    ValueAnalysis result = ValueAnalysis(predAnalysis);

    BranchInst *br = dyn_cast<BranchInst>(&inst);

    int numOperands = br->getNumOperands();
    if (numOperands == 1)
    {
        // if end case
        return result;
    }

    // other cases will have 3 operands
    bool cmpResult = false;
    if (current == br->getOperand(2))
    {
        // if then
        cmpResult = true;
    }
    if (current == br->getOperand(1))
    {
        // if else
        cmpResult = false;
    }

    CmpInst &compareInst = cast<CmpInst>(*(br->getOperand(0)));
    Value *leftOperand = compareInst.getOperand(0);
    Value *rightOperand = compareInst.getOperand(1);

    // For use in switch cases
    string lLabel;
    Range lRange;
    Range rRange;

    // Assumption here is either left or right operand must be a constant
    bool reverseOperation = false;
    if (isa<Constant>(rightOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(rightOperand);
        int rConstant = CI->getSExtValue();
        rRange = narrow_str(rConstant);

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Right side constant, false result
            reverseOperation = true;
        }
    }
    else if (isa<Constant>(leftOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(leftOperand);
        int rConstant = CI->getSExtValue();
        rRange = narrow_str(rConstant);

        lLabel = get_load_store_label(*rightOperand);
        lRange = predAnalysis[lLabel];

        if (cmpResult)
        {
            // Left side constant, true result
            reverseOperation = true;
        }
    }
    else
    {
        string rLabel = get_load_store_label(*rightOperand);
        rRange = predAnalysis[rLabel];

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Test against left side
            reverseOperation = true;
        }
    }

    // NE not handled as it is complex condition for interval analysis
    // NE is equal to SGT + SLT
    switch (compareInst.getPredicate())
    {
    case CmpInst::ICMP_SGE:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_lt(lRange, rRange);
        }
        else
        {
            result[lLabel] = narrow_ge(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SGT:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_le(lRange, rRange);
        }
        else
        {
            result[lLabel] = narrow_gt(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SLE:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_gt(lRange, rRange);
        }
        else
        {
            result[lLabel] = narrow_le(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_SLT:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_ge(lRange, rRange);
        }
        else
        {
            result[lLabel] = narrow_lt(lRange, rRange);
        }
        break;
    }
    case CmpInst::ICMP_EQ:
    {
        result[lLabel] = narrow_eq(lRange, rRange);
        break;
    }
    default:
    {
        break;
    }
    }
    return result;
}

ValueAnalysis narrow_pred_cond(ValueAnalysis predAnalysis, BasicBlock *predecessor, BasicBlock *current)
{
    ValueAnalysis result;
    for (auto rit = predecessor->rbegin(), ritend = predecessor->rend(); rit != ritend; ++rit)
    {
        Instruction &inst = *rit;

        if (isa<BranchInst>(inst))
        {
            result = narrow_pred_cond_branch(predAnalysis, inst, current);
            break;
        }
    }
    return result;
}

map<string, Range> narrow_get_operand_ranges(Instruction &I, ValueAnalysis temp)
{
    Value *op1 = I.getOperand(0);
    Value *op2 = I.getOperand(1);

    Range lRange, rRange;

    if (isa<ConstantInt>(op1))
    {
        lRange = narrow_str(get_const_value(*op1));
    }
    else if (temp.find(label(op1)) != temp.end())
    {
        lRange = narrow_str(temp[label(op1)]);
    }
    else
    {
        lRange = narrow_all();
    }

    if (isa<ConstantInt>(op2))
    {
        rRange = narrow_str(get_const_value(*op2));
    }
    else if (temp.find(label(op2)) != temp.end())
    {
        rRange = narrow_str(temp[label(op2)]);
    }
    else
    {
        rRange = narrow_all();
    }

    map<string, Range> result;

    result["left"] = lRange;
    result["right"] = rRange;

    return result;
}

ValueAnalysis narrow_generate(BasicBlock *BB, ValueAnalysis predecessorAnalysis, ValueAnalysis previousRoundAnalysis)
{
    ValueAnalysis result = ValueAnalysis(predecessorAnalysis);
    ValueAnalysis temp = ValueAnalysis(predecessorAnalysis); // Storing % stuff

    for (auto &I : *BB)
    {
        if (isa<AllocaInst>(I))
        {
            if (previousRoundAnalysis.find(label(&I)) != previousRoundAnalysis.end())
            {
                result[label(&I)] = previousRoundAnalysis[label(&I)];
                temp[label(&I)] = previousRoundAnalysis[label(&I)];
            }
            else
            {
                result[label(&I)] = narrow_all();
                temp[label(&I)] = narrow_all();
            }
        }
        else if (isa<StoreInst>(I))
        {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);

            Range lRange;

            if (isa<ConstantInt>(op1))
            {
                lRange = narrow_str(get_const_value(*op1));
            }
            else if (temp.find(label(op1)) != temp.end())
            {
                lRange = narrow_str(temp[label(op1)]);
            }
            else
            {
                lRange = narrow_all();
            }

            result[label(op2)] = lRange;
            temp[label(op2)] = lRange;
        }
        else if (isa<LoadInst>(I))
        {
            Value *op1 = I.getOperand(0);

            Range lRange;

            if (isa<ConstantInt>(op1))
            {
                lRange = narrow_str(get_const_value(*op1));
            }
            else if (temp.find(label(op1)) != temp.end())
            {
                lRange = narrow_str(temp[label(op1)]);
            }
            else
            {
                lRange = narrow_all();
            }
            temp[label(&I)] = lRange;
        }
        else if (I.getOpcode() == BinaryOperator::SDiv)
        {
            map<string, Range> ops = narrow_get_operand_ranges(I, temp);
            temp[label(&I)] = narrow_div(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Mul)
        {
            map<string, Range> ops = narrow_get_operand_ranges(I, temp);
            temp[label(&I)] = narrow_mul(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Add)
        {
            map<string, Range> ops = narrow_get_operand_ranges(I, temp);
            temp[label(&I)] = narrow_add(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::Sub)
        {
            map<string, Range> ops = narrow_get_operand_ranges(I, temp);
            temp[label(&I)] = narrow_sub(ops["left"], ops["right"]);
        }
        else if (I.getOpcode() == BinaryOperator::SRem)
        {
            map<string, Range> ops = narrow_get_operand_ranges(I, temp);
            temp[label(&I)] = narrow_rem(ops["left"], ops["right"]);
        }
    }

    return result;
}

void narrow_generate(Function *F)
{
    for (auto &BB : *F)
    {
        if (narrowImpossibleBlock[BB.getName()])
        {
            narrowValueAnalysisMap.erase(BB.getName());
            continue;
        }
        ValueAnalysis predUnion;
        // Load the current stored analysis for all predecessor nodes
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            if (narrowImpossibleBlock[predecessor->getName()])
            {
                continue;
            }
            ValueAnalysis predSet = narrow_pred_cond(narrowValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            predUnion = narrow_combine(predUnion, predSet);
        }

        ValueAnalysis OldBBAnalysis = narrowValueAnalysisMap[BB.getName()];
        ValueAnalysis BBAnalysis = narrow_generate(&BB, predUnion, OldBBAnalysis);
        if (OldBBAnalysis != BBAnalysis)
        {
            narrowValueAnalysisMap[BB.getName()] = BBAnalysis;
        }
    }
}

void narrow_check_block(Function *F)
{
    for (auto &BB : *F)
    {
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            ValueAnalysis predSet = narrow_pred_cond(narrowValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            if (impossible_path(predSet))
            {
                narrowImpossibleBlock[BB.getName()] = true;
            }
        }
    }
}

bool narrow_fixed_point(map<string, ValueAnalysis> oldAnalysisMap)
{
    if (oldAnalysisMap.empty())
        return false;

    for (auto it = narrowValueAnalysisMap.begin(); it != narrowValueAnalysisMap.end(); ++it)
    {
        ValueAnalysis oldAnalysis = oldAnalysisMap[it->first];
        ValueAnalysis newAnalysis = it->second;
        if (oldAnalysis.size() != newAnalysis.size())
            return false;

        for (auto varRangeAnalysis = newAnalysis.begin(); varRangeAnalysis != newAnalysis.end(); ++varRangeAnalysis)
        {
            if (oldAnalysis.find(varRangeAnalysis->first) == oldAnalysis.end())
            {
                return false;
            }

            Range oldRange = oldAnalysis[varRangeAnalysis->first];
            Range newRange = varRangeAnalysis->second;

            if (oldRange != newRange)
            {
                return false;
            }
        }
    }
    return true;
}

void narrow(Function *F)
{
    map<string, ValueAnalysis> oldAnalysisMap;

    // Fixpoint Loop
    while (!narrow_fixed_point(oldAnalysisMap))
    {
        oldAnalysisMap.clear();
        oldAnalysisMap.insert(narrowValueAnalysisMap.begin(), narrowValueAnalysisMap.end());

        narrow_check_block(F);
        narrow_generate(F);

        if (print_steps)
        {
            outs() << "Round:" << narrowing_count++ << "\n";
            print(narrowValueAnalysisMap);
            llvm::errs() << "\n";
        }
    }
}

#pragma endregion

#pragma region Max Difference

void max_diff_analysis()
{
    for (auto it = narrowValueAnalysisMap.begin(); it != narrowValueAnalysisMap.end(); ++it)
    {
        string blockName = it->first;
        ValueAnalysis valueAnalysis = it->second;

        map<string, int> blockMap;
        for (auto itLeft = valueAnalysis.begin(); itLeft != valueAnalysis.end(); ++itLeft)
        {
            if (itLeft->first == "retval")
            {
                continue;
            }
            for (auto itRight = valueAnalysis.rbegin(); itRight->first != itLeft->first; ++itRight)
            {
                if (itRight->first == "retval")
                {
                    continue;
                }
                if (itLeft->first == itRight->first)
                {
                    continue;
                }
                string label = itLeft->first + " - " + itRight->first;
                if (itLeft->second.left == 1000 || itLeft->second.right == 1000 || itLeft->second.left == -1000 || itLeft->second.right == -1000)
                {
                    blockMap[label] = 1000;
                }
                else if (itRight->second.left == 1000 || itRight->second.right == 1000 || itRight->second.left == -1000 || itRight->second.right == -1000)
                {
                    blockMap[label] = 1000;
                }
                else
                {
                    blockMap[label] = itLeft->second.right - itRight->second.left;
                }
            }
        }
        diffAnalysisMap[blockName] = blockMap;
    }
}

#pragma endregion

int main(int argc, char **argv)
{
    // Read the IR file.
    static LLVMContext Context;
    SMDiagnostic Err;
    unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);

    if (M == nullptr)
    {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    Function *F = init(&M);

    print_steps = false;
    print_infinity = false;

    widen(F);
    narrowValueAnalysisMap = map<string, ValueAnalysis>(wideValueAnalysisMap);
    narrow(F);
    outs() << "\n====================================\n\n";
    max_diff_analysis();
    print(diffAnalysisMap);

    return 0;
}
