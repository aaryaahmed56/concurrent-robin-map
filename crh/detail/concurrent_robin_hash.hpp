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
    
    /**
     * @brief Hashes for bucket entries
     * 
     * @tparam StoreHash Flag representing 
     * storage state of hash
     */
    template< bool StoreHash >
    class bucket_entry_hash
    {
    protected:
        void set_hash(const hash::hash_t& hash) noexcept {}

    public:
        bool bucket_hash_equal(const hash::hash_t& hash) noexcept { return true; }
        hash::truncated_hash_t truncated_hash() const noexcept { return 0; }
    };
    /**
     * @brief Specialization for if a hash
     * has been stored
     * 
     * @tparam  
     */
    template<>
    class bucket_entry_hash<true>
    {
    private:
        hash::truncated_hash_t _m_hash;
        
    protected:
        void set_hash(const hash::truncated_hash_t& hash) noexcept
        {
            this->_m_hash = hash::truncated_hash_t(hash);
        }
    
    public:
        bool bucket_hash_equal(const hash::hash_t& hash) const noexcept
        {
            return this->_m_hash == hash::truncated_hash_t(hash);
        }

        hash::truncated_hash_t truncated_hash() const noexcept
        {
            return this->_m_hash;
        }
    };
    
} // namespace crh

#endif // !CONCURRENT_ROBIN_HASH_HPP