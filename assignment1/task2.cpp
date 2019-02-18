#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>

#include "llvm/IR/DebugInfoMetadata.h"
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

void traverse(BasicBlock *BB);
void print(const Value *bb);
void print(const Instruction *bb);
void print(const BasicBlock *bb);
void print(const map<string, set<Instruction *>> analysisMap);

string getSimpleNodeLabel(const BasicBlock *Node);
string getSimpleNodeLabel(const Instruction *Node);

map<string, set<Instruction *>> analysisMap;

map<string, set<string>> initializedVariables;

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
            print(BB);
            traverse(BB);
        }
    }

    outs() << "Final Analysis: \n";
    print(analysisMap);

    return 0;
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

void traverse(BasicBlock *BB)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    set<Instruction *> affectingInstruction;
    set<string> initializedVariables;

    unsigned originalCount = 0;
    bool traversed = false;

    if (analysisMap.count(getSimpleNodeLabel(BB)) > 0)
    {
        // found
        affectingInstruction = analysisMap[getSimpleNodeLabel(BB)];
        originalCount = affectingInstruction.size();
        traversed = true;
    }

    for (auto &I : *BB)
    {
        if (isa<LoadInst>(I))
        {
            Value *v = I.getOperand(0);
            Instruction *var = dyn_cast<Instruction>(v);
            affectingInstruction.insert(var);
        }
        else if (isa<StoreInst>(I))
        {
            Value *v = I.getOperand(1);
            Instruction *var = dyn_cast<Instruction>(v);
            affectingInstruction.insert(var);
        }
        else
        {
            outs() << "Skipped ";
            print(&I);
        }
    }

    analysisMap[getSimpleNodeLabel(BB)] = affectingInstruction;

    unsigned finalCount = affectingInstruction.size();

    if (traversed && originalCount == finalCount)
    {
        return;
    }

    for (unsigned i = 0; i < NSucc; i++)
    {
        BasicBlock *Succ = TInst->getSuccessor(i);
        print(Succ);
        traverse(Succ);
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
