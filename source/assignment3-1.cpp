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
    }

    Range(Range const &r)
    {
        left = r.left;
        right = r.right;
        divisionByZero = r.divisionByZero;
        impossibleRange = r.impossibleRange;
    }

    bool operator==(Range r)
    {
        return (left == r.left) && (right == r.right);
    }

    bool operator!=(Range r)
    {
        return (left != r.left) || (right != r.right);
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
// Narrowing
map<string, ValueAnalysis> narrowValueAnalysisMap;

// Basic block to range for diff analysis, populated during narrowing
map<string, Range> diffAnalysisMap;

#pragma endregion

#pragma region Narrowing

// All implementations are simple approximations using range computation

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
    int newL = l.left + r.left;
    int newR = l.right + r.right;

    return Range(newL, newR);
}

Range narrow_sub(Range l, Range r)
{
    set<int> potentialValues;

    potentialValues.insert(l.left - r.left);
    potentialValues.insert(l.left - r.right);
    potentialValues.insert(l.right - r.left);
    potentialValues.insert(l.right - r.right);

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
    int newR = r.right;

    if ((l.left < 0 || r.left < 0) && (l.right >= 0 || r.right >= 0))
    {
        newL = -r.right;
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
Range narrow_gt(Range l, int r)
{
    if (r < l.left)
    {
        // r, l.left, l.right
        return Range(l);
    }
    if (r < l.right)
    {
        // l.left, r, l.right
        return Range(r + 1, l.right);
    }
    if (l.right == 1000 && r == 1000)
    {
        // Positive infinity range
        return Range(1000, 1000);
    }
    // l.left, l.right, r, impossible case
    return Range(0, 0, false, true);
}

Range narrow_ge(Range l, int r)
{
    if (r <= l.left)
    {
        // r, l.left, l.right
        return Range(l);
    }
    if (r <= l.right)
    {
        // l.left, r, l.right
        // Includes positive infinity range
        return Range(r, l.right);
    }
    // l.left, l.right, r, impossible case
    return Range(0, 0, false, true);
}

Range narrow_lt(Range l, int r)
{
    if (r > l.right)
    {
        // l.left, l.right, r
        return Range(l);
    }
    if (r > l.left)
    {
        // l.left, r, l.right
        return Range(l.left, r - 1);
    }
    if (l.left == -1000 && r == -1000)
    {
        // Negative infinity range
        return Range(-1000, -1000);
    }
    // r, l.left, l.right, impossible case
    return Range(0, 0, false, true);
}

Range narrow_le(Range l, int r)
{
    if (r >= l.right)
    {
        // l.left, l.right, r
        return Range(l);
    }
    if (r >= l.left)
    {
        // l.left, r, l.right
        // Includes negative infinity case
        return Range(l.left, r);
    }
    // r, l.left, l.right, impossible case
    return Range(0, 0, false, true);
}

Range narrow_eq(Range l, int r)
{
    if (l.left <= r && l.right >= r)
    {
        return Range(r);
    }
    else
    {
        return Range(0, 0, false, true);
    }
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

Range widen_gt(Range l, int r)
{
    return widen_str(narrow_gt(l, r));
}

Range widen_ge(Range l, int r)
{
    return widen_str(narrow_ge(l, r));
}

Range widen_lt(Range l, int r)
{
    return widen_str(narrow_lt(l, r));
}

Range widen_le(Range l, int r)
{
    return widen_str(narrow_le(l, r));
}

Range widen_eq(Range l, int r)
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
    return "[" + to_string(r.left) + ", " + to_string(r.right) + "] - Division By Zero: " + divByZero;
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
        outs() << "\t" << variable << " : " << label(variableRange) << "\n";
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

string get_load_store_label(Value &v)
{
    Instruction &I = cast<Instruction>(v);
    if (isa<StoreInst>(I))
    {
        return label(I.getOperand(1));
    }
    else
    {
        // Load inst
        Value *storeInstVal = I.getOperand(0);
        Instruction *storeInst = cast<Instruction>(storeInstVal);
        return label(storeInst->getOperand(1));
    }
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

void widen_flow(BasicBlock *BB, ValueAnalysis entrySet)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    unsigned originalCount = 0;
    bool traversed = false;

    string bblabel = label(BB);

    if (wideValueAnalysisMap.count(bblabel) == 0)
    {
        // Initialize
        ValueAnalysis empty;
        wideValueAnalysisMap[bblabel] = empty;
    }
    else
    {
        originalCount = wideValueAnalysisMap[bblabel].size();
        traversed = true;
    }

    ValueAnalysis generated;
    ValueAnalysis exitSet;
    // set<string> generated = generate(BB, entrySet);
    // set<string> exitSet = combine(analysisMap[bblabel], generated);

    wideValueAnalysisMap[bblabel] = exitSet;

    if (NSucc == 0)
    {
        return;
    }

    unsigned finalCount = exitSet.size();

    if (traversed && originalCount == finalCount)
    {
        return;
    }

    for (unsigned i = 0; i < NSucc; i++)
    {
        BasicBlock *Succ = TInst->getSuccessor(i);
        widen_flow(Succ, exitSet);
    }
}

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
    int rConstant;

    // Assumption here is either left or right operand must be a constant
    bool reverseOperation = false;
    if (isa<Constant>(rightOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(rightOperand);
        rConstant = CI->getSExtValue();

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Right side constant, false result
            reverseOperation = true;
        }
    }
    else
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(leftOperand);
        rConstant = CI->getSExtValue();

        lLabel = get_load_store_label(*rightOperand);
        lRange = predAnalysis[lLabel];

        if (cmpResult)
        {
            // Left side constant, true result
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
            result[lLabel] = widen_lt(lRange, rConstant);
        }
        else
        {
            result[lLabel] = widen_ge(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SGT:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_le(lRange, rConstant);
        }
        else
        {
            result[lLabel] = widen_gt(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SLE:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_gt(lRange, rConstant);
        }
        else
        {
            result[lLabel] = widen_le(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SLT:
    {
        if (reverseOperation)
        {
            result[lLabel] = widen_ge(lRange, rConstant);
        }
        else
        {
            result[lLabel] = widen_lt(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_EQ:
    {
        result[lLabel] = widen_eq(lRange, rConstant);
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

ValueAnalysis widen_generate(BasicBlock *BB, ValueAnalysis predecessorAnalysis)
{
    ValueAnalysis result;

    return result;
}

void widen_generate(Function *F)
{
    for (auto &BB : *F)
    {
        ValueAnalysis predUnion;
        // Load the current stored analysis for all predecessor nodes
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            ValueAnalysis predSet = widen_pred_cond(wideValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            predUnion = widen_combine(predUnion, predSet);
        }

        ValueAnalysis BBAnalysis = widen_generate(&BB, predUnion);
        ValueAnalysis OldBBAnalysis = wideValueAnalysisMap[BB.getName()];
        if (OldBBAnalysis != BBAnalysis)
        {
            wideValueAnalysisMap[BB.getName()] = widen_combine(BBAnalysis, OldBBAnalysis);
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
    int i = 0;
    while (!widen_fixed_point(oldAnalysisMap))
    {
        oldAnalysisMap.clear();
        oldAnalysisMap.insert(wideValueAnalysisMap.begin(), wideValueAnalysisMap.end());

        widen_generate(F);

        outs() << "Round:" << i++ << "\n";
        print(wideValueAnalysisMap);
        llvm::errs() << "\n";
    }
}

#pragma endregion

#pragma region Narrow Flow

void narrow_flow(BasicBlock *BB, ValueAnalysis entrySet)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    unsigned originalCount = 0;
    bool traversed = false;

    string bblabel = label(BB);

    if (narrowValueAnalysisMap.count(bblabel) == 0)
    {
        // Initialize
        ValueAnalysis empty;
        narrowValueAnalysisMap[bblabel] = empty;
    }
    else
    {
        originalCount = narrowValueAnalysisMap[bblabel].size();
        traversed = true;
    }

    ValueAnalysis generated;
    ValueAnalysis exitSet;
    // set<string> generated = generate(BB, entrySet);
    // set<string> exitSet = combine(analysisMap[bblabel], generated);

    narrowValueAnalysisMap[bblabel] = exitSet;

    if (NSucc == 0)
    {
        return;
    }

    unsigned finalCount = exitSet.size();

    if (traversed && originalCount == finalCount)
    {
        return;
    }

    for (unsigned i = 0; i < NSucc; i++)
    {
        BasicBlock *Succ = TInst->getSuccessor(i);
        narrow_flow(Succ, exitSet);
    }
}

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
    int rConstant;

    // Assumption here is either left or right operand must be a constant
    bool reverseOperation = false;
    if (isa<Constant>(rightOperand))
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(rightOperand);
        rConstant = CI->getSExtValue();

        lLabel = get_load_store_label(*leftOperand);
        lRange = predAnalysis[lLabel];

        if (!cmpResult)
        {
            // Right side constant, false result
            reverseOperation = true;
        }
    }
    else
    {
        ConstantInt *CI = dyn_cast<ConstantInt>(leftOperand);
        rConstant = CI->getSExtValue();

        lLabel = get_load_store_label(*rightOperand);
        lRange = predAnalysis[lLabel];

        if (cmpResult)
        {
            // Left side constant, true result
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
            result[lLabel] = narrow_lt(lRange, rConstant);
        }
        else
        {
            result[lLabel] = narrow_ge(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SGT:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_le(lRange, rConstant);
        }
        else
        {
            result[lLabel] = narrow_gt(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SLE:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_gt(lRange, rConstant);
        }
        else
        {
            result[lLabel] = narrow_le(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_SLT:
    {
        if (reverseOperation)
        {
            result[lLabel] = narrow_ge(lRange, rConstant);
        }
        else
        {
            result[lLabel] = narrow_lt(lRange, rConstant);
        }
        break;
    }
    case CmpInst::ICMP_EQ:
    {
        result[lLabel] = narrow_eq(lRange, rConstant);
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

ValueAnalysis narrow_generate(BasicBlock *BB, ValueAnalysis predecessorAnalysis)
{
    ValueAnalysis result;

    return result;
}

void narrow_generate(Function *F)
{
    for (auto &BB : *F)
    {
        ValueAnalysis predUnion;
        // Load the current stored analysis for all predecessor nodes
        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
        {
            BasicBlock *predecessor = *it;
            ValueAnalysis predSet = narrow_pred_cond(narrowValueAnalysisMap[predecessor->getName()], predecessor, &BB);
            predUnion = narrow_combine(predUnion, predSet);
        }

        ValueAnalysis BBAnalysis = narrow_generate(&BB, predUnion);
        ValueAnalysis OldBBAnalysis = narrowValueAnalysisMap[BB.getName()];
        if (OldBBAnalysis != BBAnalysis)
        {
            narrowValueAnalysisMap[BB.getName()] = narrow_combine(BBAnalysis, OldBBAnalysis);
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
    int i = 0;
    while (!narrow_fixed_point(oldAnalysisMap))
    {
        oldAnalysisMap.clear();
        oldAnalysisMap.insert(narrowValueAnalysisMap.begin(), narrowValueAnalysisMap.end());

        narrow_generate(F);

        outs() << "Round:" << i++ << "\n";
        print(narrowValueAnalysisMap);
        llvm::errs() << "\n";
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

    widen(F);
    // print(wideValueAnalysisMap);

    narrow(F);
    // print(narrowValueAnalysisMap);

    return 0;
}
