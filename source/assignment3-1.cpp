#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>

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

// typedef std::map<Value *, std::set<int>> BBANALYSIS;

// void flow(BasicBlock *BB, set<string> entrySet);

// set<string> generate(BasicBlock *bb, set<string> previous);
// set<string> combine(set<string> entry, set<string> generate, set<string> previous);

// void print(const Value *bb);
// void print(const Value *bb, string label);

// void print(set<string> stringSet);
// void print(const map<string, set<string>> analysisMap);

// string label(const Value *Node);

// BasicBlock *findMain(unique_ptr<Module> *m);

// BasicBlock *findMain(unique_ptr<Module> *m)
// {
//     for (auto &F : **m)
//     {
//         if (strncmp(F.getName().str().c_str(), "main", 4) == 0)
//         {
//             BasicBlock *BB = dyn_cast<BasicBlock>(F.begin());
//             return BB;
//         }
//     }
//     return NULL;
// }

// set<string> generate(BasicBlock *bb, set<string> entry)
// {
//     set<string> generate;

//     set<string> sinks;

//     generate.insert("source");
//     generate.insert(entry.begin(), entry.end());

//     sinks.insert("source");
//     sinks.insert(entry.begin(), entry.end());

//     for (auto &I : *bb)
//     {
//         if (isa<StoreInst>(I))
//         {
//             Value *sink = I.getOperand(1);
//             Value *source = I.getOperand(0);

//             if (sinks.count(label(source)) > 0)
//             {
//                 generate.insert(label(sink));
//                 sinks.insert(label(sink));
//             }
//             else
//             {
//                 generate.erase(label(sink));
//                 sinks.erase(label(sink));
//             }
//         }
//         else if (isa<LoadInst>(I))
//         {
//             Value *source = I.getOperand(0);
//             if (sinks.count(label(source)) > 0)
//             {
//                 generate.insert(label(&I));
//                 sinks.insert(label(&I));
//             }
//             else
//             {
//                 generate.erase(label(&I));
//                 sinks.erase(label(&I));
//             }
//         }
//         else if (isa<BinaryOperator>(I))
//         {
//             Value *op1 = I.getOperand(0);
//             Value *op2 = I.getOperand(1);

//             if ((!isa<Constant>(op1)) && sinks.count(label(op1)) > 0)
//             {
//                 generate.insert(label(&I));
//                 sinks.insert(label(&I));
//             }
//             else if ((!isa<Constant>(op2)) && sinks.count(label(op2)) > 0)
//             {
//                 generate.insert(label(&I));
//                 sinks.insert(label(&I));
//             }
//             else
//             {
//                 generate.erase(label(&I));
//                 sinks.erase(label(&I));
//             }
//         }
//     }

//     return generate;
// }

// set<string> combine(set<string> entry, set<string> generate)
// {
//     set<string> combined;

//     combined.insert(entry.begin(), entry.end());
//     combined.insert(generate.begin(), generate.end());

//     return combined;
// }

// void flow(BasicBlock *BB, set<string> entrySet)
// {
//     const TerminatorInst *TInst = BB->getTerminator();
//     unsigned NSucc = TInst->getNumSuccessors();

//     unsigned originalCount = 0;
//     bool traversed = false;

//     string bblabel = label(BB);

//     if (analysisMap.count(bblabel) == 0)
//     {
//         // Initialize
//         set<string> empty;
//         analysisMap[bblabel] = empty;
//     }
//     else
//     {
//         originalCount = analysisMap[bblabel].size();
//         traversed = true;
//     }

//     set<string> generated = generate(BB, entrySet);
//     set<string> exitSet = combine(analysisMap[bblabel], generated);

//     analysisMap[bblabel] = exitSet;

//     if (NSucc == 0)
//     {
//         return;
//     }

//     unsigned finalCount = exitSet.size();

//     if (traversed && originalCount == finalCount)
//     {
//         return;
//     }

//     for (unsigned i = 0; i < NSucc; i++)
//     {
//         BasicBlock *Succ = TInst->getSuccessor(i);
//         flow(Succ, exitSet);
//     }
// }

// #pragma region Instruction Analysis

// // Processing Alloca Instruction
// std::set<int> processAlloca()
// {
//     std::set<int> set;
//     set.insert(-1000); // Represents negative infinity
//     set.insert(-100);
//     set.insert(-10);
//     set.insert(-1);
//     set.insert(0);
//     set.insert(1);
//     set.insert(10);
//     set.insert(100);
//     set.insert(1000); // Represents infinity
//     return set;
// }

