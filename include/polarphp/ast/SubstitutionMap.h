//===--- SubstitutionMap.h - Swift Substitution Map ASTs --------*- C++ -*-===//
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
// This file defines the SubstitutionMap class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_SUBSTITUTION_MAP_H
#define POLARPHP_AST_SUBSTITUTION_MAP_H

#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/InterfaceConformanceRef.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeExpansionContext.h"
#include "polarphp/basic/Debug.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Optional.h"

namespace llvm {
class FoldingSetNodeID;
}

namespace polar {

class GenericEnvironment;
class SubstitutableType;
typedef CanTypeWrapper<GenericTypeParamType> CanGenericTypeParamType;

template<class Type> class CanTypeWrapper;
typedef CanTypeWrapper<SubstitutableType> CanSubstitutableType;

enum class CombineSubstitutionMaps
{
   AtDepth,
   AtIndex
};

/// SubstitutionMap is a data structure type that describes the mapping of
/// abstract types to replacement types, together with associated conformances
/// to use for deriving nested types and conformances.
///
/// Substitution maps are primarily used when performing substitutions into
/// any entity that can reference type parameters, e.g., types (via
/// Type::subst()) and conformances (via InterfaceConformanceRef::subst()).
///
/// SubstitutionMaps are constructed by calling the getSubstitutionMap() method
/// on a GenericSignature or (equivalently) by calling one of the static
/// \c SubstitutionMap::get() methods. However, most substitution maps are
/// computed using higher-level entry points such as
/// TypeBase::getMemberSubstitutionMap().
///
/// Substitution maps are AstContext-allocated and are uniqued on construction,
/// so they can be used as fields in AST nodes.
class SubstitutionMap
{
public:
   /// Stored data for a substitution map, which uses tail allocation for the
   /// replacement types and conformances.
   class Storage;

private:
   /// The storage needed to describe the set of substitutions.
   ///
   /// When null, this substitution map is empty, having neither a generic
   /// signature nor any replacement types/conformances.
   Storage *storage = nullptr;

public:
   /// Retrieve the array of replacement types, which line up with the
   /// generic parameters.
   ///
   /// Note that the types may be null, for cases where the generic parameter
   /// is concrete but hasn't been queried yet.
   ///
   /// Prefer \c getReplacementTypes, this is public for printing purposes.
   ArrayRef<Type> getReplacementTypesBuffer() const;

private:
   MutableArrayRef<Type> getReplacementTypesBuffer();

   /// Retrieve a mutable reference to the buffer of conformances.
   MutableArrayRef<InterfaceConformanceRef> getConformancesBuffer();

   /// Form a substitution map for the given generic signature with the
   /// specified replacement types and conformances.
   SubstitutionMap(GenericSignature genericSig,
                   ArrayRef<Type> replacementTypes,
                   ArrayRef<InterfaceConformanceRef> conformances);

   explicit SubstitutionMap(Storage *storage) : storage(storage) { }

public:
   /// Build an empty substitution map.
   SubstitutionMap() { }

   /// Build an interface type substitution map for the given generic
   /// signature and a vector of Substitutions that correspond to the
   /// requirements of this generic signature.
   static SubstitutionMap get(GenericSignature genericSig,
                              ArrayRef<Type> replacementTypes,
                              ArrayRef<InterfaceConformanceRef> conformances) {
      return SubstitutionMap(genericSig, replacementTypes, conformances);
   }

   /// Build an interface type substitution map for the given generic
   /// signature using the mapping in the given substitutions.
   static SubstitutionMap get(GenericSignature genericSig,
                              SubstitutionMap substitutions);

   /// Build an interface type substitution map for the given generic signature
   /// from a type substitution function and conformance lookup function.
   static SubstitutionMap get(GenericSignature genericSig,
                              TypeSubstitutionFn subs,
                              LookupConformanceFn lookupConformance);

   /// Retrieve the generic signature describing the environment in which
   /// substitutions occur.
   GenericSignature getGenericSignature() const;

