#include <cstdio>
#include <iostream>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;

void generatePath(BasicBlock *BB);
void traverse(BasicBlock *BB);
void print(const BasicBlock *bb);
std::string getSimpleNodeLabel(const BasicBlock *Node);

int main(int argc, char **argv)
{
    // Read the IR file.
    static LLVMContext Context;
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
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

    return 0;
}

void print(const BasicBlock *bb)
{
    llvm::outs() << "Label:" << getSimpleNodeLabel(bb) << "\n";
    // bb->print(llvm::errs(), false);
}


// Printing Basic Block Label
std::string getSimpleNodeLabel(const BasicBlock *Node)
{
    if (!Node->getName().empty())
        return Node->getName().str();
    std::string Str;
    raw_string_ostream OS(Str);
    Node->printAsOperand(OS, false);
    return OS.str();
}

void traverse(BasicBlock *BB)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();

    for (unsigned i = 0; i < NSucc; i++)
    {
        BasicBlock *Succ = TInst->getSuccessor(i);
        print(Succ);
        traverse(Succ);
    }
}

void generatePath(BasicBlock *BB)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();
    if (NSucc == 1)
    {
        BasicBlock *Succ = TInst->getSuccessor(0);
        print(Succ);
        generatePath(Succ);
    }
    else if (NSucc > 1)
    {
        srand(time(NULL));
        unsigned rnd = std::rand() / (RAND_MAX / NSucc); // rand() return a number between 0 and RAND_MAX
        BasicBlock *Succ = TInst->getSuccessor(rnd);
        print(Succ);
        generatePath(Succ);
    }
}
