#ifndef CRH_FORWARDING_CONSTRUCTORS_HPP
#define CRH_FORWARDING_CONSTRUCTORS_HPP

#include <vector>

namespace crh
{
namespace forwardingconstructors
{
    template< typename T >
    struct identity { using type = T; };

    template< class D, class... Ts>
    struct ret : identity<D> {};

    template< class... Ts>
    struct ret<void, Ts...> : std::common_type<Ts...> {};

    template< class D, class... Ts>
    using ret_t = typename ret<D, Ts...>::type;
    
    template< class D = void, class... Ts>
    std::vector<ret_t<D, Ts...>> make_vector(Ts&&... args)
    {
        std::vector<ret_t<D, Ts...>> ret;
        ret.reserve(sizeof...(args));
        using expander = int[];
        (void)expander{ ((void)ret.emplace_back(std::forward<Ts>(args)), 0)..., 0};
        return ret;
    }
} // namespace forwardingconstructors
} // namespace crh

#endif // !CRH_FORWARDING_CONSTRUCTORS_HPP