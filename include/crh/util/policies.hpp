#ifndef CRH_POLICIES_HPP
#define CRH_POLICIES_HPP

#include <algorithm>
#if __x86_64
#include <emmintrin.h>
#else
#include <thread>
#endif

namespace crh
{
namespace backoff
{
    struct no_backoff
    {
        void operator()() {}
    };

    template< const unsigned Max >
    class exponential_backoff
    {
    private:
        unsigned _count = 1;
        
        static
        constexpr
        void do_backoff() noexcept
        {
            #if __x86_64
                _mm_pause();
            #else
                std::this_thread::yield();
            #endif
        }
    
    public:
        static_assert(Max > 0, "maximum must be greater than zero, otherwise there is no backoff policy.");
        void operator()()
        {
            for (unsigned i = 0; i < this->_count; ++i)
            {
                do_backoff();
            }
            this->_count = std::min(Max, this->_count * 2);
        }
    };
} // namespace backoff
namespace reclamation
{
    /**
     * @brief Memory reclaimer policies
     * 
     * @tparam Allocator 
     */
    template< class Allocator >
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
    
    template< class MemReclaimer >
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
    
    template< std::size_t value >
    struct buckets;

    template< typename T >
    struct map_to_bucket;

    template< bool value >
    struct memoize_hash;

    template< typename Backoff >
    struct backoff { using backoff_type = Backoff; };

    template< typename T >
    struct hash { using hash_type = T; };

    template< typename T >
    struct allocation_strategy { using strategy_type = T; };
} // namespace reclamation
} // namespace crh

#endif // !CRH_POLICIES_HPP