//===--- GenClass.h - Swift IR generation for classes -----------*- C++ -*-===//
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
//  This file provides the private interface to the class-emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENCLASS_H
#define POLARPHP_IRGEN_INTERNAL_GENCLASS_H

#include "polarphp/ast/Types.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"

namespace llvm {
class Constant;
class Value;
class Function;
}

namespace polar {

class ClassDecl;
class ExtensionDecl;
class InterfaceDecl;
struct PILDeclRef;
class PILType;
class VarDecl;

namespace irgen {

class ConstantStructBuilder;
class FunctionPointer;
class HeapLayout;
class IRGenFunction;
class IRGenModule;
class MemberAccessStrategy;
class OwnedAddress;
class Address;
class Size;
class StructLayout;
class TypeInfo;

enum class ClassDeallocationKind : unsigned char;
enum class FieldAccess : uint8_t;

OwnedAddress projectPhysicalClassMemberAddress(
   IRGenFunction &IGF, llvm::Value *base,
   PILType baseType, PILType fieldType, VarDecl *field);

/// Return a strategy for accessing the given stored class property.
///
/// This API is used by RemoteAST.
MemberAccessStrategy
getPhysicalClassMemberAccessStrategy(IRGenModule &IGM,
                                     PILType baseType, VarDecl *field);


enum ForMetaClass_t : bool {
   ForClass = false,
   ForMetaClass = true
};

enum HasUpdateCallback_t : bool {
   DoesNotHaveUpdateCallback = false,
   HasUpdateCallback = true
};

/// Creates a layout for the class \p classType with allocated tail elements
/// \p tailTypes.
///
/// The caller is responsible for deleting the returned StructLayout.
StructLayout *getClassLayoutWithTailElems(IRGenModule &IGM, PILType classType,
                                          llvm::ArrayRef<PILType> tailTypes);

ClassDecl *getRootClassForMetaclass(IRGenModule &IGM, ClassDecl *theClass);

enum class ClassMetadataStrategy {
   /// Does the given class have resilient ancestry, or is the class itself
   /// generic?
      Resilient,

   /// Does the class require at in-place initialization because of
   /// non-fixed size properties or generic ancestry? The class does not
   /// export a static symbol visible to Objective-C code.
      Singleton,

   /// A more restricted case of the above. Does the class require at in-place
   /// initialization because of non-fixed size properties, while exporting a
   /// static symbol visible to Objective-C code? The Objective-C runtime is
   /// able to initialize the metadata by calling the update callback stored
   /// in rodata. This strategy can only be used if the class availability
   /// restricts its use to newer Objective-C runtimes that support this
   /// feature.
      Update,

   /// An even more restricted case of the above. The class requires in-place
   /// initialization on newer Objective-C runtimes, but the metadata is
   /// statically valid on older runtimes because field offsets were computed
   /// assuming type layouts loaded from a legacy type info YAML file.
      FixedOrUpdate,

