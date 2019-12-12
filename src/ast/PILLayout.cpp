//===--- PILLayout.cpp - Defines PIL-level aggregate layouts --------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines classes that describe the physical layout of nominal
// types in PIL, including structs, classes, and boxes. This is distinct from
// the AST-level layout for several reasons:
// - It avoids redundant work lowering the layout of aggregates from the AST.
// - It allows optimizations to manipulate the layout of aggregates without
//   requiring changes to the AST. For instance, optimizations can eliminate
//   dead fields from instances or turn invariant fields into global variables.
// - It allows for PIL-only aggregates to exist, such as boxes.
// - It improves the robustness of code in the face of resilience. A resilient
//   type can be modeled in PIL as not having a layout at all, preventing the
//   inappropriate use of fragile projection and injection operations on the
//   type.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/PILLayout.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/Range.h"

namespace polar::ast {

using polar::basic::indices;

static bool anyMutable(ArrayRef <PILField> Fields) {
   for (auto &field : Fields) {
      if (field.isMutable())
         return true;
   }
   return false;
}

#ifndef NDEBUG

/// Verify that the types of fields are valid within a given generic signature.
static void verifyFields(CanGenericSignature Sig, ArrayRef <PILField> Fields) {
   for (auto &field : Fields) {
      auto ty = field.getLoweredType();
      // Layouts should never refer to archetypes, since they represent an
      // abstract generic type layout.
      assert(!ty->hasArchetype()
             && "PILLayout field cannot have an archetype type");
      assert(!ty->hasTypeVariable()
             && "PILLayout cannot contain constraint system type variables");
      if (!ty->hasTypeParameter())
         continue;
      field.getLoweredType().findIf([Sig](Type t) -> bool {
         if (auto gpt = t->getAs<GenericTypeParamType>()) {
            // Check that the generic param exists in the generic signature.
            assert(Sig && "generic param in nongeneric layout?");
            assert(std::find(Sig.getGenericParams().begin(),
                             Sig.getGenericParams().end(),
                             gpt->getCanonicalType()) != Sig.getGenericParams().end()
                   && "generic param not declared in generic signature?!");
         }
         return false;
      });
   }
}

#endif

PILLayout::PILLayout(CanGenericSignature Sig,
                     ArrayRef <PILField> Fields)
   : GenericSigAndFlags(Sig, getFlagsValue(anyMutable(Fields))),
     NumFields(Fields.size()) {
#ifndef NDEBUG
   verifyFields(Sig, Fields);
#endif
   auto FieldsMem = getTrailingObjects<PILField>();
   for (unsigned i : indices(Fields)) {
      new(FieldsMem + i) PILField(Fields[i]);
   }
}

void PILLayout::Profile(llvm::FoldingSetNodeID &id,
                        CanGenericSignature Generics,
                        ArrayRef <PILField> Fields) {
   id.AddPointer(Generics.getPointer());
   for (auto &field : Fields) {
      id.AddPointer(field.getLoweredType().getPointer());
      id.AddBoolean(field.isMutable());
   }
}

} // polar::ast