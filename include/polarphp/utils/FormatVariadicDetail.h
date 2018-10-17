// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/06/03.

#ifndef POLARPHP_UTILS_FORMAT_VARIADIC_DETAILS_H
#define POLARPHP_UTILS_FORMAT_VARIADIC_DETAILS_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/RawOutStream.h"
#include <type_traits>

namespace polar {
namespace utils {

template <typename T, typename Enable = void>
struct FormatProvider
{};

namespace internal {

class FormatAdapterImpl {
protected:
   virtual ~FormatAdapterImpl()
   {}

public:
   virtual void format(RawOutStream &stream, StringRef options) = 0;
};

template <typename T> class ProviderFormatAdapter : public FormatAdapterImpl
{
   T m_item;

public:
   explicit ProviderFormatAdapter(T &&item) : m_item(std::forward<T>(item))
   {}

   void format(polar::utils::RawOutStream &stream, StringRef options) override
   {
      FormatProvider<typename std::decay<T>::type>::format(m_item, stream, options);
   }
};

template <typename T>
class MissingFormatAdapter;

// Test if FormatProvider<T> is defined on T and contains a member function
// with the signature:
//   static void format(const T&, raw_stream &, StringRef);
//
template <class T> class HasFormatProvider
{
public:
   using Decayed = typename std::decay<T>::type;
   typedef void (*Signature_format)(const Decayed &, polar::utils::RawOutStream &,
                                    StringRef);

   template <typename U>
   static char test(polar::basic::SameType<Signature_format, &U::format> *);

   template <typename U> static double test(...);

   static bool const value =
         (sizeof(test<polar::utils::FormatProvider<Decayed>>(nullptr)) == 1);
};

// Simple template that decides whether a type T should use the member-function
// based format() invocation.
template <typename T>
struct UsesFormatMember
      : public std::integral_constant<
      bool,
      std::is_base_of<FormatAdapterImpl,
      typename std::remove_reference<T>::type>::value> {};

// Simple template that decides whether a type T should use the FormatProvider
// based format() invocation.  The member function takes priority, so this test
// will only be true if there is not ALSO a format member.
template <typename T>
struct uses_FormatProvider
      : public std::integral_constant<
      bool, !UsesFormatMember<T>::value && HasFormatProvider<T>::value> {
};

// Simple template that decides whether a type T has neither a member-function
// nor FormatProvider based implementation that it can use.  Mostly used so
// that the compiler spits out a nice diagnostic when a type with no format
// implementation can be located.
template <typename T>
struct UsesMissingProvider
      : public std::integral_constant<bool,
      !UsesFormatMember<T>::value &&
      !uses_FormatProvider<T>::value> {};

template <typename T>
typename std::enable_if<UsesFormatMember<T>::value, T>::type
build_format_adapter(T &&item)
{
   return std::forward<T>(item);
}

template <typename T>
typename std::enable_if<uses_FormatProvider<T>::value,
ProviderFormatAdapter<T>>::type
build_format_adapter(T &&item)
{
   return ProviderFormatAdapter<T>(std::forward<T>(item));
}

template <typename T>
typename std::enable_if<UsesMissingProvider<T>::value,
MissingFormatAdapter<T>>::type
build_format_adapter(T &&item)
{
   return MissingFormatAdapter<T>();
}

} // internal

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMAT_VARIADIC_DETAILS_H
