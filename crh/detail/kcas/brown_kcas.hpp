#ifndef CRH_BROWN_KCAS_HPP
#define CRH_BROWN_KCAS_HPP

#include "harris_kcas.hpp"
namespace crh
{
    /**
     * @brief Implementation of 
     * modified kCAS algorithm as presented 
     * by Brown and Arbel-Raviv
     * 
     * @tparam Allocator An allocator policy
     * @tparam MemReclaimer A memory reclaimer policy
     */
    template< class Allocator,
              class MemReclaimer >
    class brown_kcas
    {
    public:
        using alloc_t = std::size_t;
        using state_t = std::uintptr_t;
        
        enum class tag_type
        {
            NONE,
            RDCSS,
            KCAS
        };

        static constexpr alloc_t S_NO_TAG = 0x0, S_KCAS_TAG = 0x1, S_RDCSS_TAG = 0x2;
        static constexpr alloc_t S_THREAD_ID_SHIFT = 2, S_THREAD_ID_MASK = (1 << 8) - 1;
        static constexpr alloc_t S_SEQUENCE_SHIFT = 10, S_SEQUENCE_MASK = (alloc_t(1) << 54) - 1;
        
        static constexpr state_t UNDECIDED = 0, SUCCESS = 1, FAILED = 2;
        
    private:
        /**
         * @brief A class representing the
         * status of a given descriptor for
         * kCAS
         * 
         */
        class k_cas_descriptor_status
        {
        private:
            state_t _sequence_number, _status;
            
        public:
            explicit
            k_cas_descriptor_status() :
                _status(UNDECIDED),
                _sequence_number(0) {}
            
            explicit
            k_cas_descriptor_status(const state_t& status, 
                const state_t& sequence_number) :
                _status(status), 
                _sequence_number(sequence_number) {}

            ~k_cas_descriptor_status() {}
        };

        /**
         * @brief A pointer with additional associated
         * data, in this case a raw bit count
         * 
         */
        class tagged_pointer
        {
        private:
            state_t _raw_bits;
        
        public:
            explicit
            tagged_pointer() : _raw_bits(0) {}

            explicit
            tagged_pointer(const state_t& raw_bits) : 
                _raw_bits(raw_bits)
            {
                static_assert(this->is_bits(*this));
            }

            explicit
            tagged_pointer(const state_t& tag_bits, 
                const state_t& thread_id,
                const state_t& sequence_number) : 
                _raw_bits(tag_bits | (thread_id << S_THREAD_ID_SHIFT) 
                    | (sequence_number << S_SEQUENCE_SHIFT)) {}

            tagged_pointer(const tagged_pointer&) = default;
            tagged_pointer &operator=(const tagged_pointer&) = default;

            ~tagged_pointer() {}

            inline
            bool is_kcas(const tagged_pointer& tag_ptr) const noexcept
            {
                return (tag_ptr._raw_bits & S_KCAS_TAG) == S_KCAS_TAG;
            }
            
            inline
            bool is_rdcss(const tagged_pointer& tag_ptr) const noexcept
            {
                return (tag_ptr._raw_bits & S_RDCSS_TAG) == S_RDCSS_TAG;
            }

            inline
            bool is_bits(const tagged_pointer& tag_ptr) const noexcept
            {
                return !(this->is_kcas(tag_ptr) || this->is_rdcss(tag_ptr));
            }
        };
    };
} // namespace crh

#endif // !CRH_BROWN_KCAS_HPP