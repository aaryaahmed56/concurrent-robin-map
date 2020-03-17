#ifndef CONCURRENT_ROBIN_HASH_HPP
#define CONCURRENT_ROBIN_HASH_HPP

#include "precomp.hpp"

namespace crh
{
    template< typename T >
    struct make_void
    {
        using type = void;
    };

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam void 
     */
    template< typename T, typename = void >
    struct has_is_transparent : public constraints::is_set<constraints::unit> {};

    /**
     * @brief 
     * 
     * @tparam T 
     */
    template< typename T >
    struct has_is_transparent<T, typename make_void<typename T::is_transparent>::type> : 
        public constraints::is_set<T> {};
} // namespace crh

#endif // !CONCURRENT_ROBIN_HASH_HPP