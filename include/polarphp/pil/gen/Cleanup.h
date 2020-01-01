//===--- Cleanup.h - Declarations for PIL Cleanup Generation ----*- C++ -*-===//
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
// This file defines the Cleanup and CleanupManager classes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_GEN_CLEANUP_H
#define POLARPHP_PIL_GEN_CLEANUP_H

#include "polarphp/basic/DiverseStack.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

class PILBasicBlock;
class PILFunction;
class PILValue;

namespace lowering {

class RValue;
class JumpDest;
class PILGenFunction;
class PILGenBuilder;
class ManagedValue;
class Scope;
class SharedBorrowFormalAccess;
class FormalEvaluationScope;

/// Is a cleanup being executed as a result of some sort of forced
/// unwinding, such as an error being thrown, or are we just cleaning up
/// after some operation?
///
/// Most cleanups don't care, but the cleanups tied to l-value accesses do:
/// the access will be aborted rather than ended normally, which may cause
/// e.g. writebacks to be skipped.  It is also important that no actions
/// be undertaken by an unwind cleanup that might change control flow,
/// such as throwing an error.  In contrast, non-unwinding cleanups are
/// permitted to change control flow.
enum ForUnwind_t : bool {
   NotForUnwind,
   IsForUnwind
};

/// The valid states that a cleanup can be in.
enum class CleanupState {
   /// The cleanup is inactive but may be activated later.
      Dormant,

   /// The cleanup is inactive and will not be activated later.
      Dead,

   // Only active states after this point

   /// The cleanup is currently active.
      Active,

   /// The cleanup is currently active.  When it's forwarded, it should
   /// be placed in a dormant state, not a dead state.
      PersistentlyActive
};

llvm::raw_ostream &operator<<(raw_ostream &os, CleanupState state);

class LLVM_LIBRARY_VISIBILITY Cleanup {
   friend class CleanupManager;

   unsigned allocatedSize;

   CleanupState state;

protected:
   Cleanup() {}
   virtual ~Cleanup() {}

public:
   /// Return the allocated size of this object.  This is required by
   /// DiverseStack for iteration.
   size_t allocated_size() const { return allocatedSize; }

   CleanupState getState() const { return state; }
   virtual void setState(PILGenFunction &SGF, CleanupState newState) {
      state = newState;
   }
   bool isActive() const { return state >= CleanupState::Active; }
   bool isDead() const { return state == CleanupState::Dead; }

   virtual void emit(PILGenFunction &SGF, CleanupLocation loc,
                     ForUnwind_t forUnwind) = 0;
   virtual void dump(PILGenFunction &SGF) const = 0;
};

/// A cleanup depth is generally used to denote the set of cleanups
/// between the given cleanup (and not including it) and the top of
/// the stack.
///
/// Cleanup depths can be the stack's stable_end(), but generally
/// cannot be invalid.
typedef DiverseStackImpl<Cleanup>::stable_iterator CleanupsDepth;

/// A cleanup handle is a stable pointer to a single cleanup.
///
/// Cleanup handles can be invalid() (if no cleanup was required), but
/// generally cannot be the stack's stable_end().
typedef DiverseStackImpl<Cleanup>::stable_iterator CleanupHandle;

class LLVM_LIBRARY_VISIBILITY CleanupManager {
   friend class Scope;

   PILGenFunction &SGF;

   /// Stack - Currently active cleanups in this scope tree.
   DiverseStack<Cleanup, 128> stack;

   /// The shallowest depth held by an active Scope object.
   ///
   /// Generally, the rule is that a CleanupHandle is invalidated as
   /// soon as the underlying cleanup is marked dead, meaning that
   /// further uses of that handle are free to misbehave, and therefore
   /// that we're free to actually pop the cleanup.  But doing so might
   /// break any outstanding CleanupsDepths we've vended, of which we
   /// only really care about those held by the Scope RAII objects.  So
   /// we can only reap the cleanup stack up to the innermost depth
   /// that we've handed out as a Scope.
   Scope *innermostScope = nullptr;
   FormalEvaluationScope *innermostFormalScope = nullptr;

   void popTopDeadCleanups();
   void emitCleanups(CleanupsDepth depth, CleanupLocation l,
                     ForUnwind_t forUnwind, bool popCleanups);
   void endScope(CleanupsDepth depth, CleanupLocation l);

   Cleanup &initCleanup(Cleanup &cleanup, size_t allocSize, CleanupState state);
   void setCleanupState(Cleanup &cleanup, CleanupState state);

   friend class CleanupStateRestorationScope;
   friend class SharedBorrowFormalEvaluation;
   friend class FormalEvaluationScope;

public:
   CleanupManager(PILGenFunction &SGF)
      : SGF(SGF) {}

   /// Return a stable reference to the last cleanup pushed.
   CleanupsDepth getCleanupsDepth() const { return stack.stable_begin(); }

