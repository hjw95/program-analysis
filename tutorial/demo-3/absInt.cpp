#include <cstdio>
#include <iostream>
#include <set>
#include <map>
#include <stack>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"

using namespace llvm;
#define ODD 1
#define EVEN 2
typedef std::map<Value *, std::set<int>> BBANALYSIS;

std::map<std::string, BBANALYSIS> analysisMap;

//======================================================================
// Check fixpoint reached
//======================================================================
bool fixPointReached(std::map<std::string, BBANALYSIS> oldAnalysisMap)
{
	if (oldAnalysisMap.empty())
		return false;
	for (auto it = analysisMap.begin(); it != analysisMap.end(); ++it)
	{
		if (oldAnalysisMap[it->first] != it->second)
			return false;
	}
	return true;
}

// Performs set union
std::set<int> union_sets(std::set<int> A, std::set<int> B)
{
	A.insert(B.cbegin(), B.cend());
	return A;
}

// Performs analysis union
BBANALYSIS union_analysis(BBANALYSIS A, BBANALYSIS B)
{
	for (auto it = A.begin(); it != A.end(); ++it)
	{
		A[it->first] = union_sets(it->second, B[it->first]);
	}

	for (auto it = B.begin(); it != B.end(); ++it)
	{
		A[it->first] = union_sets(it->second, A[it->first]);
	}

	return A;
}

//======================================================================
// update Basic Block Analysis
//======================================================================

// Processing Alloca Instruction
std::set<int> processAlloca()
{
	std::set<int> set;
	set.insert(ODD);
	set.insert(EVEN);
	return set;
}

// Processing Store Instruction
std::set<int> processStore(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		std::set<int> set;
		if (op1Int % 2 == 1)
			set.insert(ODD);
		else if (op1Int % 2 == 0)
			set.insert(EVEN);
		return set;
	}
	else if (analysis.find(op1) != analysis.end())
	{
		return analysis[op1];
	}
	else
	{
		std::set<int> set;
		set.insert(ODD);
		set.insert(EVEN);
		return set;
	}
}

// Processing Load Instruction
std::set<int> processLoad(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		std::set<int> set;
		if (op1Int % 2 == 1)
			set.insert(ODD);
		else if (op1Int % 2 == 0)
			set.insert(EVEN);
		return set;
	}
	else if (analysis.find(op1) != analysis.end())
	{
		return analysis[op1];
	}
	else
	{
		std::set<int> set;
		set.insert(ODD);
		set.insert(EVEN);
		return set;
	}
}

// Processing Mul Instructions
std::set<int> processMul(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	std::set<int> set1, set2;
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		if (op1Int % 2 == 1)
			set1.insert(ODD);
		else if (op1Int % 2 == 0)
			set1.insert(EVEN);
	}
	else if (analysis.find(op1) != analysis.end())
	{
		set1 = analysis[op1];
	}
	else
	{
		set1.insert(ODD);
		set1.insert(EVEN);
	}

	if (isa<ConstantInt>(op2))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
		int64_t op2Int = CI->getSExtValue();
		if (op2Int % 2 == 1)
			set2.insert(ODD);
		else if (op2Int % 2 == 0)
			set2.insert(EVEN);
	}
	else if (analysis.find(op2) != analysis.end())
	{
		set2 = analysis[op2];
	}
	else
	{
		set2.insert(ODD);
		set2.insert(EVEN);
	}

	if (set1.size() == 1 && set1.find(ODD) != set1.end() &&
			set2.size() == 1 && set2.find(ODD) != set1.end())
		return set1;

	if (set1.find(ODD) != set1.end() &&
			set2.find(ODD) != set1.end())
		return union_sets(set1, set2);

	std::set<int> set;
	set.insert(EVEN);
	return set;
}

