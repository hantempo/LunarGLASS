//===- Loops.h - Utility functions and wrappers for loops -----------------===//
//
// LunarGLASS: An Open Modular Shader Compiler Architecture
// Copyright (c) 2010-2013 LunarG, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the
// License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
//===----------------------------------------------------------------------===//
//
// Author: Michael Ilseman, LunarG
//
// Provide a utility wrapper around llvm::Loop, and utility functions for
// dealing with and analyzing loops
//
//===----------------------------------------------------------------------===//

#ifndef GLA_LOOP_UTIL_H
#define GLA_LOOP_UTIL_H

#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include "Passes/Util/ADT.h"
#include "Passes/Util/BasicBlockUtil.h"

#include <stack>

namespace gla_llvm {
    using namespace llvm;
    class LoopStack;

    // TODO LLVM 3.2: Remove the need for the dominance frontier, it's
    // only used for the merges and testing for returns/discards. The second
    // purpose can be generalized elsewhere.

    // Loop wrapper providing more queries/information
    class LoopWrapper {
    public:
        LoopWrapper(Loop* loop, DominanceFrontier* df, ScalarEvolution* sev, bool simpleBE)
            : domFront(df)
            , scalarEvo(sev)
            , simpleLatch(simpleBE)
            , header(loop->getHeader())
            , latch(loop->getLoopLatch())
            , blocks(loop->getBlocks().begin(), loop->getBlocks().end())
            , uniqueExiting(loop->getExitingBlock())
            , inductiveVar(loop->getCanonicalInductionVariable())
            // Scalar Evolution's trip count for the header block will be 1
            // higher than the inductive loop's true trip count. This is
            // because it is counting the number of times that the exit
            // condition may be tested, which is 1 + the number of times the
            // exit condition fails.
            , tripCount(scalarEvo->getSmallConstantTripCount(loop, header) - 1)
            , loopDepth(loop->getLoopDepth())
            , simpleConditional(-1)
            , function(loop->getHeader()->getParent())
            , isMain(IsMain(*function))
            , stageExit(0)
            , stageEpilogue(0)
        {
            if (isMain) {
                stageExit    = GetMainExit(*function);
                stageEpilogue = GetMainEpilogue(*function);
            }

            loop->getUniqueExitBlocks(exits);

            SmallPtrSet<BasicBlock*,3> merges;
            GetMergePoints(exits, *domFront, merges);
            merges.erase(stageExit);
            merges.erase(stageEpilogue);

            // See if any of the exit subgraphs go directly to stage-epilogue or
            // stage-exit
            discards = false;
            returns  = false;
            for (SmallVector<BasicBlock*,4>::iterator bb = exits.begin(), e = exits.end(); bb != e; ++bb) {
                if (!returns && domFront->find(*bb)->second.count(stageEpilogue)) {
                    returns = true;
                }
                if (!discards && domFront->find(*bb)->second.count(stageExit)) {
                    discards = true;
                }
            }

            assert(merges.size() <= 1 && "Unstructured control flow");

            if (merges.size() == 1)
                exitMerge = *merges.begin();
            else
                exitMerge = NULL;

        }

        // Accessors
        BasicBlock* getHeader()    const { return header; }
        BasicBlock* getLatch()     const { return latch; }
        BasicBlock* getExitMerge() const { return exitMerge; }

        // Queries
        bool isSimpleLatching() const { return simpleLatch; }
        bool hasDiscard()       const { return discards; }
        bool hasReturn()        const { return returns; }

        bool isLatch(const BasicBlock* bb)   const { return latch == bb; }
        bool isHeader(const BasicBlock* bb)  const { return header == bb; }
        bool isExiting(const BasicBlock* bb) const { return isLoopExiting(bb); }

        // Whether the given instruction is inside the loop and referenced
        // outside the loop. Instructions not in the loop are trivially
        // externally referenced.
        // Linear in the number of uses
        bool isExternallyReferenced(const Instruction* inst) const
        {
            if (! contains(inst))
                return false;

            if (inst->use_empty())
                return false;

            for (Value::const_use_iterator i = inst->use_begin(), e = inst->use_end(); i != e; ++i) {
                const Instruction* use = dyn_cast<const Instruction>(*i);
                assert(use);
                // TODO: in some cases, it can appear in PHINodes where the phi
                // copies will be added prior to exiting the loop. Figure out
                // logic for this
                if (! contains(use))
                    return true;
            }

            return false;
        }


        // A loop-relevant block is one whose control-flow (e.g. successors)
        // makes this loop specially recognizable. This will be true for
        // headers, latches, and exiting blocks.
        bool isLoopRelevant(const BasicBlock* bb) const
        {
            return contains(bb) && (isExiting(bb) || isLatch(bb) || isHeader(bb));
        }

        // Provide interfaces from the Loop class
        unsigned getLoopDepth()                  const { return loopDepth; }
        PHINode* getCanonicalInductionVariable() const { return inductiveVar; }
        bool     contains(const BasicBlock* bb)  const { return blocks.count(bb); }
        bool     contains(const Instruction* i)  const { return blocks.count(i->getParent()); }
        unsigned int getTripCount()              const { return tripCount; }

        const SmallPtrSet<const BasicBlock*,16>& getBlocks() const { return blocks; }

        bool isLoopExiting(const BasicBlock* bb) const
        {
            for (succ_const_iterator i = succ_begin(bb), e = succ_end(bb); i != e; ++i)
                if (!contains(*i))
                    return true;

            return false;
        }


