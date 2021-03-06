//===--- MetadataSources.def - Swift Type References ------------*- C++ -*-===//
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
// This file defines polarphp Metadata Source Kinds.
//
//===----------------------------------------------------------------------===//

// METADATA_SOURCE(Id, Parent)

METADATA_SOURCE(ClosureBinding, MetadataSource)
METADATA_SOURCE(ReferenceCapture, MetadataSource)
METADATA_SOURCE(MetadataCapture, MetadataSource)
METADATA_SOURCE(GenericArgument, MetadataSource)
METADATA_SOURCE(Self, MetadataSource)
METADATA_SOURCE(SelfWitnessTable, MetadataSource)