// Processing Div Instructions
std::set<int> processDiv(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	std::set<int> set1, set2;
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		if (op1Int % 2 == 1)
			set1.insert(ODD);
		else if (op1Int % 2 == 0)
			set1.insert(EVEN);
	}
	else if (analysis.find(op1) != analysis.end())
	{
		set1 = analysis[op1];
	}
	else
	{
		set1.insert(ODD);
		set1.insert(EVEN);
	}

	if (isa<ConstantInt>(op2))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
		int64_t op2Int = CI->getSExtValue();
		if (op2Int % 2 == 1)
			set2.insert(ODD);
		else if (op2Int % 2 == 0)
			set2.insert(EVEN);
	}
	else if (analysis.find(op2) != analysis.end())
	{
		set2 = analysis[op2];
	}
	else
	{
		set2.insert(ODD);
		set2.insert(EVEN);
	}

	if (set1.size() == 1 && set1.find(ODD) != set1.end() &&
			set2.size() == 1 && set2.find(ODD) != set1.end())
		return set1;

	std::set<int> set;
	set.insert(ODD);
	set.insert(EVEN);
	return set;
}

// Processing Add & Sub Instructions
std::set<int> processAddSub(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);
	std::set<int> set1, set2;
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		if (op1Int % 2 == 1)
			set1.insert(ODD);
		else if (op1Int % 2 == 0)
			set1.insert(EVEN);
	}
	else if (analysis.find(op1) != analysis.end())
	{
		set1 = analysis[op1];
	}
	else
	{
		set1.insert(ODD);
		set1.insert(EVEN);
	}

	if (isa<ConstantInt>(op2))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
		int64_t op2Int = CI->getSExtValue();
		if (op2Int % 2 == 1)
			set2.insert(ODD);
		else if (op2Int % 2 == 0)
			set2.insert(EVEN);
	}
	else if (analysis.find(op2) != analysis.end())
	{
		set2 = analysis[op2];
	}
	else
	{
		set2.insert(ODD);
		set2.insert(EVEN);
	}

	std::set<int> EvenSet;
	EvenSet.insert(EVEN);

	std::set<int> OddSet;
	OddSet.insert(ODD);

	if ((set1.size() == 1 && set1.find(ODD) != set1.end() &&
			 set2.size() == 1 && set2.find(ODD) != set1.end()) ||
			(set1.size() == 1 && set1.find(EVEN) != set1.end() &&
			 set2.size() == 1 && set2.find(EVEN) != set1.end()))
		return EvenSet;

	if ((set1.size() == 1 && set1.find(ODD) != set1.end() &&
			 set2.size() == 1 && set2.find(EVEN) != set1.end()) ||
			(set1.size() == 1 && set1.find(EVEN) != set1.end() &&
			 set2.size() == 1 && set2.find(ODD) != set1.end()))
		return OddSet;

	return union_sets(set1, set2);
}

// Processing Rem Instructions
std::set<int> processRem(llvm::Instruction *I, BBANALYSIS analysis)
{
	Value *op1 = I->getOperand(0);
	Value *op2 = I->getOperand(1);

	if (isa<ConstantInt>(op2))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
		int64_t op2Int = CI->getSExtValue();
		if (op2Int != 2)
		{
			std::set<int> set;
			set.insert(ODD);
			set.insert(EVEN);
			return set;
		}
	}

	std::set<int> set1, set2;
	if (isa<ConstantInt>(op1))
	{
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		if (op1Int % 2 == 1)
			set1.insert(ODD);
		else if (op1Int % 2 == 0)
			set1.insert(EVEN);
	}
	else if (analysis.find(op1) != analysis.end())
	{
		set1 = analysis[op1];
	}
	else
	{
		set1.insert(ODD);
		set1.insert(EVEN);
	}

	if (set1.size() == 1 && set1.find(ODD) != set1.end())
		return set1;

	if (set1.size() == 1 && set1.find(EVEN) != set1.end())
		return set1;

	return set1;
}

// update Basic Block Analysis
BBANALYSIS updateBBAnalysis(BasicBlock *BB, BBANALYSIS analysis)
{
	// Loop through instructions in BB
	for (auto &I : *BB)
	{
		if (isa<AllocaInst>(I))
		{
			analysis[&I] = processAlloca();
		}
		else if (isa<StoreInst>(I))
		{
			Value *op2 = I.getOperand(1);
			analysis[op2] = processStore(&I, analysis);
		}
		else if (isa<LoadInst>(I))
		{
			analysis[&I] = processLoad(&I, analysis);
		}
		else if (I.getOpcode() == BinaryOperator::SDiv)
		{
			analysis[&I] = processDiv(&I, analysis);
		}
		else if (I.getOpcode() == BinaryOperator::Mul)
		{
			analysis[&I] = processMul(&I, analysis);
		}
		else if (I.getOpcode() == BinaryOperator::Add || I.getOpcode() == BinaryOperator::Sub)
		{
			analysis[&I] = processAddSub(&I, analysis);
		}
		else if (I.getOpcode() == BinaryOperator::SRem)
		{
			analysis[&I] = processRem(&I, analysis);
		}
	}
	return analysis;
}

