#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <type_traits>
#include <utility>

#define CONTRACT_PP_CAT_IMPL(a, b) a##b
#define CONTRACT_PP_CAT(a, b) CONTRACT_PP_CAT_IMPL(a, b)
#define CONTRACT_PP_UNPAREN(...) __VA_ARGS__
#define CONTRACT_PP_FIRST(first, ...) first

// Preprocessor utilities below are just enough to keep the public DSL small.
// Max 32 field entries per CONTRACT(...) call (raised from 16 - see the
// CONTRACT_PP_FOR_EACH*/BOOL chains below, which must all be extended in
// lockstep with this). Per-field attribute count (PROPERTY_*/MAKE_ENTRY_*_*)
// is a separate, independent limit, still 16 - not raised, not requested.
#define CONTRACT_PP_NARG_IMPL( \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
    _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
    _30, _31, _32, N, ...) N

#define CONTRACT_PP_NARG(...) \
    CONTRACT_PP_NARG_IMPL(0 __VA_OPT__(,) __VA_ARGS__, \
        32, 31, 30, 29, 28, 27, 26, 25, 24, 23, \
        22, 21, 20, 19, 18, 17, 16, 15, 14, 13, \
        12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define CONTRACT_PP_BOOL_0 0
#define CONTRACT_PP_BOOL_1 1
#define CONTRACT_PP_BOOL_2 1
#define CONTRACT_PP_BOOL_3 1
#define CONTRACT_PP_BOOL_4 1
#define CONTRACT_PP_BOOL_5 1
#define CONTRACT_PP_BOOL_6 1
#define CONTRACT_PP_BOOL_7 1
#define CONTRACT_PP_BOOL_8 1
#define CONTRACT_PP_BOOL_9 1
#define CONTRACT_PP_BOOL_10 1
#define CONTRACT_PP_BOOL_11 1
#define CONTRACT_PP_BOOL_12 1
#define CONTRACT_PP_BOOL_13 1
#define CONTRACT_PP_BOOL_14 1
#define CONTRACT_PP_BOOL_15 1
#define CONTRACT_PP_BOOL_16 1
#define CONTRACT_PP_BOOL_17 1
#define CONTRACT_PP_BOOL_18 1
#define CONTRACT_PP_BOOL_19 1
#define CONTRACT_PP_BOOL_20 1
#define CONTRACT_PP_BOOL_21 1
#define CONTRACT_PP_BOOL_22 1
#define CONTRACT_PP_BOOL_23 1
#define CONTRACT_PP_BOOL_24 1
#define CONTRACT_PP_BOOL_25 1
#define CONTRACT_PP_BOOL_26 1
#define CONTRACT_PP_BOOL_27 1
#define CONTRACT_PP_BOOL_28 1
#define CONTRACT_PP_BOOL_29 1
#define CONTRACT_PP_BOOL_30 1
#define CONTRACT_PP_BOOL_31 1
#define CONTRACT_PP_BOOL_32 1
#define CONTRACT_PP_BOOL(value) CONTRACT_PP_CAT(CONTRACT_PP_BOOL_, value)

#define CONTRACT_PP_FOR_EACH(macro, ...) \
    CONTRACT_PP_CAT(CONTRACT_PP_FOR_EACH_, CONTRACT_PP_NARG(__VA_ARGS__))(macro, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_COMMA(macro, ...) \
    CONTRACT_PP_CAT(CONTRACT_PP_FOR_EACH_COMMA_, CONTRACT_PP_NARG(__VA_ARGS__))(macro, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_COMMA_ARG(macro, arg, ...) \
    CONTRACT_PP_CAT(CONTRACT_PP_FOR_EACH_COMMA_ARG_, CONTRACT_PP_NARG(__VA_ARGS__))(macro, arg, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_ARG(macro, arg, ...) \
    CONTRACT_PP_CAT(CONTRACT_PP_FOR_EACH_ARG_, CONTRACT_PP_NARG(__VA_ARGS__))(macro, arg, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_1(m, x) m(x)
#define CONTRACT_PP_FOR_EACH_2(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_1(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_3(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_2(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_4(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_3(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_5(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_4(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_6(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_5(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_7(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_6(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_8(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_7(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_9(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_8(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_10(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_9(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_11(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_10(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_12(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_11(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_13(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_12(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_14(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_13(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_15(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_14(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_16(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_15(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_17(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_16(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_18(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_17(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_19(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_18(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_20(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_19(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_21(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_20(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_22(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_21(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_23(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_22(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_24(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_23(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_25(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_24(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_26(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_25(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_27(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_26(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_28(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_27(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_29(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_28(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_30(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_29(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_31(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_30(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_32(m, x, ...) m(x) CONTRACT_PP_FOR_EACH_31(m, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_COMMA_1(m, x) m(x)
#define CONTRACT_PP_FOR_EACH_COMMA_2(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_1(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_3(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_2(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_4(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_3(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_5(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_4(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_6(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_5(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_7(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_6(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_8(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_7(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_9(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_8(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_10(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_9(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_11(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_10(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_12(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_11(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_13(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_12(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_14(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_13(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_15(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_14(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_16(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_15(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_17(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_16(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_18(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_17(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_19(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_18(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_20(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_19(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_21(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_20(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_22(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_21(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_23(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_22(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_24(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_23(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_25(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_24(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_26(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_25(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_27(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_26(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_28(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_27(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_29(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_28(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_30(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_29(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_31(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_30(m, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_32(m, x, ...) m(x), CONTRACT_PP_FOR_EACH_COMMA_31(m, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_COMMA_ARG_1(m, arg, x) m(arg, x)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_2(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_1(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_3(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_2(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_4(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_3(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_5(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_4(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_6(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_5(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_7(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_6(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_8(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_7(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_9(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_8(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_10(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_9(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_11(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_10(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_12(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_11(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_13(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_12(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_14(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_13(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_15(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_14(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_16(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_15(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_17(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_16(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_18(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_17(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_19(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_18(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_20(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_19(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_21(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_20(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_22(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_21(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_23(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_22(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_24(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_23(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_25(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_24(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_26(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_25(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_27(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_26(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_28(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_27(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_29(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_28(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_30(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_29(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_31(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_30(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_COMMA_ARG_32(m, arg, x, ...) m(arg, x), CONTRACT_PP_FOR_EACH_COMMA_ARG_31(m, arg, __VA_ARGS__)

#define CONTRACT_PP_FOR_EACH_ARG_1(m, arg, x) m(arg, x)
#define CONTRACT_PP_FOR_EACH_ARG_2(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_1(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_3(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_2(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_4(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_3(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_5(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_4(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_6(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_5(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_7(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_6(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_8(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_7(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_9(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_8(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_10(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_9(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_11(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_10(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_12(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_11(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_13(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_12(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_14(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_13(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_15(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_14(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_16(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_15(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_17(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_16(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_18(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_17(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_19(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_18(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_20(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_19(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_21(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_20(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_22(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_21(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_23(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_22(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_24(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_23(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_25(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_24(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_26(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_25(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_27(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_26(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_28(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_27(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_29(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_28(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_30(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_29(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_31(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_30(m, arg, __VA_ARGS__)
#define CONTRACT_PP_FOR_EACH_ARG_32(m, arg, x, ...) m(arg, x) CONTRACT_PP_FOR_EACH_ARG_31(m, arg, __VA_ARGS__)

// Field tuples are still the public contract syntax; these extract the name.
#define CONTRACT_PAIR_NAME(pair) CONTRACT_PAIR_NAME_IMPL pair
#define CONTRACT_PAIR_NAME_IMPL(name, id, ...) name

#define CONTRACT_STRINGIZE_IMPL(value) #value
#define CONTRACT_STRINGIZE(value) CONTRACT_STRINGIZE_IMPL(value)
#define CONTRACT_DESCRIBE_ATTRIBUTE(value) \
    ::contract::describe_attribute((value), CONTRACT_STRINGIZE(value))

#define BASE(type, offset) (CONTRACT_BASE_MARKER, type, offset)
#define REFERENCE(...) CONTRACT_PP_CAT(CONTRACT_REFERENCE_, CONTRACT_PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define CONTRACT_REFERENCE_2(name, id) (CONTRACT_REFERENCE_MARKER, name, id)
#define CONTRACT_REFERENCE_WITH_ATTRS(name, id, ...) \
    (CONTRACT_REFERENCE_MARKER, name, id, __VA_ARGS__)
#define CONTRACT_REFERENCE_3 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_4 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_5 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_6 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_7 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_8 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_9 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_10 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_11 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_12 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_13 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_14 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_15 CONTRACT_REFERENCE_WITH_ATTRS
#define CONTRACT_REFERENCE_16 CONTRACT_REFERENCE_WITH_ATTRS
// PROPERTY keeps the legacy 3-argument form and adds an attrs-bearing form.
#define PROPERTY(...) CONTRACT_PP_CAT(CONTRACT_PROPERTY_, CONTRACT_PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define CONTRACT_PROPERTY_3(name, id, type) (CONTRACT_PROPERTY_MARKER, name, id, type)
#define CONTRACT_PROPERTY_WITH_ATTRS(name, id, type, ...) \
    (CONTRACT_PROPERTY_MARKER, name, id, type, __VA_ARGS__)
#define CONTRACT_PROPERTY_4 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_5 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_6 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_7 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_8 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_9 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_10 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_11 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_12 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_13 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_14 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_15 CONTRACT_PROPERTY_WITH_ATTRS
#define CONTRACT_PROPERTY_16 CONTRACT_PROPERTY_WITH_ATTRS
#define ATTRS(...) (CONTRACT_ATTRS_MARKER, __VA_ARGS__)

#define CONTRACT_PP_SECOND(a, b, ...) b
#define CONTRACT_PP_PROBE() ~, 1
#define CONTRACT_PP_IS_PROBE(...) CONTRACT_PP_SECOND(__VA_ARGS__, 0)

#define CONTRACT_IS_BASE_MARKER(value) CONTRACT_PP_IS_PROBE(CONTRACT_PP_CAT(CONTRACT_IS_BASE_MARKER_, value))
#define CONTRACT_IS_BASE_MARKER_CONTRACT_BASE_MARKER CONTRACT_PP_PROBE()

#define CONTRACT_IS_PROPERTY_MARKER(value) CONTRACT_PP_IS_PROBE(CONTRACT_PP_CAT(CONTRACT_IS_PROPERTY_MARKER_, value))
#define CONTRACT_IS_PROPERTY_MARKER_CONTRACT_PROPERTY_MARKER CONTRACT_PP_PROBE()

#define CONTRACT_IS_REFERENCE_MARKER(value) CONTRACT_PP_IS_PROBE(CONTRACT_PP_CAT(CONTRACT_IS_REFERENCE_MARKER_, value))
#define CONTRACT_IS_REFERENCE_MARKER_CONTRACT_REFERENCE_MARKER CONTRACT_PP_PROBE()

#define CONTRACT_IS_ATTRS_MARKER(value) CONTRACT_PP_IS_PROBE(CONTRACT_PP_CAT(CONTRACT_IS_ATTRS_MARKER_, value))
#define CONTRACT_IS_ATTRS_MARKER_CONTRACT_ATTRS_MARKER CONTRACT_PP_PROBE()

// Every entry starts with a marker so the DSL can remain a single tuple form.
#define CONTRACT_ENTRY_HEAD(entry) CONTRACT_ENTRY_HEAD_IMPL entry
#define CONTRACT_ENTRY_HEAD_IMPL(first, ...) first

// Entry detection is marker-based, so the same tuple syntax can carry attrs.
#define CONTRACT_ENTRY_IS_BASE(entry) CONTRACT_IS_BASE_MARKER(CONTRACT_ENTRY_HEAD(entry))
#define CONTRACT_ENTRY_IS_PROPERTY(entry) CONTRACT_IS_PROPERTY_MARKER(CONTRACT_ENTRY_HEAD(entry))
#define CONTRACT_ENTRY_IS_REFERENCE(entry) CONTRACT_IS_REFERENCE_MARKER(CONTRACT_ENTRY_HEAD(entry))
#define CONTRACT_ENTRY_IS_ATTRS(entry) CONTRACT_IS_ATTRS_MARKER(CONTRACT_ENTRY_HEAD(entry))
#define CONTRACT_ENTRY_KIND(entry) \
    CONTRACT_PP_CAT(CONTRACT_ENTRY_KIND_BASE_, CONTRACT_ENTRY_IS_BASE(entry))(entry)

#define CONTRACT_ENTRY_KIND_BASE_1(entry) 1
#define CONTRACT_ENTRY_KIND_BASE_0(entry) \
    CONTRACT_PP_CAT(CONTRACT_ENTRY_KIND_REFERENCE_, CONTRACT_ENTRY_IS_REFERENCE(entry))(entry)

#define CONTRACT_ENTRY_KIND_REFERENCE_1(entry) 2
#define CONTRACT_ENTRY_KIND_REFERENCE_0(entry) \
    CONTRACT_PP_CAT(CONTRACT_ENTRY_KIND_PROPERTY_, CONTRACT_ENTRY_IS_PROPERTY(entry))(entry)

#define CONTRACT_ENTRY_KIND_PROPERTY_1(entry) 3
#define CONTRACT_ENTRY_KIND_PROPERTY_0(entry) 0

#define CONTRACT_BASE_TYPE(entry) CONTRACT_BASE_TYPE_IMPL entry
#define CONTRACT_BASE_TYPE_IMPL(marker, type, offset) type

#define CONTRACT_BASE_OFFSET(entry) CONTRACT_BASE_OFFSET_IMPL entry
#define CONTRACT_BASE_OFFSET_IMPL(marker, type, offset) offset

#define CONTRACT_PROPERTY_NAME(entry) CONTRACT_PROPERTY_NAME_IMPL entry
#define CONTRACT_PROPERTY_NAME_IMPL(marker, name, id, type, ...) name

#define CONTRACT_REFERENCE_NAME(entry) CONTRACT_REFERENCE_NAME_IMPL entry
#define CONTRACT_REFERENCE_NAME_IMPL(marker, name, id, ...) name

#define CONTRACT_ATTRS_VALUES(entry) CONTRACT_ATTRS_VALUES_IMPL entry
#define CONTRACT_ATTRS_VALUES_IMPL(marker, ...) __VA_ARGS__

#define CONTRACT_MAKE_FIELD_ATTRIBUTES(...) \
    ::contract::make_field_attributes( \
        __VA_OPT__(CONTRACT_PP_FOR_EACH_COMMA(CONTRACT_DESCRIBE_ATTRIBUTE, __VA_ARGS__)))

#define CONTRACT_MAKE_ENTRY(contract_self_type, entry) \
    CONTRACT_PP_CAT(CONTRACT_MAKE_ENTRY_, CONTRACT_ENTRY_KIND(entry))(contract_self_type, entry)

#define CONTRACT_DECLARE_FIELD(contract_self_type, entry) \
    CONTRACT_PP_CAT(CONTRACT_DECLARE_FIELD_, CONTRACT_ENTRY_KIND(entry))(contract_self_type, entry)

#define CONTRACT_DECLARE_FIELD_0(contract_self_type, pair) \
    CONTRACT_DECLARE_FIELD_0_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN pair)

#define CONTRACT_DECLARE_FIELD_0_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_DECLARE_FIELD_0_, CONTRACT_PP_NARG(__VA_ARGS__))(contract_self_type, __VA_ARGS__)

#define CONTRACT_DECLARE_FIELD_0_BUILD(contract_self_type, name, id, ...) \
    using name = decltype(::contract::make_member_field<contract_self_type, id, &contract_self_type::name>( \
        CONTRACT_STRINGIZE(name), CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__)));

#define CONTRACT_DECLARE_FIELD_0_2(contract_self_type, name, id) \
    CONTRACT_DECLARE_FIELD_0_BUILD(contract_self_type, name, id)
#define CONTRACT_DECLARE_FIELD_0_WITH_ATTRS(contract_self_type, name, id, ...) \
    CONTRACT_DECLARE_FIELD_0_BUILD(contract_self_type, name, id, __VA_ARGS__)
#define CONTRACT_DECLARE_FIELD_0_3 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_4 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_5 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_6 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_7 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_8 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_9 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_10 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_11 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_12 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_13 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_14 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_15 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS
#define CONTRACT_DECLARE_FIELD_0_16 CONTRACT_DECLARE_FIELD_0_WITH_ATTRS

#define CONTRACT_DECLARE_FIELD_1(contract_self_type, entry)

#define CONTRACT_DECLARE_FIELD_2(contract_self_type, entry) \
    CONTRACT_DECLARE_REFERENCE_FIELD(contract_self_type, entry)

#define CONTRACT_DECLARE_REFERENCE_FIELD(contract_self_type, entry) \
    CONTRACT_DECLARE_REFERENCE_FIELD_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN entry)

#define CONTRACT_DECLARE_REFERENCE_FIELD_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_DECLARE_REFERENCE_FIELD_, CONTRACT_PP_NARG(__VA_ARGS__))(contract_self_type, __VA_ARGS__)

#define CONTRACT_DECLARE_REFERENCE_FIELD_BUILD(contract_self_type, name, id, ...) \
    struct name : ::contract::field< \
        contract_self_type, id, ::contract::field_kind::reference, \
        decltype(CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__)), \
        std::remove_cvref_t<decltype(contract_self_type::name)>, nullptr, name> { \
        using base_type = ::contract::field< \
            contract_self_type, id, ::contract::field_kind::reference, \
            decltype(CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__)), \
            std::remove_cvref_t<decltype(contract_self_type::name)>, nullptr, name>; \
        using base_type::base_type; \
        template<class Obj> \
        static constexpr decltype(auto) contract_ref_get(Obj& obj) { return (obj.name); } \
        template<class Obj> \
        static constexpr decltype(auto) contract_ref_get(const Obj& obj) { return (obj.name); } \
        template<class Obj, class Value> \
        static constexpr void contract_ref_set(Obj& obj, Value&& value) \
            requires std::is_assignable_v<decltype((std::declval<Obj&>().name)), Value&&> { \
            obj.name = std::forward<Value>(value); \
        } \
    };