   /// Retrieve the array of protocol conformances, which line up with the
   /// requirements of the generic signature.
   ArrayRef<InterfaceConformanceRef> getConformances() const;

   /// Look up a conformance for the given type to the given protocol.
   InterfaceConformanceRef lookupConformance(CanType type,
                                             InterfaceDecl *proto) const;

   /// Whether the substitution map is empty.
   bool empty() const;

   /// Whether the substitution has any substitutable parameters, i.e.,
   /// it is non-empty and at least one of the type parameters can be
   /// substituted (i.e., is not mapped to a concrete type).
   bool hasAnySubstitutableParams() const;

   /// True if this substitution map is an identity mapping.
   bool isIdentity() const;

   /// Whether the substitution map is non-empty.
   explicit operator bool() const { return !empty(); }

   /// Retrieve the array of replacement types, which line up with the
   /// generic parameters.
   ArrayRef<Type> getReplacementTypes() const;

   /// Query whether any replacement types in the map contain archetypes.
   bool hasArchetypes() const;

   /// Query whether any replacement types in the map contain an opened
   /// existential.
   bool hasOpenedExistential() const;

   /// Query whether any replacement types in the map contain dynamic Self.
   bool hasDynamicSelf() const;

   /// Whether the replacement types are all canonical.
   bool isCanonical() const;

   /// Return the canonical form of this substitution map.
   SubstitutionMap getCanonical() const;

   /// Apply a substitution to all replacement types in the map. Does not
   /// change keys.
   SubstitutionMap subst(SubstitutionMap subMap,
                         SubstOptions options=None) const;

   /// Apply a substitution to all replacement types in the map. Does not
   /// change keys.
   SubstitutionMap subst(TypeSubstitutionFn subs,
                         LookupConformanceFn conformances,
                         SubstOptions options=None) const;

   /// Apply type expansion lowering to all types in the substitution map. Opaque
   /// archetypes will be lowered to their underlying types if the type expansion
   /// context allows.
   SubstitutionMap mapIntoTypeExpansionContext(
         TypeExpansionContext context) const;

   /// Create a substitution map for a protocol conformance.
   static SubstitutionMap
   getInterfaceSubstitutions(InterfaceDecl *protocol,
                             Type selfType,
                             InterfaceConformanceRef conformance);

   /// Given that 'derivedDecl' is an override of 'baseDecl' in a subclass,
   /// and 'derivedSubs' is a set of substitutions written in terms of the
   /// generic signature of 'derivedDecl', produce a set of substitutions
   /// written in terms of the generic signature of 'baseDecl'.
   static SubstitutionMap
   getOverrideSubstitutions(const ValueDecl *baseDecl,
                            const ValueDecl *derivedDecl,
                            Optional<SubstitutionMap> derivedSubs);

   /// Variant of the above for when we have the generic signatures but not
   /// the decls for 'derived' and 'base'.
   static SubstitutionMap
   getOverrideSubstitutions(const ClassDecl *baseClass,
                            const ClassDecl *derivedClass,
                            GenericSignature baseSig,
                            GenericSignature derivedSig,
                            Optional<SubstitutionMap> derivedSubs);

   /// Combine two substitution maps as follows.
   ///
   /// The result is written in terms of the generic parameters of 'genericSig'.
   ///
   /// Generic parameters with a depth or index less than 'firstDepthOrIndex'
   /// come from 'firstSubMap'.
   ///
   /// Generic parameters with a depth greater than 'firstDepthOrIndex' come
   /// from 'secondSubMap', but are looked up starting with a depth or index of
   /// 'secondDepthOrIndex'.
   ///
   /// The 'how' parameter determines if we're looking at the depth or index.
   static SubstitutionMap
   combineSubstitutionMaps(SubstitutionMap firstSubMap,
                           SubstitutionMap secondSubMap,
                           CombineSubstitutionMaps how,
                           unsigned baseDepthOrIndex,
                           unsigned origDepthOrIndex,
                           GenericSignature genericSig);