//======================================================================
// update Graph Analysis
//======================================================================

BBANALYSIS applyCond_aux(BBANALYSIS predSet, Instruction *I, std::set<int> set)
{
	if (isa<AllocaInst>(I))
	{
		predSet[I] = set;
	}
	else if (isa<LoadInst>(I))
	{
		predSet[I] = set;
		predSet = applyCond_aux(predSet, dyn_cast<Instruction>(I->getOperand(0)), set);
	}
	return predSet;
}

// Apply condition to the predecessor set
BBANALYSIS applyCond(BBANALYSIS predSet, BasicBlock *predecessor, BasicBlock *BB)
{
	for (auto &I : *predecessor)
	{
		if (isa<BranchInst>(I))
		{
			BranchInst *br = dyn_cast<BranchInst>(&I);
			if (!br->isConditional())
				return predSet;
			llvm::CmpInst *cmp = dyn_cast<llvm::CmpInst>(br->getCondition());

			Value *op1 = cmp->getOperand(0);
			Value *op2 = cmp->getOperand(1);
			std::set<int> set1, set2;
			if (isa<ConstantInt>(op1))
			{
				llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
				int64_t op1Int = CI->getSExtValue();
				if (op1Int % 2 == 1)
					set1.insert(ODD);
				else if (op1Int % 2 == 0)
					set1.insert(EVEN);
			}
			else if (predSet.find(op1) != predSet.end())
			{
				set1 = predSet[op1];
			}
			else
			{
				set1.insert(ODD);
				set1.insert(EVEN);
			}

			if (isa<ConstantInt>(op2))
			{
				llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op2);
				int64_t op2Int = CI->getSExtValue();
				if (op2Int % 2 == 1)
					set2.insert(ODD);
				else if (op2Int % 2 == 0)
					set2.insert(EVEN);
			}
			else if (predSet.find(op2) != predSet.end())
			{
				set2 = predSet[op2];
			}
			else
			{
				set2.insert(ODD);
				set2.insert(EVEN);
			}

			bool flag;
			if (BB == br->getOperand(2))
				flag = true;
			if (BB == br->getOperand(1))
				flag = false;

			int cmpValue;
			switch (cmp->getPredicate())
			{
			case llvm::CmpInst::ICMP_EQ:
			{
				if (flag == true)
					cmpValue = 1;
				else
				{
					cmpValue = 0;
					//if(set2.find(ODD) != set2.end()) 	set.insert(EVEN);
					//else set.insert(ODD);
				}
				break;
			}
			case llvm::CmpInst::ICMP_NE:
			{
				if (flag == false)
					cmpValue = 1; // set = set2;
				else
				{
					cmpValue = 0;
				}
				break;
			}
			}

			if (isa<LoadInst>(dyn_cast<Instruction>(cmp->getOperand(0))))
			{
				std::set<int> tempSet;
				Instruction *l = dyn_cast<Instruction>(cmp->getOperand(0));
				if (cmpValue == 1)
				{
					predSet[l] = set2;
					predSet = applyCond_aux(predSet, dyn_cast<Instruction>(l->getOperand(0)), set2);
				}
				else
				{
					tempSet.insert(ODD);
					tempSet.insert(EVEN);
					predSet[l] = tempSet;
					predSet = applyCond_aux(predSet, dyn_cast<Instruction>(l->getOperand(0)), tempSet);
				}
			}
			else if (dyn_cast<Instruction>(cmp->getOperand(0))->getOpcode() == BinaryOperator::SRem)
			{
				std::set<int> tempSet;
				Instruction *rem = dyn_cast<Instruction>(cmp->getOperand(0));
				if (cmpValue == 1)
				{
					predSet[rem] = set2;
					predSet = applyCond_aux(predSet, dyn_cast<Instruction>(rem->getOperand(0)), set2);
				}
				else
				{
					if (set2.find(ODD) != set2.end())
						tempSet.insert(EVEN);
					else
						tempSet.insert(ODD);
					predSet[rem] = tempSet;
					predSet = applyCond_aux(predSet, dyn_cast<Instruction>(rem->getOperand(0)), tempSet);
				}
			}
		}
	}
	return predSet;
}

