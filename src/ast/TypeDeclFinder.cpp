//===--- TypeDeclFinder.cpp - Finds TypeDecls inside Types/TypeReprs ------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/TypeDeclFinder.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/ast/Types.h"

namespace polar {

TypeWalker::Action TypeDeclFinder::walkToTypePre(Type T) {
   if (auto *TAT = dyn_cast<TypeAliasType>(T.getPointer()))
      return visitTypeAliasType(TAT);

   // FIXME: We're looking through sugar here so that we visit, e.g.,
   // Swift.Array when we see `[Int]`. But that means we do redundant work when
   // we see sugar that's purely structural, like `(Int)`. Fortunately, paren
   // types are the only such purely structural sugar at the time this comment
   // was written, and they're not so common in the first place.
   if (auto *BGT = T->getAs<BoundGenericType>())
      return visitBoundGenericType(BGT);
   if (auto *NT = T->getAs<NominalType>())
      return visitNominalType(NT);

   return Action::Continue;
}

TypeWalker::Action
SimpleTypeDeclFinder::visitNominalType(NominalType *ty) {
   return Callback(ty->getDecl());
}

TypeWalker::Action
SimpleTypeDeclFinder::visitBoundGenericType(BoundGenericType *ty) {
   return Callback(ty->getDecl());
}

TypeWalker::Action
SimpleTypeDeclFinder::visitTypeAliasType(TypeAliasType *ty) {
   return Callback(ty->getDecl());
}

bool TypeReprIdentFinder::walkToTypeReprPost(TypeRepr *TR) {
   auto CITR = dyn_cast<ComponentIdentTypeRepr>(TR);
   if (!CITR || !CITR->getBoundDecl())
      return true;
   return Callback(CITR);
}

} // polar