   /// Swap archetypes in the substitution map's replacement types with their
   /// interface types.
   SubstitutionMap mapReplacementTypesOutOfContext() const;

   /// Verify that this substitution map is valid.
   void verify() const;

   /// Whether to dump the full substitution map, or just a minimal useful subset
   /// (on a single line).
   enum class DumpStyle { Minimal, Full };
   /// Dump the contents of this substitution map for debugging purposes.
   void dump(llvm::raw_ostream &out, DumpStyle style = DumpStyle::Full,
             unsigned indent = 0) const;

   POLAR_DEBUG_DUMP;

   /// Profile the substitution map, for use with LLVM's FoldingSet.
   void profile(llvm::FoldingSetNodeID &id) const;

   const void *getOpaqueValue() const { return storage; }

   static SubstitutionMap getFromOpaqueValue(const void *ptr) {
      return SubstitutionMap(const_cast<Storage *>((const Storage *)ptr));
   }

   static SubstitutionMap getEmptyKey() {
      return SubstitutionMap(
               (Storage *)llvm::DenseMapInfo<void*>::getEmptyKey());
   }

   static SubstitutionMap getTombstoneKey() {
      return SubstitutionMap(
               (Storage *)llvm::DenseMapInfo<void*>::getTombstoneKey());
   }

   friend bool operator ==(SubstitutionMap lhs, SubstitutionMap rhs) {
      return lhs.storage == rhs.storage;
   }

   friend bool operator !=(SubstitutionMap lhs, SubstitutionMap rhs) {
      return lhs.storage != rhs.storage;
   }

private:
   friend class GenericSignature;
   friend class GenericEnvironment;
   friend struct QuerySubstitutionMap;

   /// Look up the replacement for the given type parameter or interface type.
   /// Note that this only finds replacements for maps that are directly
   /// stored inside the map. In most cases, you should call Type::subst()
   /// instead, since that will resolve member types also.
   Type lookupSubstitution(CanSubstitutableType type) const;
};

/// A function object suitable for use as a \c TypeSubstitutionFn that
/// queries an underlying \c SubstitutionMap.
struct QuerySubstitutionMap
{
   SubstitutionMap subMap;

   Type operator()(SubstitutableType *type) const;
};

/// Functor class suitable for use as a \c LookupConformanceFn to look up a
/// conformance in a \c SubstitutionMap.
class LookUpConformanceInSubstitutionMap {
   SubstitutionMap Subs;
public:
   explicit LookUpConformanceInSubstitutionMap(SubstitutionMap Subs)
      : Subs(Subs) {}

   InterfaceConformanceRef operator()(CanType dependentType,
                                      Type conformingReplacementType,
                                      InterfaceDecl *conformedInterface) const;
};

} // end namespace polar

namespace llvm
{
template <>
struct PointerLikeTypeTraits<polar::SubstitutionMap> {
   static void *getAsVoidPointer(polar::SubstitutionMap map) {
      return const_cast<void *>(map.getOpaqueValue());
   }
   static polar::SubstitutionMap getFromVoidPointer(const void *ptr) {
      return polar::SubstitutionMap::getFromOpaqueValue(ptr);
   }

   /// Note: Assuming storage is at leaste 4-byte aligned.
   enum { NumLowBitsAvailable = 2 };
};

// Substitution maps hash just like pointers.
template<> struct DenseMapInfo<polar::SubstitutionMap> {
   static polar::SubstitutionMap getEmptyKey() {
      return polar::SubstitutionMap::getEmptyKey();
   }
   static polar::SubstitutionMap getTombstoneKey() {
      return polar::SubstitutionMap::getTombstoneKey();
   }
   static unsigned getHashValue(polar::SubstitutionMap map) {
      return DenseMapInfo<void*>::getHashValue(map.getOpaqueValue());
   }
   static bool isEqual(polar::SubstitutionMap lhs,
                       polar::SubstitutionMap rhs) {
      return lhs.getOpaqueValue() == rhs.getOpaqueValue();
   }
};

} // llvm

#endif
