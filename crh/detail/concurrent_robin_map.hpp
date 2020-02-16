#ifndef CONCURRENT_ROBIN_MAP_HPP
#define CONCURRENT_ROBIN_MAP_HPP

#include "precomp.hpp"

namespace crh
{
    template<class Key,
             class T,
             class Hash = hash::hash<Key>,
             class Alloc = std::allocator<std::pair<const Key, T>>,
             class... Policies >
    class concurrent_robin_map
    {
    public:
        using key_type = Key;
        using map_type = T;
        using value_type = std::pair<const key_type, map_type>;
        using hasher = hash::hash<key_type>;
        using allocator_type = std::allocator<value_type>;
        using reclaimer = constraints::type_constraint_t<policy::reclaimer_allocator, constraints::unit, Policies...>;
        using hash = constraints::type_constraint_t<policy::hash, hasher, Policies...>;
        using map_to_bucket = constraints::type_constraint_t<policy::map_to_bucket, ops::modulo<std::size_t>, Policies...>;
    
        template< class... NewPolicies >
        using with = concurrent_robin_map<key_type, map_type, allocator_type, NewPolicies..., Policies...>;
    
        static_assert(constraints::is_set<reclaimer>::value, "specify reclaimer policy");
    
        class iterator;
        class accessor;
    
    private:
        unsigned _m_size, _m_size_mask, _m_num_timestamps;
        std::atomic_uint8_t _m_timestamp_shift;
        std::unique_ptr<map_to_bucket> _m_table;
        reclaimer _reclaimer;
    
        struct key_select
        {
            const key_type& operator()(const std::pair<key_type, map_type>& key_value) const noexcept
            {
                return key_value.first;
            }
            
            key_type& operator()(std::pair<key_type, map_type>& key_value) noexcept
            {
                return key_value.first;
            }
        };
    
        struct value_select
        {
            const map_type& operator()(const std::pair<key_type, map_type>& key_value) const noexcept
            {
                return key_value.second;
            }
    
            map_type& operator()(std::pair<key_type, map_type>& key_value) noexcept
            {
                return key_value.second;
            }
        };
    
    public:
        concurrent_robin_map(const unsigned& size, 
            const unsigned& threads) : 
            _m_size(size),
            _m_size_mask(_m_size - 1) {}
        
        ~concurrent_robin_map() {}
    
        bool emplace(const key_type& key, const unsigned thread_id);
    
        template< typename... Args >
        bool emplace(Args&&... args);
    
        template< typename... Args >
        std::pair<iterator, bool> emplace_or_get(Args&&... args);
    
        template< typename... Args >
        std::pair<iterator, bool> get_or_emplace(const key_type& key, Args&&... args);
        
        bool erase(const key_type& key, const unsigned thread_id);
        bool contains(const key_type& key);
        
        accessor operator[](const key_type& key);
        
        iterator erase(iterator pos);
        iterator find(const key_type& key);
        iterator begin();
        iterator end();
};

} // namespace crh

#endif // !CONCURRENT_ROBIN_MAP_HPP