#define CONTRACT_DECLARE_REFERENCE_FIELD_3(contract_self_type, marker, name, id) \
    CONTRACT_DECLARE_REFERENCE_FIELD_BUILD(contract_self_type, name, id)
#define CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS(contract_self_type, marker, name, id, ...) \
    CONTRACT_DECLARE_REFERENCE_FIELD_BUILD(contract_self_type, name, id, __VA_ARGS__)
#define CONTRACT_DECLARE_REFERENCE_FIELD_4 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_5 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_6 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_7 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_8 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_9 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_10 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_11 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_12 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_13 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_14 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_15 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_REFERENCE_FIELD_16 CONTRACT_DECLARE_REFERENCE_FIELD_WITH_ATTRS

#define CONTRACT_DECLARE_FIELD_3(contract_self_type, entry) \
    CONTRACT_DECLARE_PROPERTY_FIELD(contract_self_type, entry)

#define CONTRACT_DECLARE_PROPERTY_FIELD(contract_self_type, entry) \
    CONTRACT_DECLARE_PROPERTY_FIELD_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN entry)
#define CONTRACT_DECLARE_PROPERTY_FIELD_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_DECLARE_PROPERTY_FIELD_, CONTRACT_PP_NARG(__VA_ARGS__))(contract_self_type, __VA_ARGS__)
#define CONTRACT_DECLARE_PROPERTY_FIELD_4(contract_self_type, marker, name, id, type) \
    using name = decltype(::contract::make_property_field<contract_self_type, id, type>( \
        CONTRACT_STRINGIZE(name), CONTRACT_MAKE_FIELD_ATTRIBUTES()));
