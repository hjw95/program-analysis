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

void traverse(BasicBlock *BB, set<Instruction *> entrySet);

set<Instruction *> generate(BasicBlock *bb);
set<Instruction *> combine(set<Instruction *> entry, set<Instruction *> generate, set<Instruction *> previous);

void print(const Value *bb);
void print(const map<string, set<Instruction *>> analysisMap);

string label(const Value *Node);

map<string, set<Instruction *>> analysisMap;

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

    for (auto &F : *M)
    {
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0)
        {
            BasicBlock *BB = dyn_cast<BasicBlock>(F.begin());
            set<Instruction *> emptySet;
            traverse(BB, emptySet);
        }
    }

    print(analysisMap);

    return 0;
}

set<Instruction *> generate(BasicBlock *bb)
{
    set<Instruction *> generate;

    for (auto &I : *bb)
    {
        if (isa<StoreInst>(I))
        {
            Value *v = I.getOperand(1);
            Instruction *var = dyn_cast<Instruction>(v);
            if (label(var).compare("retval") != 0)
            {
                generate.insert(var);
            }
        }
    }

    return generate;
}

set<Instruction *> combine(set<Instruction *> entry, set<Instruction *> generate, set<Instruction *> previous)
{
    set<Instruction *> combined;

    combined.insert(entry.begin(), entry.end());
    combined.insert(generate.begin(), generate.end());
    combined.insert(previous.begin(), previous.end());

    return combined;
}

void traverse(BasicBlock *BB, set<Instruction *> entrySet)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    unsigned originalCount = 0;
    bool traversed = false;

    string bblabel = label(BB);

    if (analysisMap.count(bblabel) == 0)
    {
        // Initialize
        set<Instruction *> empty;
        analysisMap[bblabel] = empty;
    }
    else
    {
        originalCount = analysisMap[bblabel].size();
        traversed = true;
    }

    set<Instruction *> generated = generate(BB);
    set<Instruction *> exitSet = combine(entrySet, generated, analysisMap[bblabel]);

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
        traverse(Succ, exitSet);
    }
}

void print(const map<string, set<Instruction *>> analysisMap)
{
    for (auto &row : analysisMap)
    {
        set<Instruction *> initializedVars = row.second;
        string BBLabel = row.first;

        outs() << BBLabel << "\t: {";
        bool first = true;
        for (Instruction *var : initializedVars)
        {
            if (!first)
            {
                outs() << ", ";
            }
            outs() << label(var);
            first = false;
        }
        outs() << "} \n";
    }
}

void print(const Value *bb)
{
    outs() << "Label:" << label(bb) << "\n";
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