// // Processing Store Instruction
// std::set<int> processStore(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     Value *op2 = I->getOperand(1);
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         std::set<int> set;
//         if (op1Int % 2 == 1)
//             set.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set.insert(EVEN);
//         return set;
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         return analysis[op1];
//     }
//     else
//     {
//         std::set<int> set;
//         set.insert(ODD);
//         set.insert(EVEN);
//         return set;
//     }
// }

// // Processing Load Instruction
// std::set<int> processLoad(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         std::set<int> set;
//         if (op1Int % 2 == 1)
//             set.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set.insert(EVEN);
//         return set;
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         return analysis[op1];
//     }
//     else
//     {
//         std::set<int> set;
//         set.insert(ODD);
//         set.insert(EVEN);
//         return set;
//     }
// }

// // Processing Mul Instructions
// std::set<int> processMul(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     Value *op2 = I->getOperand(1);
//     std::set<int> set1, set2;
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         if (op1Int % 2 == 1)
//             set1.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set1.insert(EVEN);
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         set1 = analysis[op1];
//     }
//     else
//     {
//         set1.insert(ODD);
//         set1.insert(EVEN);
//     }

//     if (isa<ConstantInt>(op2))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
//         int64_t op2Int = CI->getSExtValue();
//         if (op2Int % 2 == 1)
//             set2.insert(ODD);
//         else if (op2Int % 2 == 0)
//             set2.insert(EVEN);
//     }
//     else if (analysis.find(op2) != analysis.end())
//     {
//         set2 = analysis[op2];
//     }
//     else
//     {
//         set2.insert(ODD);
//         set2.insert(EVEN);
//     }

//     if (set1.size() == 1 && set1.find(ODD) != set1.end() &&
//         set2.size() == 1 && set2.find(ODD) != set1.end())
//         return set1;

//     if (set1.find(ODD) != set1.end() &&
//         set2.find(ODD) != set1.end())
//         return union_sets(set1, set2);

//     std::set<int> set;
//     set.insert(EVEN);
//     return set;
// }

// // Processing Div Instructions
// std::set<int> processDiv(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     Value *op2 = I->getOperand(1);
//     std::set<int> set1, set2;
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         if (op1Int % 2 == 1)
//             set1.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set1.insert(EVEN);
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         set1 = analysis[op1];
//     }
//     else
//     {
//         set1.insert(ODD);
//         set1.insert(EVEN);
//     }

//     if (isa<ConstantInt>(op2))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
//         int64_t op2Int = CI->getSExtValue();
//         if (op2Int % 2 == 1)
//             set2.insert(ODD);
//         else if (op2Int % 2 == 0)
//             set2.insert(EVEN);
//     }
//     else if (analysis.find(op2) != analysis.end())
//     {
//         set2 = analysis[op2];
//     }
//     else
//     {
//         set2.insert(ODD);
//         set2.insert(EVEN);
//     }

//     if (set1.size() == 1 && set1.find(ODD) != set1.end() &&
//         set2.size() == 1 && set2.find(ODD) != set1.end())
//         return set1;

//     std::set<int> set;
//     set.insert(ODD);
//     set.insert(EVEN);
//     return set;
// }

// // Processing Add & Sub Instructions
// std::set<int> processAddSub(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     Value *op2 = I->getOperand(1);
//     std::set<int> set1, set2;
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         if (op1Int % 2 == 1)
//             set1.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set1.insert(EVEN);
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         set1 = analysis[op1];
//     }
//     else
//     {
//         set1.insert(ODD);
//         set1.insert(EVEN);
//     }

//     if (isa<ConstantInt>(op2))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
//         int64_t op2Int = CI->getSExtValue();
//         if (op2Int % 2 == 1)
//             set2.insert(ODD);
//         else if (op2Int % 2 == 0)
//             set2.insert(EVEN);
//     }
//     else if (analysis.find(op2) != analysis.end())
//     {
//         set2 = analysis[op2];
//     }
//     else
//     {
//         set2.insert(ODD);
//         set2.insert(EVEN);
//     }

//     std::set<int> EvenSet;
//     EvenSet.insert(EVEN);

//     std::set<int> OddSet;
//     OddSet.insert(ODD);

//     if ((set1.size() == 1 && set1.find(ODD) != set1.end() &&
//          set2.size() == 1 && set2.find(ODD) != set1.end()) ||
//         (set1.size() == 1 && set1.find(EVEN) != set1.end() &&
//          set2.size() == 1 && set2.find(EVEN) != set1.end()))
//         return EvenSet;