   /// Return a stable reference to the last cleanup pushed.
   CleanupHandle getTopCleanup() const {
      assert(!stack.empty());
      return stack.stable_begin();
   }

   Cleanup &getCleanup(CleanupHandle iter) {
      return *stack.find(iter);
   }

   Cleanup &findAndAdvance(CleanupsDepth &iter) {
      return stack.findAndAdvance(iter);
   }

   /// Emit a branch to the given jump destination,
   /// threading out through any cleanups we need to run. This does not pop the
   /// cleanup stack.
   ///
   /// \param dest       The destination scope and block.
   /// \param branchLoc  The location of the branch instruction.
   /// \param args       Arguments to pass to the destination block.
   void emitBranchAndCleanups(JumpDest dest, PILLocation branchLoc,
                              ArrayRef<PILValue> args = {},
                              ForUnwind_t forUnwind = NotForUnwind);

   /// emitCleanupsForReturn - Emit the top-level cleanups needed prior to a
   /// return from the function.
   void emitCleanupsForReturn(CleanupLocation loc, ForUnwind_t forUnwind);

   /// Emit a new block that jumps to the specified location and runs necessary
   /// cleanups based on its level.  If there are no cleanups to run, this just
   /// returns the dest block.
   PILBasicBlock *emitBlockForCleanups(JumpDest dest, PILLocation branchLoc,
                                       ArrayRef<PILValue> args = {},
                                       ForUnwind_t forUnwind = NotForUnwind);

   /// pushCleanup - Push a new cleanup.
   template<class T, class... A>
   T &pushCleanupInState(CleanupState state,
                         A &&... args) {
      assert(state != CleanupState::Dead);

#ifndef NDEBUG
      CleanupsDepth oldTop = stack.stable_begin();
#endif

      T &cleanup = stack.push<T, A...>(::std::forward<A>(args)...);
      T &result = static_cast<T&>(initCleanup(cleanup, sizeof(T), state));

#ifndef NDEBUG
      auto newTop = stack.begin();
      ++newTop;
      assert(newTop == stack.find(oldTop));
#endif
      return result;
   }
   template<class T, class... A>
   T &pushCleanup(A &&... args) {
      return pushCleanupInState<T, A...>(CleanupState::Active,
                                         ::std::forward<A>(args)...);
   }

   /// Emit the given active cleanup now and transition it to being inactive.
   void popAndEmitCleanup(CleanupHandle handle, CleanupLocation loc,
                          ForUnwind_t forUnwind);

   /// Transition the given active cleanup to the corresponding
   /// inactive state: Active becomes Dead and PersistentlyActive
   /// becomes Dormant.
   void forwardCleanup(CleanupHandle depth);

   /// Set the state of the cleanup at the given depth.
   /// The transition must be non-trivial and legal.
   void setCleanupState(CleanupHandle depth, CleanupState state);

   /// True if there are any active cleanups in the scope between the two
   /// cleanup handles.
   bool hasAnyActiveCleanups(CleanupsDepth from, CleanupsDepth to);

   /// True if there are any active cleanups in the scope between the specified
   /// cleanup handle and the current top of stack.
   bool hasAnyActiveCleanups(CleanupsDepth from);

   /// Dump the output of each cleanup on this stack.
   void dump() const;

   /// Dump the given cleanup handle if it is on the current stack.
   void dump(CleanupHandle handle) const;

   /// Verify that the given cleanup handle is valid.
   void checkIterator(CleanupHandle handle) const;
};

/// An RAII object that allows the state of a cleanup to be
/// temporarily modified.
class CleanupStateRestorationScope {
   CleanupManager &cleanups;
   llvm::SmallVector<std::pair<CleanupHandle, CleanupState>, 4> savedStates;

   CleanupStateRestorationScope(const CleanupStateRestorationScope &) = delete;
   CleanupStateRestorationScope &
   operator=(const CleanupStateRestorationScope &) = delete;
public:
   CleanupStateRestorationScope(CleanupManager &cleanups) : cleanups(cleanups) {}

   /// Set the state of the given cleanup and remember what we set it to.
   void pushCleanupState(CleanupHandle handle, CleanupState newState);

   /// Just remember whatever the current state of the given cleanup is.
   void pushCurrentCleanupState(CleanupHandle handle);

   void pop() &&;

   ~CleanupStateRestorationScope() { popImpl(); }

private:
   void popImpl();
};

class CleanupCloner {
   PILGenFunction &SGF;
   bool hasCleanup;
   bool isLValue;

public:
   CleanupCloner(PILGenFunction &SGF, const ManagedValue &mv);
   CleanupCloner(PILGenBuilder &builder, const ManagedValue &mv);
   CleanupCloner(PILGenFunction &SGF, const RValue &rv);
   CleanupCloner(PILGenBuilder &builder, const RValue &rv);

   ManagedValue clone(PILValue value) const;
};

} // end namespace lowering
} // end namespace polar

#endif
