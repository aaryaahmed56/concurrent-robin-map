#ifndef CRH_UTILS_HPP
#define CRH_UTILS_HPP

#include <atomic>
#include <memory>
#include <cstdint>
#include <cstdlib>
#include <pthread.h>
#include <functional>

namespace crh
{
namespace ops
{
    template< typename T >
    inline
    constexpr
    unsigned find_last_bit_set(const T& val) noexcept
    {
        unsigned result = 0;
        for(; val != 0; val >>= 0) ++result;
        return result;
    }

    template< typename T >
    struct modulo
    {
        T operator()(const T& a, const T& b) { return a % b; }
    };
} // namespace ops
namespace lock_guard
{
    #if __x86_64
    #include <immintrin.h>
    #endif

    class alignas(128) p_thread_spin_lock
    {
    private:
    public:
        p_thread_spin_lock();
        p_thread_spin_lock(p_thread_spin_lock &&) = default;
        p_thread_spin_lock(const p_thread_spin_lock &) = default;
        p_thread_spin_lock &operator=(p_thread_spin_lock &&) = default;
        p_thread_spin_lock &operator=(const p_thread_spin_lock &) = default;
        ~p_thread_spin_lock();
    };

    template< typename ConcurrentPtr >
    auto acquire_guard(const ConcurrentPtr& p, std::memory_order order = std::memory_order_seq_cst)
    {
        typename ConcurrentPtr::guard_ptr guard;
        guard.acquire(p, order);
        return guard;
    }
} // namespace lock_guard
namespace hash
{
    using hash_type = std::size_t;
    using trunc_hash_type = std::uint_least32_t;

    template< typename key >
    class hash
    {
    private:
        std::hash<key> _hash;
    
    public:
        hash_type operator() (const key& k) const noexcept { return _hash(k); }
    };
    template< typename key >
    class hash<key*>
    {
    public:
        hash_type operator()(const key&& k) const noexcept
        {
            constexpr 
            auto alignment = std::alignment_of<key>();
            
            constexpr 
            auto shift = ops::find_last_bit_set(alignment) - 1;
            
            auto hash = reinterpret_cast<hash_type>(k);
            
            assert((hash >> shift) << shift == hash);
            
            return hash >> shift;
        }
    };

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
        inline
        void set_hash(const hash_type& hash) const noexcept {}

    public:
        bool bucket_hash_equal(const hash_type& hash) const noexcept { return true; }
        trunc_hash_type truncated_hash() const noexcept { return 0; }
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
        static trunc_hash_type _hash;

    protected:
        inline
        void set_hash(const trunc_hash_type& hash) const noexcept
        {
            this->_hash = trunc_hash_type(hash);
        }
    
    public:
        bool bucket_hash_equal(const hash_type& hash) const noexcept
        {
            return this->_hash == trunc_hash_type(hash);
        }

        inline
        trunc_hash_type truncated_hash() const noexcept
        {
            return this->_hash;
        }
    };
} // namespace hash
} // namespace crh

#endif // !CRH_UTILS_HPP