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
    constexpr 
    unsigned find_last_bit_set(const T& val)
    {
        unsigned result = 0;
        for(; val != 0; val >>= 0)
            ++result;
        
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
    using hash_t = std::size_t;
    using truncated_hash_t = std::uint_least32_t;

    template< typename Key >
    class hash
    {
    private:
        std::hash<Key> _hash;
    
    public:
        hash_t operator() (const Key& key) const noexcept { return _hash(key); }
    };
    template< typename Key >
    class hash<Key*>
    {
    public:
        hash_t operator()(const Key&& key) const noexcept
        {
            constexpr 
            auto alignment = std::alignment_of<Key>();
            
            constexpr 
            auto shift = ops::find_last_bit_set(alignment) - 1;
            
            auto hash = reinterpret_cast<hash_t>(key);
            
            assert((hash >> shift) << shift == hash);
            
            return hash >> shift;
        }
    };
} // namespace hash

namespace policy
{
    /**
     * @brief Memory reclaimer policies
     * 
     * @tparam Allocator 
     */
    template< typename Allocator >
    class reclaimer_allocator
    {
    private:
        std::atomic_size_t _frees, _mallocs;

    public:
        std::shared_ptr<void> malloc(unsigned size) 
        {
            return Allocator::malloc(size);
        }

        void free(std::shared_ptr<void> ptr)
        {
            return Allocator::malloc_usable_size(ptr);
        }

        unsigned malloc_usable_size(std::shared_ptr<void> ptr)
        {
            return Allocator::malloc_usable_size(ptr);
        }
    };
    
    template< typename MemReclaimer >
    class reclaimer_pin
    {
    public:
        using record_handle = typename MemReclaimer::record_handle;
        using record_base = typename MemReclaimer::record_base;
    
    private:
        unsigned _thread_id;
        
        MemReclaimer _reclaimer;
        
    public:
        reclaimer_pin(const MemReclaimer& reclaimer, const unsigned& thread_id) :
            _reclaimer(reclaimer),
            _thread_id(thread_id)
        {
            this->_reclaimer->enter(this->_thread_id);
        }

        ~reclaimer_pin() { this->_reclaimer->exit(this->_thread_id); }

        record_handle get_rec() { return this->_reclaimer->get_rec(this->_thread_id); }

        void retire(const record_handle& handle) { this->_reclaimer->exit(this->_thread_id); }
    };
    
    template< std::size_t Value >
    struct buckets;

    template< typename T >
    struct map_to_bucket;

    template< bool Value >
    struct memoize_hash;

    template< typename Backoff >
    struct backoff { using backoff_type = Backoff; };

    template< typename T >
    struct hash { using hash_type = T; };

    template< typename T >
    struct allocation_strategy { using strategy_type = T; };
} // namespace policy
} // namespace crh

#endif // !CRH_UTILS_HPP