#define CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS(contract_self_type, marker, name, id, type, ...) \
    using name = decltype(::contract::make_property_field<contract_self_type, id, type>( \
        CONTRACT_STRINGIZE(name), CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__)));
#define CONTRACT_DECLARE_PROPERTY_FIELD_5 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_6 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_7 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_8 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_9 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_10 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_11 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_12 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_13 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_14 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_15 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS
#define CONTRACT_DECLARE_PROPERTY_FIELD_16 CONTRACT_DECLARE_PROPERTY_FIELD_WITH_ATTRS

#define CONTRACT_MAKE_ENTRY_0(contract_self_type, pair) \
    CONTRACT_MAKE_ENTRY_0_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN pair)

#define CONTRACT_MAKE_ENTRY_0_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_MAKE_ENTRY_0_, CONTRACT_PP_NARG(__VA_ARGS__))( \
        contract_self_type, __VA_ARGS__)

#define CONTRACT_MAKE_ENTRY_0_2(contract_self_type, name, id) \
    CONTRACT_MAKE_ENTRY_0_BUILD(contract_self_type, name, id)

#define CONTRACT_MAKE_ENTRY_0_WITH_ATTRS(contract_self_type, name, id, ...) \
    CONTRACT_MAKE_ENTRY_0_BUILD(contract_self_type, name, id, __VA_ARGS__)

