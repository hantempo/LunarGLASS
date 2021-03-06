//===- ConstantUtil.h - Utility functions for constants -------------------===//
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
// Provides utility functions for Constants
//
//===----------------------------------------------------------------------===//

#ifndef GLA_CONSTANTUTIL_H
#define GLA_CONSTANTUTIL_H

#include "llvm/Constants.h"

// LunarGLASS helpers
#include "LunarGLASSTopIR.h"
#include "Util.h"

namespace gla_llvm {
    using namespace llvm;

    inline bool IsOne(Constant* c)
    {
        if (ConstantFP* cFP = dyn_cast<ConstantFP>(c))
            return cFP->isExactlyValue(1.0);

        if (ConstantVector* cVector = dyn_cast<ConstantVector>(c)) {
            Constant* splat = cVector->getSplatValue();
            return splat && IsOne(splat);
        }

        if (ConstantDataVector* cVector = dyn_cast<ConstantDataVector>(c)) {
            Constant* splat = cVector->getSplatValue();
            return splat && IsOne(splat);
        }

        if (ConstantInt* cInt = dyn_cast<ConstantInt>(c))
            return cInt->equalsInt(1);

        return false;
    }

    // Get all the elements of an aggregate data type. Will fill with zeros for
    // ConstantAggregateZero and undefs for UndefValues. Result may contain
    // NULLs from calls to getAggregateElement(). Currently only supports
    // vectors.
    inline void GetElements(const Constant* c, SmallVectorImpl<Constant*>& res)
    {
        // TODO LLVM 3.2: handle ConstantDataVector
        for (unsigned int i = 0; i < gla::GetComponentCount(c); ++i)
            res.push_back(c->getAggregateElement(i));
    }

} // end namespace gla_llvm

#endif /* GLA_CONSTANTUTIL_H */
