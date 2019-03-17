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

void flow(BasicBlock *BB, set<string> entrySet);

set<string> generate(BasicBlock *bb, set<string> previous);
set<string> combine(set<string> entry, set<string> generate, set<string> previous);

void print(const Value *bb);
void print(const Value *bb, string label);

void print(set<string> stringSet);
void print(const map<string, set<string>> analysisMap);

string label(const Value *Node);

BasicBlock *findMain(unique_ptr<Module> *m);

map<string, set<string>> analysisMap;

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

    set<string> emptySet;
    flow(main, emptySet);

    print(analysisMap);

    return 0;
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

set<string> generate(BasicBlock *bb, set<string> entry)
{
    set<string> generate;

    set<string> sinks;

    generate.insert("source");
    generate.insert(entry.begin(), entry.end());

    sinks.insert("source");
    sinks.insert(entry.begin(), entry.end());

    for (auto &I : *bb)
    {
        if (isa<StoreInst>(I))
        {
            Value *sink = I.getOperand(1);
            Value *source = I.getOperand(0);

            if (sinks.count(label(source)) > 0)
            {
                generate.insert(label(sink));
                sinks.insert(label(sink));
            }
        }
        else if (isa<LoadInst>(I))
        {
            Value *source = I.getOperand(0);
            if (sinks.count(label(source)) > 0)
            {
                sinks.insert(label(&I));
            }
        }
        else if (isa<BinaryOperator>(I))
        {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);

            if ((!isa<Constant>(op1)) && sinks.count(label(op1)) > 0)
            {
                sinks.insert(label(&I));
            }
            else if ((!isa<Constant>(op2)) && sinks.count(label(op2)) > 0)
            {
                sinks.insert(label(&I));
            }
        }
    }

    print(sinks);

    return generate;
}

set<string> combine(set<string> entry, set<string> generate)
{
    set<string> combined;

    combined.insert(entry.begin(), entry.end());
    combined.insert(generate.begin(), generate.end());

    return combined;
}

void flow(BasicBlock *BB, set<string> entrySet)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    unsigned originalCount = 0;
    bool traversed = false;

    string bblabel = label(BB);

    if (analysisMap.count(bblabel) == 0)
    {
        // Initialize
        set<string> empty;
        analysisMap[bblabel] = empty;
    }
    else
    {
        originalCount = analysisMap[bblabel].size();
        traversed = true;
    }

    set<string> generated = generate(BB, entrySet);
    set<string> exitSet = combine(analysisMap[bblabel], generated);

    analysisMap[bblabel] = exitSet;

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
        flow(Succ, exitSet);
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

string label(const Value *Node)
{
    if (!Node->getName().empty())
        return Node->getName().str();

    string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}
