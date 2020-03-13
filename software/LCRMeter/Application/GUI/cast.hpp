#pragma once
#include <type_traits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"
#pragma GCC diagnostic ignored "-Wpedantic"

namespace {

template<typename Signature>
struct drop_first_argument;

template<typename R, typename F, typename ... A>
struct drop_first_argument<R (*)(F, A...)> {
	using type = R(A...);
};

template<typename R, typename F, typename ... A>
struct drop_first_argument<R (*const)(F, A...)> {
	using type = R(A...) const;
};

template<typename Signature, class C, typename drop_first_argument<Signature>::type C::*p>
struct pmf_cast;

template<class C, typename R, typename F, typename ...A, R (C::*p) (A...)>
struct pmf_cast<R (*)(F, A...), C, p> {
	static constexpr R (*cfn) (F t, A...a) = reinterpret_cast<R (*) (F t, A...a)>(p);

	static constexpr R (*toCfn()) (F t, A...a) { return reinterpret_cast<R (*) (F t, A...a)>(p); }
};

template<class C, typename R, typename F, typename ...A, R (C::*p) (A...) const>
struct pmf_cast<R (*const)(const F, A...), C, p> {
	static constexpr R (*cfn) (const F t, A...a) = reinterpret_cast<R (*) (const F t, A...a)>(p);

	static constexpr R (*toCfn()) (const F t, A...a) { return reinterpret_cast<R (*) (const F t, A...a)>(p); }
};

#pragma GCC diagnostic pop

}
