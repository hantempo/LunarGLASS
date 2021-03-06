//===- Backend.h - Public interface to LunarGLASS Backends ----------------===//
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
// Author: John Kessenich, LunarG
// Author: Michael Ilseman, LunarG
//
// Public interface to LunarGLASS Backends.
//
// Don't include Mesa headers here.  LunarGLASS is not Mesa-dependent.
//
// Don't include LLVM headers here.  Users of LunarGLASS don't need to
// pull in LLVM headers.
//
//===----------------------------------------------------------------------===//

#pragma once
#ifndef Backend_H
#define Backend_H

#include "LunarGLASSManager.h"
#include "metadata.h"

// Forward decls
namespace llvm {
    class PHINode;
    class Value;
    class CmpInst;
    class GlobalVariable;
    class Instruction;
    class StringRef;
} // end namespace llvm

namespace gla {

    // support for back-end queries
    enum EFlowControlMode {
        EFcmStructuredOpCodes,
        EFcmExplicitMasking,
        EFcmDynamic
    };

    enum EDecomposableIntrinsic {
        EDiMin,
        EDiMax,
        EDiClamp,
        EDiAsin,
        EDiAcos,
        EDiAtan,
        EDiAtan2,
        EDiSinh,
        EDiCosh,
        EDiTanh,
        EDiASinh,
        EDiACosh,
        EDiATanh,
        EDiPowi,
        EDiExp10,       // decompose from base 10 to base 2
        EDiLog10,       // decompose from base 10 to base 2
        EDiExp,         // decompose from base e to base 2
        EDiLog,         // decompose from base e to base 2
        EDiInverseSqrt,
        EDiFraction,
        EDiSign,
        EDiModF,
        EDiMix,
        EDiStep,
        EDiSmoothStep,
        EDiIsNan,
        EDiFma,
        EDiPackUnorm2x16,
        EDiPackUnorm4x8,
        EDiPackSnorm4x8,
        EDiUnpackUnorm2x16,
        EDiUnpackUnorm4x8,
        EDiUnpackSnorm4x8,
        EDiPackDouble2x32,
        EDiUnpackDouble2x32,
        EDiLength,
        EDiDistance,
        EDiDot,
        EDiCross,
        EDiNormalize,
        EDiNormalize3D,
        EDiLit,
        EDiFaceForward,
        EDiReflect,
        EDiRefract,
        EDiFilterWidth,
        EDiFixedTransform,
        EDiAny,
        EDiAll,
        EDiNot,

        // Intrinsic preferences (decompose to simpler form when possible)
        EDiPreferSwizzle,       // Make swizzles when possible

        // do projection of texture coordinates
        EDiTextureProjection,
        
        EDiCount // number of entries in this table
    };

    enum EVariableQualifier {
        EVQNone,
        EVQUniform,
        EVQGlobal,
        EVQInput,
        EVQOutput,
        EVQTemporary,
        EVQConstant,
        EVQUndef
    };

    class BackEndTranslator {
    public:

        explicit BackEndTranslator(Manager* m) : manager(m) { }
        virtual ~BackEndTranslator() { }
        virtual void start() { }
        virtual void addGlobal(const llvm::GlobalVariable*) { }
        virtual void addIoDeclaration(EVariableQualifier, const llvm::MDNode*) { }
        virtual void startFunctionDeclaration(const llvm::Type*, llvm::StringRef) = 0;
        virtual void addArgument(const llvm::Value*, bool last) = 0;
        virtual void endFunctionDeclaration() = 0;
        virtual void startFunctionBody() = 0;
        virtual void endFunctionBody() = 0;
        virtual void addInstruction(const llvm::Instruction*, bool lastBlock, bool referencedOutsideScope=false) = 0;

        //
        // The following set of functions is motivated by need to convert to
        // structured flow control and eliminate phi functions.
        //

        // This must get called soon enough that it is before the split that
        // makes the phi.  It is then referred to in addPhiCopy().
        virtual void declarePhiCopy(const llvm::Value* dst) { }

        // Called for Phi elimination.
        virtual void addPhiCopy(const llvm::Value* dst, const llvm::Value* src) = 0;

        // Called to build structured flow control.
        virtual void addIf(const llvm::Value* cond, bool invert=false) = 0;
        virtual void addElse() = 0;
        virtual void addEndif() = 0;


        // Specialized loop constructs

        // TODO: add more loop constructs here

        // TODO: add backend queries for each of these forms

        // TODO: figure out how to name things, as this for now simpleInductive
        // really means "simpleStaticInductive". Alternatively, you could remove
        // all the static constraints.

