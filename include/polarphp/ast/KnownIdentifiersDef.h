//===-- KnownIdentifiers.def - Known identifier metaprogramming -*- C++ -*-===//
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
// This file defines macros used for macro-metaprogramming with compiler-known
// identifiers.
//
//===----------------------------------------------------------------------===//

#ifndef IDENTIFIER_WITH_NAME
#  error Must define IDENTIFIER_WITH_NAME(Name, IdStr) before including this x-macro file
#endif

#define IDENTIFIER(name) IDENTIFIER_WITH_NAME(name, #name)
#define IDENTIFIER(name) IDENTIFIER_WITH_NAME(name, "_" #name)

IDENTIFIER(AllCases)
IDENTIFIER(allCases)
IDENTIFIER(alloc)
IDENTIFIER(allocWithZone)
IDENTIFIER(allZeros)
IDENTIFIER(Any)
IDENTIFIER(ArrayLiteralElement)
IDENTIFIER(atIndexedSubscript)
IDENTIFIER(buildBlock)
IDENTIFIER(buildDo)
IDENTIFIER(buildEither)
IDENTIFIER(buildIf)
IDENTIFIER(Change)
IDENTIFIER_WITH_NAME(code_, "_code")
IDENTIFIER(CodingKeys)
IDENTIFIER(combine)
IDENTIFIER(container)
IDENTIFIER(CoreGraphics)
IDENTIFIER(CoreMedia)
IDENTIFIER(CGFloat)
IDENTIFIER(CoreFoundation)
IDENTIFIER(CVarArg)
IDENTIFIER(Darwin)
IDENTIFIER(dealloc)
IDENTIFIER(Decodable)
IDENTIFIER(decode)
IDENTIFIER(decodeIfPresent)
IDENTIFIER(Decoder)
IDENTIFIER(decoder)
IDENTIFIER(dynamicallyCall)
IDENTIFIER(dynamicMember)
IDENTIFIER(Element)
IDENTIFIER(enclosingInstance)
IDENTIFIER(Encodable)
IDENTIFIER(encode)
IDENTIFIER(encodeIfPresent)
IDENTIFIER(Encoder)
IDENTIFIER(encoder)
IDENTIFIER(error)
IDENTIFIER(errorDomain)
IDENTIFIER(first)
IDENTIFIER(forKeyedSubscript)
IDENTIFIER(Foundation)
IDENTIFIER(for)
IDENTIFIER(forKey)
IDENTIFIER(from)
IDENTIFIER(fromRaw)
IDENTIFIER(hash)
IDENTIFIER(hasher)
IDENTIFIER(hashValue)
IDENTIFIER(initialize)
IDENTIFIER(initStorage)
IDENTIFIER(initialValue)
IDENTIFIER(into)
IDENTIFIER(intValue)
IDENTIFIER(Key)
IDENTIFIER(KeyedDecodingContainer)
IDENTIFIER(KeyedEncodingContainer)
IDENTIFIER(keyedBy)
IDENTIFIER(keyPath)
IDENTIFIER(makeIterator)
IDENTIFIER(Iterator)
IDENTIFIER(load)
IDENTIFIER(next)
IDENTIFIER(nsErrorDomain)
IDENTIFIER(objectAtIndexedSubscript)
IDENTIFIER(objectForKeyedSubscript)
IDENTIFIER(Optional)
IDENTIFIER(OptionalNilComparisonType)
IDENTIFIER(parameter)
IDENTIFIER(projected)
IDENTIFIER(projectedValue)
IDENTIFIER(Protocol)
IDENTIFIER(rawValue)
IDENTIFIER(RawValue)
IDENTIFIER(second)
IDENTIFIER(Selector)
IDENTIFIER(self)
IDENTIFIER(Self)
IDENTIFIER(setObject)
IDENTIFIER(simd)
IDENTIFIER(storage)
IDENTIFIER(stringValue)
IDENTIFIER(super)
IDENTIFIER(superDecoder)
IDENTIFIER(superEncoder)
IDENTIFIER_WITH_NAME(SwiftObject, "_TtCs12_SwiftObject")
IDENTIFIER(to)
IDENTIFIER(toRaw)
IDENTIFIER(Type)
IDENTIFIER(type)
IDENTIFIER(Value)
IDENTIFIER(value)
IDENTIFIER_WITH_NAME(value_, "_value")
IDENTIFIER(with)
IDENTIFIER(withArguments)
IDENTIFIER(withKeywordArguments)
IDENTIFIER(wrapped)
IDENTIFIER(wrappedValue)
IDENTIFIER(wrapperValue)