#define CONTRACT_MAKE_ENTRY_0_BUILD(contract_self_type, name, id, ...) \
    ::contract::make_member_field<contract_self_type, id, &contract_self_type::name>( \
            CONTRACT_STRINGIZE(name), \
            CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__))

#define CONTRACT_MAKE_ENTRY_0_3 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_4 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_5 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_6 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_7 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_8 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_9 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_10 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_11 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_12 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_13 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_14 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_15 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_0_16 CONTRACT_MAKE_ENTRY_0_WITH_ATTRS

#define CONTRACT_MAKE_ENTRY_1(contract_self_type, entry) \
    ::contract::base<CONTRACT_BASE_TYPE(entry), CONTRACT_BASE_OFFSET(entry)>{}

#define CONTRACT_MAKE_ENTRY_2(contract_self_type, entry) \
    CONTRACT_MAKE_REFERENCE_ENTRY_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN entry)

#define CONTRACT_MAKE_REFERENCE_ENTRY_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_MAKE_REFERENCE_ENTRY_, CONTRACT_PP_NARG(__VA_ARGS__))(contract_self_type, __VA_ARGS__)

#define CONTRACT_MAKE_REFERENCE_ENTRY_3(contract_self_type, marker, name, id) \
    typename contract_fields::name{CONTRACT_STRINGIZE(name), CONTRACT_MAKE_FIELD_ATTRIBUTES()}

