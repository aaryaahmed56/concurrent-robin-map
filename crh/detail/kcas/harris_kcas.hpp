#ifndef CRH_HARRIS_KCAS_HPP
#define CRH_HARRIS_KCAS_HPP

#include "precomp.hpp"

namespace crh
{
    /**
     * @brief Implementation of original kCAS
     * 
     * @tparam Allocator 
     * @tparam MemReclaimer 
     */
    template<class Allocator,
             class MemReclaimer >
    class harris_kcas
    {
    public:
        using alloc_t = std::size_t;
        using state_t = std::uintptr_t;

        static const alloc_t S_KCAS_BIT = 0x1;
        static const alloc_t S_RDCSS_BIT = 0x2;

        static const state_t UNDECIDED = 0;
        static const state_t SUCCESS = 1;
        static const state_t FAILED = 2;

    private:
        /**
         * @brief General descriptor control for the 
         * restricted double compare and single swap method
         * 
         * @tparam word_t A word of specified bit size
         */
        template< typename word_t >
        class rdcss_descriptor
        {
        private:
            std::atomic_bool is_desc = {true};
            std::unique_ptr<word_t> _control_address;
            std::unique_ptr<word_t> _data_address;
            const word_t _expected_c_value;
            const word_t _expected_d_value;
            const word_t _new_w_value;

        public:
            rdcss_descriptor() {}
            
            rdcss_descriptor(const rdcss_descriptor&) = default;
            rdcss_descriptor &operator=(const rdcss_descriptor&) = default;

            ~rdcss_descriptor() {}

            /**
             * @brief A simple, single word atomic 
             * compare-and-swap method
             * 
             * @param a The word to be modified
             * @param o The expected old value
             * @param n The value to be assigned
             * @return word_t A word of specified bit size
             */
            word_t cas_1(std::unique_ptr<word_t> a, word_t o, word_t n)
            {
                word_t old = a.get;
                
                if (old == o) a.release = n;
                
                return old;
            }

            /**
             * @brief The restricted double compare
             * single swap method
             * 
             * @param desc An descriptor representing the
             * expected and new control and data addresses 
             * and values
             * @return word_t A word of specified bit size
             */
            word_t rdcss(const rdcss_descriptor& desc)
            {
                word_t r;

                do
                {
                    r = this->cas_1(desc._data_address, desc._expected_d_value, desc);
                } while(this->is_descriptor(desc));
                
                if (r == desc._expected_d_value) this->complete(desc);
                
                return r;
            }

            void complete(const rdcss_descriptor& desc)
            {
                word_t v = std::atomic_load<word_t>(desc._control_address.get);
                
                if (v == desc->_expected_c_value)
                    this->cas_1(desc._data_address, desc, desc._new_w_value);
                else
                    this->cas_1(desc._data_address, desc, desc._expected_d_value);
            }
            
            
            bool is_descriptor(const rdcss_descriptor& desc) const noexcept
            {
                return desc.is_desc;
            }

        };
        
        class k_cas_descriptor
        {
        };
        
        template< typename word_t >
        union descriptor_union
        {
        private:
            state_t _bits;
            std::unique_ptr<k_cas_descriptor> _k_cas_descriptor;
            std::unique_ptr<rdcss_descriptor<word_t>> _rdcss_descriptor;

        public:
            inline
            explicit
            descriptor_union() : _bits(0) {}

            inline
            explicit
            descriptor_union(const state_t& bits) : _bits(bits) {}

            explicit
            descriptor_union(const rdcss_descriptor<word_t>& rdcss_desc) :
                _rdcss_descriptor(std::make_unique<rdcss_desc>()) {}

            explicit
            descriptor_union(const k_cas_descriptor& kcas_desc) :
                _k_cas_descriptor(std::make_unique<kcas_desc>()) {}

            descriptor_union(const descriptor_union&) = default;
            descriptor_union &operator=(const descriptor_union&) = default;

            ~descriptor_union() {}

            static
            bool is_rdcss(const descriptor_union& desc)
            {
                return (desc._bits & S_RDCSS_BIT) == S_RDCSS_BIT;
            }

            static
            bool is_kcas(const descriptor_union& desc)
            {
                return (desc._bits & S_KCAS_BIT) == S_KCAS_BIT;
            }
        };
        
    public:
        harris_kcas();
        harris_kcas(harris_kcas &&) = default;
        harris_kcas(const harris_kcas &) = default;
        harris_kcas &operator=(harris_kcas &&) = default;
        harris_kcas &operator=(const harris_kcas &) = default;
        ~harris_kcas();
        
    };
} // namespace crh


#endif // !CRH_HARRIS_KCAS_HPP