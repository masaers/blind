#ifndef COM_MASAERS_BLIND_BLIND_HPP
#define COM_MASAERS_BLIND_BLIND_HPP
#include <tuple>
#include <functional>
#include <iostream>

namespace com { namespace masaers { namespace blind {

  template<typename, typename...> class blind_t;
  /**
  This is the magic function that you will use to (dazzle and) blind.
  */
  template<typename Func, typename... Args>
  inline auto blind(Func&& func, Args&&... args) {
    return blind_t<Func, Args...>(std::forward<Func>(func), std::forward<Args>(args)...);
  }

  /**
  This macro takes a collection of functions with the same name and
  produces a single lambda closure that can be used to call any of them.
  */
  #define BLIND_FUNC(func_name) [](auto&&... x) -> decltype(auto) { return func_name (std::forward<decltype(x)>(x)...); }



  namespace detail {
    // Correct template instantiateion requires reference_wrappers
    // to be unwrapped.
    template<typename T> inline constexpr T& unwrap(const std::reference_wrapper<T>&  x) { return x.get(); }
    template<typename T> inline constexpr T& unwrap(      std::reference_wrapper<T>&  x) { return x.get(); }
    template<typename T> inline constexpr T& unwrap(      std::reference_wrapper<T>&& x) { return x.get(); }
    template<typename T> inline constexpr T  unwrap(                             T && x) { return std::forward<T>(x); }

    // Predicate (helper) to determine the offset of a bound argument
    // in the final argument list.
    template<std::size_t I, typename T, std::size_t Offset, std::size_t Size> struct _arg_offset
    : std::conditional<std::is_placeholder<T>::value != 0,
                       std::integral_constant<std::size_t, Offset + std::is_placeholder<T>::value - 1>,
                       std::integral_constant<std::size_t, I>
                       >::type
    {
      static_assert(std::is_placeholder<T>::value <= Size,
                    "Placeholder outside provided arguments!");
    };
    template<std::size_t I, typename Tuple, std::size_t Size> struct arg_offset
    : _arg_offset<I,
                  typename std::tuple_element<I, Tuple>::type,
                  std::tuple_size<Tuple>::value,
                  Size>
    {};

    // Predicate to determine whether a provided argument has been bound
    // by a place holder, and should be used in place of a bound argument.
    template<std::size_t, typename...> struct placeheld_in;
    template<std::size_t I> struct placeheld_in<I> : std::false_type {};
    template<std::size_t I, typename H, typename... Rest>
    struct placeheld_in<I, H, Rest...>
    : std::integral_constant<bool, std::is_placeholder<H>::value - 1 == I || placeheld_in<I, Rest...>::value>
    {};
    // The _arg_seq predicate figures out the list of arguments to a
    // blind function call as offsets into the concatenated range of
    // bound (N0, T0...) and provided (N1, T1...) arguments. It
    // builds the list from right to left.
    template<typename, typename, std::size_t, std::size_t, std::size_t...> struct _arg_seq;
    // Starting with the provided arguments, we need to ensure that
    // only arguments that haven't been placeheld by bound arguments
    // are added; luckily, we know that each of the bound arguments
    // will take one spot, so the offset is trivial.
    template<std::size_t N0, std::size_t N1, std::size_t... S, typename... T0, typename... T1>
    struct _arg_seq<std::tuple<T0...>, std::tuple<T1...>, N0, N1, S...>
    : std::conditional<placeheld_in<N1 - 1, T0...>::value,
                       _arg_seq<std::tuple<T0...>,
                                std::tuple<T1...>,
                                N0,
                                N1 - 1,
                                S...>,
                       _arg_seq<std::tuple<T0...>,
                                std::tuple<T1...>,
                                N0,
                                N1 - 1,
                                N0 + N1 - 1, S...>
                       >::type
    {};
    // Continuing with the bound arguments, we need to ensure that
    // placeholders are correctly identified, which is what we use
    // the arg_offset predicate to do for us.
    template<std::size_t N0, std::size_t... S, typename... T0, typename... T1>
    struct _arg_seq<std::tuple<T0...>, std::tuple<T1...>, N0, 0, S...>
    : _arg_seq<std::tuple<T0...>,
               std::tuple<T1...>,
               N0 - 1,
               0,
               arg_offset<N0 - 1, std::tuple<T0...>, sizeof...(T1) >::value, S...>
    {};
    // Finally, the base case with two exhaused lists merely sets
    // the correct index_sequence as the calcualted type.
    template<std::size_t... S, typename... T0, typename... T1>
    struct _arg_seq<std::tuple<T0...>, std::tuple<T1...>, 0, 0, S...> {
      typedef std::index_sequence<S...> type;
    };

    // Since the _arg_seq predicate is a bit clunky to use, this nicer
    // version is provided that derives the correct _arg_seq to use, and
    // inherits from the calculated index_sequence.
    template<typename T0, typename... T1>
    struct arg_seq : public _arg_seq<typename std::decay<T0>::type,
                                     std::tuple<T1...>,
                                     std::tuple_size<typename std::decay<T0>::type>::value,
                                     sizeof...(T1)
                                     >::type
    {};

    // Applies a tuple of arguments to a provided function in a given order.
    template<typename Func, typename Tuple, std::size_t... I>
    inline constexpr decltype(auto) call_with(Func&& f, Tuple&& t, std::index_sequence<I...>) {
      return f(unwrap(std::get<I>(std::forward<Tuple>(t)))...);
    }

    // Merge a tuple of arguments possibly containing place holders with
    // a list of additional arguments into an effective argument list and
    // apply that list to a function.
    template<typename Func, typename Tuple, typename... Args, std::size_t... I>
    inline constexpr decltype(auto)
    call_with_merged_args(Func&& f, Tuple&& t, std::index_sequence<I...>, Args&&... args) {
      return call_with(unwrap(std::forward<Func>(f)),
                       std::forward_as_tuple(std::get<I>(t)..., std::forward<Args>(args)...),
                       arg_seq<Tuple, Args...>());
    }
  } // namespace detail
  
  
  /**
  This class describes late bound functions. Use `blind` to create an
  instance of it.
  */
  template<typename Func, typename... Args>
  class blind_t {
  protected:
    typedef typename std::decay<Func>::type func_type;
    typedef std::tuple<typename std::decay<Args>::type...> args_type;
    func_type func_m;
    args_type args_m;
  public:
    inline blind_t(Func&& func, Args&&... args)
      : func_m(std::forward<Func>(func))
      , args_m(std::forward<Args>(args)...)
    {}
    inline blind_t(const blind_t&) = default;
    inline blind_t(blind_t&&) = default;
    inline blind_t& operator=(const blind_t&) = default;
    inline blind_t& operator=(blind_t&&) = default;
    template<typename... LateArgs>
    inline decltype(auto) operator()(LateArgs&&... late_args) const {
      return detail::call_with_merged_args(func_m,
                                           args_m,
                                           std::make_index_sequence<sizeof...(Args)>(),
                                           std::forward<LateArgs>(late_args)...);
    }
    template<typename... LateArgs>
    inline decltype(auto) operator()(LateArgs&&... late_args) {
      return detail::call_with_merged_args(func_m,
                                           static_cast<const args_type>(args_m),
                                           std::make_index_sequence<sizeof...(Args)>(),
                                           std::forward<LateArgs>(late_args)...);
    }
  }; // blind_t

}}} // namespace com::masaers::blind

#endif