#define CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS(contract_self_type, marker, name, id, ...) \
    typename contract_fields::name{CONTRACT_STRINGIZE(name), CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__)}

#define CONTRACT_MAKE_REFERENCE_ENTRY_4 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_5 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_6 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_7 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_8 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_9 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_10 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_11 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_12 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_13 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_14 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_15 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS
#define CONTRACT_MAKE_REFERENCE_ENTRY_16 CONTRACT_MAKE_REFERENCE_ENTRY_WITH_ATTRS

// Property entries are still descriptors, not storage. Their value access is
// resolved by hooks in the field layer.
#define CONTRACT_MAKE_ENTRY_3(contract_self_type, entry) \
    CONTRACT_MAKE_ENTRY_2_EXPAND(contract_self_type, CONTRACT_PP_UNPAREN entry)

#define CONTRACT_MAKE_ENTRY_2_EXPAND(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_MAKE_ENTRY_2_, CONTRACT_PP_NARG(__VA_ARGS__))( \
        contract_self_type, __VA_ARGS__)

#define CONTRACT_MAKE_ENTRY_2_4(contract_self_type, marker, name, id, type) \
    ::contract::make_property_field<contract_self_type, id, type>( \
            CONTRACT_STRINGIZE(name), \
            CONTRACT_MAKE_FIELD_ATTRIBUTES())