//     if ((set1.size() == 1 && set1.find(ODD) != set1.end() &&
//          set2.size() == 1 && set2.find(EVEN) != set1.end()) ||
//         (set1.size() == 1 && set1.find(EVEN) != set1.end() &&
//          set2.size() == 1 && set2.find(ODD) != set1.end()))
//         return OddSet;

//     return union_sets(set1, set2);
// }

// // Processing Rem Instructions
// std::set<int> processRem(llvm::Instruction *I, BBANALYSIS analysis)
// {
//     Value *op1 = I->getOperand(0);
//     Value *op2 = I->getOperand(1);

//     if (isa<ConstantInt>(op2))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
//         int64_t op2Int = CI->getSExtValue();
//         if (op2Int != 2)
//         {
//             std::set<int> set;
//             set.insert(ODD);
//             set.insert(EVEN);
//             return set;
//         }
//     }

//     std::set<int> set1, set2;
//     if (isa<ConstantInt>(op1))
//     {
//         llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
//         int64_t op1Int = CI->getSExtValue();
//         if (op1Int % 2 == 1)
//             set1.insert(ODD);
//         else if (op1Int % 2 == 0)
//             set1.insert(EVEN);
//     }
//     else if (analysis.find(op1) != analysis.end())
//     {
//         set1 = analysis[op1];
//     }
//     else
//     {
//         set1.insert(ODD);
//         set1.insert(EVEN);
//     }

//     if (set1.size() == 1 && set1.find(ODD) != set1.end())
//         return set1;

//     if (set1.size() == 1 && set1.find(EVEN) != set1.end())
//         return set1;

//     return set1;
// }

// #pragma endregion

// #pragma region Block Analysis

// BBANALYSIS processBlock(BasicBlock *BB)
// {
// }

// #pragma endregion

struct Range
{
    int left;
    int right;

    bool divisionByZero;

    Range()
    {
        left = 0;
        right = 0;
        divisionByZero = false;
    }

    Range(int v)
    {
        left = v;
        right = v;
        divisionByZero = false;
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
    }

    Range(Range const &r)
    {
        left = r.left;
        right = r.right;
        divisionByZero = r.divisionByZero;
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

// Variable / register name to range
typedef std::map<string, Range> ValueAnalysis;

// Basic block to value analysis
// Widening
map<string, ValueAnalysis> wideValueAnalysisMap;
// Narrowing
map<string, ValueAnalysis> narrowValueAnalysisMap;

// Basic block to range for diff analysis, populated during narrowing
map<string, Range> diffAnalysisMap;

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

    set<int> potentialValues;

    if (r.left == 0)
    {
        // Take the values by itself as if rem by 1
        potentialValues.insert(1);
    }
    else
    {
        potentialValues.insert(l.left % r.left);
        potentialValues.insert(l.right % r.left);
    }

    if (r.right == 0)
    {
        // Take the values by itself as if rem by -1
        potentialValues.insert(-1);
    }
    else
    {
        potentialValues.insert(l.left % r.right);
        potentialValues.insert(l.right % r.right);
    }

    int newL = *potentialValues.begin();
    int newR = *potentialValues.rbegin();

    return Range(newL, newR, divisionByZero);
}

#pragma endregion

#pragma region Widening

int widen(int v)
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

    return v;
}

Range widen_all()
{
    return Range(-1000, 1000);
}

Range widen_str(int v)
{
    return Range(widen(v));
}

Range widen_str(Range r)
{
    int newL = widen(r.left);
    int newR = widen(r.right);

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

void init(unique_ptr<Module> *m)
{
    for (auto &F : **m)
    {
        ValueAnalysis emptyWide, emptyNarrow;
        BasicBlock *BB = dyn_cast<BasicBlock>(F.begin());
        wideValueAnalysisMap[label(BB)] = emptyWide;
        narrowValueAnalysisMap[label(BB)] = emptyNarrow;
    }
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

    BasicBlock *main = findMain(&M);
    if (main == nullptr)
    {
        fprintf(stderr, "error: main not found in LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    ValueAnalysis emptyAnalysis;
    widen_flow(main, emptyAnalysis);
    narrow_flow(main, emptyAnalysis);
    print(wideValueAnalysisMap);
    print(narrowValueAnalysisMap);

    return 0;
}