// update Graph Analysis
void updateGraphAnalysis(Function *F)
{
	for (auto &BB : *F)
	{
		BBANALYSIS predUnion;
		// Load the current stored analysis for all predecessor nodes
		for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
		{
			BasicBlock *predecessor = *it;
			BBANALYSIS predSet = applyCond(analysisMap[predecessor->getName()], predecessor, &BB);
			predUnion = union_analysis(predUnion, predSet);
		}

		BBANALYSIS BBAnalysis = updateBBAnalysis(&BB, predUnion);
		BBANALYSIS OldBBAnalysis = analysisMap[BB.getName()];
		if (OldBBAnalysis != BBAnalysis)
		{
			analysisMap[BB.getName()] = union_analysis(BBAnalysis, OldBBAnalysis);
		}
	}
}

//======================================================================
// main function
//======================================================================

int main(int argc, char **argv)
{
	// Read the IR file.
	LLVMContext &Context = getGlobalContext();
	SMDiagnostic Err;

	// Extract Module M from IR (assuming only one Module exists)
	Module *M = ParseIRFile(argv[1], Err, Context);
	if (M == nullptr)
	{
		fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
		return EXIT_FAILURE;
	}

	// 1.Extract Function main from Module M
	Function *F = M->getFunction("main");

	// 2.Define analysisMap as a mapping of basic block labels to empty set (of instructions):
	// For example: Assume the input LLVM IR has 4 basic blocks, the map
	// would look like the following:
	// entry -> {}
	// if.then -> {}
	// if.else -> {}
	// if.end -> {}
	for (auto &BB : *F)
	{
		BBANALYSIS emptySet;
		analysisMap[BB.getName()] = emptySet;
	}
	// Note: All variables are of type "alloca" instructions. Ex.
	// Variable a: %a = alloca i32, align 4

	// Keeping a snapshot of the previous ananlysis
	std::map<std::string, BBANALYSIS> oldAnalysisMap;

	// Fixpoint Loop
	int i = 0;
	while (!fixPointReached(oldAnalysisMap))
	{
		oldAnalysisMap.clear();
		oldAnalysisMap.insert(analysisMap.begin(), analysisMap.end());
		updateGraphAnalysis(F);
		llvm::errs() << "Round:" << i++ << "\n";
		for (auto it = analysisMap.begin(); it != analysisMap.end(); ++it)
		{
			llvm::errs() << it->first << "\n";
			BBANALYSIS analysis = it->second;
			for (auto it1 = analysis.begin(); it1 != analysis.end(); ++it1)
			{
				it1->first->dump();
				std::set<int> set = it1->second;
				llvm::errs() << "\t";
				for (auto I : set)
				{
					if (I == 1)
						llvm::errs() << "ODD,";
					else if (I == 2)
						llvm::errs() << "EVEN,";
				}
				llvm::errs() << "\n";
			}
		}
		llvm::errs() << "\n";
		llvm::errs() << "\n";
		llvm::errs() << "\n";
	}

	for (auto it = analysisMap.begin(); it != analysisMap.end(); ++it)
	{
		llvm::errs() << it->first << "\n";
		BBANALYSIS analysis = it->second;
		for (auto it1 = analysis.begin(); it1 != analysis.end(); ++it1)
		{
			if (it1->first->getName() == "n")
			{
				it1->first->dump();
				std::set<int> set = it1->second;
				llvm::errs() << "\t";
				for (auto I : set)
				{
					if (I == 1)
						llvm::errs() << "ODD,";
					else if (I == 2)
						llvm::errs() << "EVEN,";
				}
				llvm::errs() << "\n";
			}
		}
	}

	return 0;
}