#define CONTRACT_MAKE_ENTRY_2_WITH_ATTRS(contract_self_type, marker, name, id, type, ...) \
    ::contract::make_property_field<contract_self_type, id, type>( \
            CONTRACT_STRINGIZE(name), \
            CONTRACT_MAKE_FIELD_ATTRIBUTES(__VA_ARGS__))

#define CONTRACT_MAKE_ENTRY_2_5 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_6 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_7 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_8 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_9 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_10 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_11 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_12 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_13 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_14 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_15 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS
#define CONTRACT_MAKE_ENTRY_2_16 CONTRACT_MAKE_ENTRY_2_WITH_ATTRS

#define CONTRACT_DEFINE(contract_self_type, make_contract_expression, ...) \
    struct contract_fields { \
        CONTRACT_PP_FOR_EACH_ARG(CONTRACT_DECLARE_FIELD, contract_self_type, __VA_ARGS__) \
    }; \
    friend constexpr auto contract_definition(::contract::tag<contract_self_type>) { \
        return make_contract_expression( \
            CONTRACT_STRINGIZE(contract_self_type), \
            CONTRACT_PP_FOR_EACH_COMMA_ARG(CONTRACT_MAKE_ENTRY, contract_self_type, __VA_ARGS__)); \
    }

#define CONTRACT_IMPL_0(contract_self_type, ...) \
    CONTRACT_DEFINE( \
        contract_self_type, \
        ::contract::make_contract<contract_self_type>, \
        __VA_ARGS__)