        // Note, we may want to move away from only wrapping LoopInfo and wrap
        // ScalarEvolution as well

        // TODO: cache the results of the more complicated queries (implemented
        // for isSimpleConditional())

        // Is the loop simple inductive one. A simple inductive loop is one
        // where the backedge is simple, a canonical induction variable exists,
        // and the execution count is known statically (e.g. there are no breaks
        // or continues)
        bool isSimpleInductive() const
        {
            // TODO: extend functionality to support early exit
            return inductiveVar && tripCount && uniqueExiting;
        }

        // Is the loop a simple conditional loop. A simple conditional loop is a
        // loop whose header is a conditionally exiting block, and the condition
        // is some comparison operator whose operands are extracts, constants,
        // or loads. Furthermore, all other instructions in the header can only
        // be phis.
        bool isSimpleConditional() const
        {
            // Check the cache
            if (simpleConditional != -1)
                return simpleConditional > 0;

            // It has to be conditional, comparison operator and the header has
            // to be exiting
            Value* v = GetCondition(header);
            if (! v || ! isLoopExiting(header))
                return false;

            CmpInst* cond = dyn_cast<CmpInst>(v);
            if (! cond)
                return false;

            // There can't be any other non-phi instructions in the block, so
            // keep track of the number of instructions we've seen.
            int count = 2;      // Already looked at the branch and condition

            // They have to be constants, extracts (vector swizzle), or loads
            // (uniforms). Add them to our count if they are in the header
            for (Instruction::const_op_iterator i = cond->op_begin(), e = cond->op_end(); i != e; ++i) {
                if (! (isa<ExtractElementInst>(i) || isa<LoadInst>(i) || isa<Constant>(i)))
                    return false;
                if (Instruction* inst = dyn_cast<Instruction>(i))
                    if (inst->getParent() == header)
                        ++count;
            }
            // TODO: handle when the cmp's arguments are phis

            // Add up the phis to the count
            for (BasicBlock::const_iterator i = header->begin(), e = header->end(); i != e; ++i) {
                if (! isa<PHINode>(i))
                    break;
                ++count;
            }

            // Our total has to be the number of instructions in the header.
            simpleConditional = header->size() == count ? 1 : 0;
            return simpleConditional > 0;
        }

        // Whether the loop is a canonical, structured loop.  In a canonical,
        // structured loop there should only be one latch.  Tf the latch is
        // conditional (and thus simple), the other branch should be an
        // exiting branch (enforces bottom-latching semantics).  If there are
        // multiple exit blocks, they should all eventually merge to a single
        // point that lies in the intersection of each of their dominance
        // frontiers (enforces structured flow control).
        bool isCanonical() const
        {
            return header && latch && exitMerge
                && (IsUnconditional(latch) || isLoopExiting(latch));
        }

        // If the loop is simple inductive, then returns the instruction
        // recomputing the exit condition, else returns NULL
        Instruction* getInductiveExitCondition() const
        {
            if (! isSimpleInductive())
                return NULL;

            // We currently only support inductive loops without early exit
            assert(uniqueExiting);

            BranchInst* br = dyn_cast<BranchInst>(uniqueExiting->getTerminator());
            assert(br && br->isConditional());

            return dyn_cast<Instruction>(br->getCondition());
        }

        // If the loop is simple inductive, then returns the instruction doing
        // the increment on the inductive variables, else returns NULL
        Instruction* getIncrement() const
        {
            if (! isSimpleInductive())
                return NULL;
            assert(inductiveVar);

            int index = inductiveVar->getBasicBlockIndex(latch);
            assert(index >= 0);

            return dyn_cast<Instruction>(inductiveVar->getIncomingValue(index));
        }

        // TODO: refactor into mask or enum or interface
        // Returns the successor number (0 or 1) of the exiting edge from an exiting
        // block. Returns -1 if none exit, and 2 if they both exit.
        int exitSuccNumber(const BasicBlock* bb) const
        {
            if (! isLoopExiting(bb))
                return -1;

            const BranchInst* br = dyn_cast<BranchInst>(bb->getTerminator());
            assert(br);

            if (br->isUnconditional())
                return 0;

            if (! contains(br->getSuccessor(0)) && ! contains(br->getSuccessor(1)))
                return 2;

            // Else return 0 or 1 indicating which one is not contained
            return contains(br->getSuccessor(0));
        }

        ~LoopWrapper() { }

    protected:
        // LoopInfo* loopInfo;
        DominanceFrontier* domFront;
        ScalarEvolution* scalarEvo;

        bool simpleLatch;

        BasicBlock* header;
        BasicBlock* latch;

        SmallPtrSet<const BasicBlock*, 16> blocks;

        BasicBlock* uniqueExiting;

        PHINode* inductiveVar;
        unsigned int tripCount;

        unsigned loopDepth;

        // Cache
        mutable int simpleConditional;

        Function* function;
        bool isMain;

        SmallVector<BasicBlock*, 4> exits;

        bool discards;
        bool returns;

        BasicBlock* stageExit;
        BasicBlock* stageEpilogue;

        BasicBlock* exitMerge;

    private:
        LoopWrapper(const LoopWrapper&);       // do not implement
        void operator=(const LoopWrapper&);    // do not implement

    };

} // end namespace llvm_gla


#endif // GLA_LOOP_UTIL_H