// Kinds of layout constraints
IDENTIFIER_WITH_NAME(UnknownLayout, "_UnknownLayout")
IDENTIFIER_WITH_NAME(TrivialLayout, "_Trivial")
IDENTIFIER_WITH_NAME(TrivialAtMostLayout, "_TrivialAtMost")
IDENTIFIER_WITH_NAME(RefCountedObjectLayout, "_RefCountedObject")
IDENTIFIER_WITH_NAME(NativeRefCountedObjectLayout, "_NativeRefCountedObject")
IDENTIFIER_WITH_NAME(ClassLayout, "_Class")
IDENTIFIER_WITH_NAME(NativeClassLayout, "_NativeClass")

// Operators
IDENTIFIER_WITH_NAME(MatchOperator, "~=")
IDENTIFIER_WITH_NAME(EqualsOperator, "==")
IDENTIFIER_WITH_NAME(derived_enum_equals, "__derived_enum_equals")
IDENTIFIER_WITH_NAME(derived_struct_equals, "__derived_struct_equals")

// Precedence groups
IDENTIFIER(AssignmentPrecedence)
IDENTIFIER(CastingPrecedence)
IDENTIFIER(DefaultPrecedence)
IDENTIFIER(FunctionArrowPrecedence)
IDENTIFIER(NilCoalescingPrecedence)
IDENTIFIER(TernaryPrecedence)

// Builtins and literals
IDENTIFIER(MaxBuiltinIntegerType)
IDENTIFIER(IntegerLiteralType)
IDENTIFIER(nilLiteral)
IDENTIFIER(integerLiteral)
IDENTIFIER(builtinIntegerLiteral)
IDENTIFIER(MaxBuiltinFloatType)
IDENTIFIER(FloatLiteralType)
IDENTIFIER(floatLiteral)
IDENTIFIER(builtinFloatLiteral)
IDENTIFIER(BooleanLiteralType)
IDENTIFIER(builtinBooleanLiteral)
IDENTIFIER(booleanLiteral)

IDENTIFIER(ExtendedGraphemeClusterLiteralType)
IDENTIFIER(builtinExtendedGraphemeClusterLiteral)
IDENTIFIER(extendedGraphemeClusterLiteral)

IDENTIFIER(UnicodeScalarLiteralType)
IDENTIFIER(builtinUnicodeScalarLiteral)
IDENTIFIER(unicodeScalarLiteral)

IDENTIFIER(stringLiteral)
IDENTIFIER(builtinStringLiteral)
IDENTIFIER(StringLiteralType)
IDENTIFIER(stringInterpolation)
IDENTIFIER(StringInterpolation)
IDENTIFIER(literalCapacity)
IDENTIFIER(interpolationCount)
IDENTIFIER(appendLiteral)
IDENTIFIER(appendInterpolation)
IDENTIFIER_WITH_NAME(dollarInterpolation, "$interpolation")
IDENTIFIER(arrayLiteral)
IDENTIFIER(dictionaryLiteral)
IDENTIFIER(className)

IDENTIFIER(ErrorType)
IDENTIFIER(Code)
IDENTIFIER(nsError)

#undef IDENTIFIER
#undef IDENTIFIER_
#undef IDENTIFIER_WITH_NAME