// The first entry decides whether the contract has a top-level ATTRS(...) pack.
#define CONTRACT_DISPATCH(contract_self_type, has_attributes, ...) \
    CONTRACT_PP_CAT(CONTRACT_DISPATCH_, has_attributes)(contract_self_type, __VA_ARGS__)

// The first entry decides whether the contract starts with ATTRS(...).
#define CONTRACT_DISPATCH_0(contract_self_type, ...) \
    CONTRACT_IMPL_0(contract_self_type, __VA_ARGS__)

#define CONTRACT_DISPATCH_1(contract_self_type, attrs_entry, ...) \
    CONTRACT_DEFINE_WITH_ATTRIBUTES( \
        contract_self_type, \
        attrs_entry, \
        __VA_ARGS__)

#define CONTRACT_DEFINE_WITH_ATTRIBUTES(contract_self_type, attrs_entry, ...) \
    struct contract_fields { \
        CONTRACT_PP_FOR_EACH_ARG(CONTRACT_DECLARE_FIELD, contract_self_type, __VA_ARGS__) \
    }; \
    friend constexpr auto contract_definition(::contract::tag<contract_self_type>) { \
        return ::contract::make_contract_with_attributes<contract_self_type>( \
            CONTRACT_STRINGIZE(contract_self_type), \
            ::contract::make_contract_attributes( \
                CONTRACT_PP_FOR_EACH_COMMA( \
                    CONTRACT_DESCRIBE_ATTRIBUTE, \
                    CONTRACT_ATTRS_VALUES(attrs_entry))), \
            CONTRACT_PP_FOR_EACH_COMMA_ARG(CONTRACT_MAKE_ENTRY, contract_self_type, __VA_ARGS__)); \
    }

#define CONTRACT_CONTRACT_0(contract_self_type, ...) \
    friend constexpr auto contract_definition(::contract::tag<contract_self_type>) { \
        return ::contract::make_contract<contract_self_type>( \
            CONTRACT_STRINGIZE(contract_self_type)); \
    }

#define CONTRACT_CONTRACT_1(contract_self_type, ...) \
    CONTRACT_DISPATCH( \
        contract_self_type, \
        CONTRACT_ENTRY_IS_ATTRS(CONTRACT_PP_FIRST(__VA_ARGS__)), \
        __VA_ARGS__)

#define CONTRACT(contract_self_type, ...) \
    CONTRACT_PP_CAT(CONTRACT_CONTRACT_, CONTRACT_PP_BOOL(CONTRACT_PP_NARG(__VA_ARGS__)))( \
        contract_self_type, __VA_ARGS__)
