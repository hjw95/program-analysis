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
void print(const Value *bb);
void print(const Instruction *bb);
void print(const BasicBlock *bb);
void print(const map<string, set<Instruction *>> analysisMap);

string getSimpleNodeLabel(const BasicBlock *Node);
string getSimpleNodeLabel(const Instruction *Node);

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

void traverse(BasicBlock *BB, set<Instruction *> entrySet)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    set<Instruction *> exitSet;

    unsigned originalCount = 0;
    bool traversed = false;

    // Union of entry set
    exitSet.insert(entrySet.begin(), entrySet.end());

    if (analysisMap.count(getSimpleNodeLabel(BB)) > 0)
    {
        // Union of previous iteration
        exitSet.insert(analysisMap[getSimpleNodeLabel(BB)].begin(), analysisMap[getSimpleNodeLabel(BB)].end());
        originalCount = analysisMap[getSimpleNodeLabel(BB)].size();
        traversed = true;
    }

    for (auto &I : *BB)
    {
        if (isa<StoreInst>(I))
        {
            Value *v = I.getOperand(1);
            Instruction *var = dyn_cast<Instruction>(v);
            exitSet.insert(var);
        }
    }

    analysisMap[getSimpleNodeLabel(BB)] = exitSet;

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

        outs() << BBLabel << ":\n";
        for (Instruction *var : initializedVars)
        {
            outs() << "\t";
            print(var);
        }
    }
}

void print(const BasicBlock *bb)
{
    outs() << "Label:" << getSimpleNodeLabel(bb) << "\n";
    // bb->print(llvm::errs(), false);
}

void print(const Instruction *bb)
{
    outs() << "Instruction : ";
    // outs() << "Label:" << getSimpleNodeLabel(bb) << "\n";
    bb->print(llvm::errs(), false);
    outs() << "\n";
}

void print(const Value *bb)
{
    outs() << "Value : ";
    // outs() << "Label:" << getSimpleNodeLabel(bb) << "\n";
    bb->print(llvm::errs(), false);
    outs() << "\n";
}

string getSimpleNodeLabel(const BasicBlock *Node)
{
    if (!Node->getName().empty())
        return Node->getName().str();

    string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}

string getSimpleNodeLabel(const Instruction *Node)
{
    if (!Node->getName().empty())
        return Node->getName().str();

    string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}