   /// The class metadata is completely static and only Objective-C runtime
   /// realization (and possibly field offset sliding) must be performed.
      Fixed
};

std::pair<Size,Size>
emitClassPrivateDataFields(IRGenModule &IGM,
                           ConstantStructBuilder &builder,
                           ClassDecl *cls);

llvm::Constant *emitClassPrivateData(IRGenModule &IGM, ClassDecl *theClass);
void emitGenericClassPrivateDataTemplate(IRGenModule &IGM,
                                         ClassDecl *theClass,
                                         llvm::SmallVectorImpl<llvm::Constant*> &fields,
                                         Size &metaclassOffset,
                                         Size &classRODataOffset,
                                         Size &metaclassRODataOffset,
                                         Size &totalSize);
llvm::Constant *emitCategoryData(IRGenModule &IGM, ExtensionDecl *ext);
llvm::Constant *emitObjCInterfaceData(IRGenModule &IGM, InterfaceDecl *ext);

/// Emit a projection from a class instance to the first tail allocated
/// element.
Address emitTailProjection(IRGenFunction &IGF, llvm::Value *Base,
                           PILType ClassType, PILType TailType);

using TailArraysRef = llvm::ArrayRef<std::pair<PILType, llvm::Value *>>;

/// Adds the size for tail allocated arrays to \p size and returns the new
/// size value. Also updades the alignment mask to represent the alignment of
/// the largest element.
std::pair<llvm::Value *, llvm::Value *>
appendSizeForTailAllocatedArrays(IRGenFunction &IGF,
                                 llvm::Value *size, llvm::Value *alignMask,
                                 TailArraysRef TailArrays);

/// Emit an allocation of a class.
/// The \p StackAllocSize is an in- and out-parameter. The passed value
/// specifies the maximum object size for stack allocation. A negative value
/// means that no stack allocation is possible.
/// The returned \p StackAllocSize value is the actual size if the object is
/// allocated on the stack or -1, if the object is allocated on the heap.
llvm::Value *emitClassAllocation(IRGenFunction &IGF, PILType selfType,
                                 bool objc, int &StackAllocSize, TailArraysRef TailArrays);

/// Emit an allocation of a class using a metadata value.
llvm::Value *emitClassAllocationDynamic(IRGenFunction &IGF,
                                        llvm::Value *metadata,
                                        PILType selfType,
                                        bool objc, TailArraysRef TailArrays);

/// Emit class deallocation.
void emitClassDeallocation(IRGenFunction &IGF, PILType selfType,
                           llvm::Value *selfValue);

/// Emit class deallocation.
void emitPartialClassDeallocation(IRGenFunction &IGF,
                                  PILType selfType,
                                  llvm::Value *selfValue,
                                  llvm::Value *metadataValue);

/// We emit Objective-C class stubs for non-generic classes with resilient
/// ancestry. This lets us attach categories to the class even though it
/// does not have statically-emitted metadata.
bool hasObjCResilientClassStub(IRGenModule &IGM, ClassDecl *D);

/// Emit a resilient class stub.
void emitObjCResilientClassStub(IRGenModule &IGM, ClassDecl *D);

/// Emit the constant fragile offset of the given property inside an instance
/// of the class.
llvm::Constant *tryEmitConstantClassFragilePhysicalMemberOffset(
   IRGenModule &IGM, PILType baseType, VarDecl *field);

FieldAccess getClassFieldAccess(IRGenModule &IGM,
                                PILType baseType,
                                VarDecl *field);

Size getClassFieldOffset(IRGenModule &IGM,
                         PILType baseType,
                         VarDecl *field);

/// Load the instance size and alignment mask from a reference to
/// class type metadata of the given type.
std::pair<llvm::Value *, llvm::Value *>
emitClassResilientInstanceSizeAndAlignMask(IRGenFunction &IGF,
                                           ClassDecl *theClass,
                                           llvm::Value *metadata);

/// Given a metadata pointer, emit the callee for the given method.
FunctionPointer emitVirtualMethodValue(IRGenFunction &IGF,
                                       llvm::Value *metadata,
                                       PILDeclRef method,
                                       CanPILFunctionType methodType);

/// Given an instance pointer (or, for a static method, a class
/// pointer), emit the callee for the given method.
FunctionPointer emitVirtualMethodValue(IRGenFunction &IGF, llvm::Value *base,
                                       PILType baseType, PILDeclRef method,
                                       CanPILFunctionType methodType,
                                       bool useSuperVTable);

/// Is the given class known to have Swift-compatible metadata?
bool hasKnownPolarphpMetadata(IRGenModule &IGM, ClassDecl *theClass);

inline bool isKnownNotTaggedPointer(IRGenModule &IGM, ClassDecl *theClass) {
   // For now, assume any class type defined in Clang might be tagged.
   return hasKnownPolarphpMetadata(IGM, theClass);
}

/// Is the given class-like type known to have Polarphp-compatible
/// metadata?
bool hasKnownPolarphpMetadata(IRGenModule &IGM, CanType theType);

} // end namespace irgen
} // end namespace polar

#endif
