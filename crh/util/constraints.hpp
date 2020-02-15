#ifndef CRH_TYPE_CONSTRAINTS_HPP
#define CRH_TYPE_CONSTRAINTS_HPP

#include <type_traits>

namespace crh
{
namespace constraints
{
    struct unit {};

    template< class T > struct is_set : std::true_type {};
    template<> struct is_set<unit> : std::false_type {};
    using is_nil_set = typename constraints::is_set<unit>;

    template< class... Args >
    struct pack;
    
    template< template< class > class Policy, class Pack, class Default = unit >
    struct type_constraints;

    template< template< class > class Policy, class Default, class Arg, class... Args >
    struct type_constraints<Policy, pack<Arg, Args...>, Default>
    {
        using type = typename type_constraints<Policy, pack<Args...>, Default>::type;
    };
    
    template< template< class > class Policy, class T, class Default, class... Args >
    struct type_constraints<Policy, pack<Policy<T>, Args...>, Default>
    {
        using type = T;
    };

    template< template< class > class Policy, class Default >
    struct type_constraints<Policy, pack<>, Default>
    {
        using type = Default;
    };

    template< template< class > class Policy, class Default, class... Args>
    using type_constraint_t = typename type_constraints<Policy, pack<Args...>, Default>::type;

    template< class ValueType, template <ValueType> class Policy, class Pack, ValueType Default >
    struct value_param;

    template< class ValueType, template <ValueType> class Policy, ValueType Default, class Arg, class... Args >
    struct value_param<ValueType, Policy, pack<Arg, Args...>, Default> 
    {
        static 
        constexpr 
        ValueType value = value_param<ValueType, Policy, pack<Args...>, Default>::value;
    };

    template< class ValueType, template <ValueType> class Policy, ValueType Value, ValueType Default, class... Args >
    struct value_param<ValueType, Policy, pack<Policy<Value>, Args...>, Default>
    {
        static 
        constexpr 
        ValueType value = Value;
    };

    template< class ValueType, template <ValueType> class Policy, ValueType Default >
    struct value_param<ValueType, Policy, pack<>, Default>
    {
        static 
        constexpr 
        ValueType value = Default;
    };

    template<class ValueType, template <ValueType> class Policy, ValueType Default, class... Args>
    using value_param_t = value_param<ValueType, Policy, pack<Args...>, Default>;
} // namespace constraints
} // namespace crh


#endif // !CRH_TYPE_CONSTRAINTS_HPP