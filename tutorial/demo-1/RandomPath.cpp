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
        if (strncmp(F.getName().str().c_str(), "main", 4) == 0)
        {
            BasicBlock *BB = dyn_cast<BasicBlock>(F.begin());
            BB->print(llvm::errs(), true);
            generatePath(BB);
        }
    return 0;
}

void generatePath(BasicBlock *BB)
{
    const TerminatorInst *TInst = BB->getTerminator();
    unsigned NSucc = TInst->getNumSuccessors();
    if (NSucc == 1)
    {
        BasicBlock *Succ = TInst->getSuccessor(0);
        Succ->print(llvm::errs(), true);
        generatePath(Succ);
    }
    else if (NSucc > 1)
    {
        srand(time(NULL));
        unsigned rnd = std::rand() / (RAND_MAX / NSucc); // rand() return a number between 0 and RAND_MAX
        BasicBlock *Succ = TInst->getSuccessor(rnd);
        Succ->print(llvm::errs(), true);
        generatePath(Succ);
    }
}