        // Add a simple conditional loop. A simple conditional loop has a simple
        // conditional expression, which consists of a comparison operation and
        // two arguments that must either be constants, extracts (from a vector),
        // or loads (for uniforms). Receives the comparison instruction, the
        // first operand, and the second operand. If the result of the
        // comparison should be inverted, invert will be passed as true.
        virtual void beginSimpleConditionalLoop(const llvm::CmpInst* cmp, const llvm::Value* op1, const llvm::Value* op2, bool invert=false) = 0;

        // Add a simple inductive loop. A simple inductive loop is a loop with
        // an inductive variable starting at 0, and incrementing by 1 through
        // the loop until some final value. 
        //
        // A simple inductive loop can have a statically known final value
        // (and hence trip count), or the final value may be a dynamic value.
        //
        // It is given the phi node corresponding to the induction
        // variable, and how many times the body will be executed as 
        // a compile-time constant (for static) or an LLVM value 
        // (for dynamic).
        virtual void beginSimpleInductiveLoop(const llvm::PHINode* phi, unsigned count) = 0;
        virtual void beginSimpleInductiveLoop(const llvm::PHINode* phi, const llvm::Value* count) = 0;

        // Generic loop constructs

        // TODO: change name to be generic loop
        // Add a generic looping construct (e.g. while (true))
        virtual void beginLoop() = 0;

        // Add an end for any of these loops (e.g. '}')
        virtual void endLoop() = 0;

        // Exit the loop (e.g. break). Recieves the condition if it's a
        // conditional exit. If it's conditional, and the condition should be
        // inverted for the exit, then invert will be true.
        virtual void addLoopExit(const llvm::Value* condition=NULL, bool invert=false) = 0;

        // Add a loop backedge (e.g. continue). Receives the condition if it's a
        // conditional loop-back. If it's conditional, and the condition should
        // be inverted for the loop-back, then invert will be true.
        virtual void addLoopBack(const llvm::Value* condition=NULL, bool invert=false) = 0;

        // Discard instructions
        virtual void addDiscard() = 0;

        virtual void print() = 0;

    protected:
        Manager* manager;
    };

    // Abstract class of back-end queries.  Back-end inherits from this to provide
    // correct answers to queries.  Use getBackEndQueries to get a real one.  The
    // methods here can be used by the derived class as defaults or as initial
    // values that some of are then overridden.
    class BackEnd {
    public:

        BackEnd()
        {
            for (int d = 0; d < EDiCount; ++d)
                decompose[d] = false;

            decompose[EDiPreferSwizzle] = true;
        }

        virtual ~BackEnd() {};

        // despite being pure virtual, there is a base implementation available
        virtual void getRegisterForm(int& outerSoA, int& innerAoS)  = 0;

        virtual void getControlFlowMode(EFlowControlMode& flowControlMode,
                                        bool& breakOp, bool& continueOp,
                                        bool& earlyReturnOp, bool& discardOp)
        {
            flowControlMode = EFcmStructuredOpCodes;
            breakOp = true;
            continueOp = true;
            earlyReturnOp = true;
            discardOp = true;
        }

        virtual bool decomposeIntrinsic(int intrinsic)
        {
            return decompose[intrinsic];
        }

        virtual bool preferRegistersOverMemory()
        {
            return true;
        }

        virtual bool getRemovePhiFunctions()
        {
            return true;
        }

        // Does the backend want to be notified before the control splits what the
        // targets of phi functions will be when eliminating phi functions?
        virtual bool getDeclarePhiCopies()
        {
            return false;
        }

        // Does the backend want to see fcmp ord ..., or would it rather have
        // LunarGLASS decompose it into fcmp eq %foo %foo?
        virtual bool decomposeNaNCompares()
        {
            return false;
        }

        // Does the backend want GEP constant expressions to be hoisted into
        // their own instructions?
        virtual bool hoistGEPConstantOperands()
        {
            return true;
        }

        // Does the backend want LunarGLASS to hoist undef operands into
        // globals? Doesn't hoist operands for vector instructions/intrinsics
        // that require a constant vector for their masks.
        virtual bool hoistUndefOperands()
        {
            return true;
        }

        // Would the back-end translator like to only have "add" called on
        // output instructions (e.g. fWriteData)?
        virtual bool addOutputInstructionsOnly()
        {
            return false;
        }

        // Would the back-end translator like to see discards hoisted into
        // discardConditional?
        virtual bool hoistDiscards()
        {
            return true;
        }

        // Should LunarGLASS try to split writeData and fWriteDatas of
        // multi-inserts out into multiple masked writes?
        virtual bool splitWrites()
        {
            return false;
        }

    protected:
        bool decompose[EDiCount];

    };
};

#endif /* Backend_H */
