#ifndef CRH_BACKOFF_HPP
#define CRH_BACKOFF_HPP

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

    template< unsigned Max >
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
} // namespace crh

#endif // !CRH_BACKOFF_HPP