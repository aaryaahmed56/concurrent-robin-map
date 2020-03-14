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
    template< class Allocator,
              class MemReclaimer >
    class harris_kcas
    {
    public:
        using alloc_t = typename std::size_t;
        using state_t = typename std::uintptr_t;

        static constexpr alloc_t S_KCAS_BIT = 0x1, S_RDCSS_BIT = 0x2;
        
        static constexpr state_t UNDECIDED = 0, SUCCESS = 1, FAILED = 2;

    private:
        /**
         * @brief General descriptor control for the restricted double
         * compare and single swap method
         * 
         * @tparam word_t A word of specified bit size
         * @tparam addr_t A control or data address
         */
        template< typename word_t,
              typename addr_t >
        class rdcss_descriptor
        {
        private:
            const word_t _expected_c_value, _expected_d_value, _new_w_value;
            
            std::atomic<bool> is_desc;
            
            std::unique_ptr<addr_t> _control_address, _data_address;

        public:
            inline
            rdcss_descriptor() : is_desc(true) {}

            explicit
            rdcss_descriptor(const addr_t& control_address,
                const addr_t& data_address) :
                _control_address(std::make_unique<addr_t>(control_address)),
                _data_address(std::make_unique<addr_t>(data_address)) {}

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
            static
            inline
            constexpr
            word_t cas_1(std::unique_ptr<word_t> a, word_t o, word_t n) noexcept
            {
                word_t old = a.get();
                
                if (old == o) a = std::make_unique<word_t>(n);
                
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
            static
            constexpr
            word_t rdcss(const rdcss_descriptor& desc) noexcept
            {
                word_t r;

                do
                {
                    r = cas_1(desc._data_address, desc._expected_d_value, desc);
                } while(is_descriptor(desc));
                
                if (r == desc._expected_d_value) complete(desc);
                
                return r;
            }

            static
            constexpr
            word_t read(std::unique_ptr<addr_t> addr) noexcept
            {
                word_t r;
                do
                {
                    r = std::atomic_load<addr_t>(addr.get());
                    if (is_descriptor(r)) complete(r);
                } while(is_descriptor(r));
                
                return r;
            }

            static
            inline
            constexpr
            void complete(const rdcss_descriptor& desc) noexcept
            {
                word_t v = std::atomic_load<word_t>(desc._control_address.get());
                
                if (v == desc._expected_c_value)
                    cas_1(desc._data_address, desc, desc._new_w_value);
                else
                    cas_1(desc._data_address, desc, desc._expected_d_value);
            }

            static
            inline
            constexpr
            bool is_descriptor(const rdcss_descriptor& desc) noexcept
            {
                return desc.is_desc;
            }
        };

        enum class k_cas_descriptor_status
        {
            UNDECIDED,
            SUCCESS,
            FAILED
        };
        
        template< typename word_t,
                  typename addr_t >
        union descriptor_union
        {
        public:
            using rdcss_descriptor = typename harris_kcas<Allocator, MemReclaimer>
                ::rdcss_descriptor<word_t, addr_t>;
        
        private:
            state_t _bits;

            word_t _val;

            std::unique_ptr<rdcss_descriptor> _descriptor;

        public:
            inline
            descriptor_union() : _bits(0) {}

            inline
            explicit
            descriptor_union(const state_t& bits) : _bits(bits) {}

            inline
            explicit
            descriptor_union(const word_t& val) : _val(val) {}
            
            explicit
            descriptor_union(const rdcss_descriptor& desc) :
                _descriptor(std::make_unique<rdcss_descriptor>(desc))
            {
                static_assert(is_rdcss(*this));
            }

            descriptor_union(const descriptor_union&) = default;
            descriptor_union &operator=(const descriptor_union&) = default;

            ~descriptor_union() {}

            static
            inline
            constexpr
            bool is_rdcss(const descriptor_union& desc) noexcept
            {
                return (desc._bits & S_RDCSS_BIT) == S_RDCSS_BIT;
            }

            static
            inline
            constexpr
            bool is_kcas(const descriptor_union& desc) noexcept
            {
                return (desc._bits & S_KCAS_BIT) == S_KCAS_BIT;
            }
        };
        
        template< typename word_t,
                  typename addr_t >
        class entry_payload
        {
        public:
            using data_location_t = typename harris_kcas<Allocator, MemReclaimer>
                ::descriptor_union<word_t, addr_t>;

        private:
            addr_t _addr;

            data_location_t _old_val, _new_val;

            std::unique_ptr<data_location_t> _data_location;

        public:
            entry_payload() :
                _old_val(data_location_t()),
                _new_val(data_location_t()),
                _data_location(std::make_unique<data_location_t>()) {}

            explicit
            entry_payload(const word_t& old_val,
                const word_t& new_val) :
                _old_val(data_location_t(old_val)),
                _new_val(data_location_t(new_val)),
                _data_location(std::make_unique<data_location_t>()) {}

            entry_payload(const entry_payload&) = default;
            entry_payload &operator=(const entry_payload&) = default;

            ~entry_payload() {}
        };
        
        template< typename word_t,
                  typename addr_t >
        class k_cas_descriptor
        {
        public:
            using entry_t = typename harris_kcas<Allocator, MemReclaimer>
                ::entry_payload<word_t, addr_t>;

            using status_t = typename std::atomic<k_cas_descriptor_status>;

        private:
            const unsigned _n = 445;
            
            entry_t _entry;
            status_t _descriptor_status;

            std::atomic<bool> is_desc = {true};

        public:
            k_cas_descriptor() :
                _entry(entry_t()),
                _descriptor_status(status_t(UNDECIDED)) {}
            
            k_cas_descriptor(const status_t& desc_status) :
                _entry(entry_t()),
                _descriptor_status(desc_status) {}

            k_cas_descriptor(const word_t& old_val, const word_t& new_val) :
                _entry(entry_t(old_val, new_val)),
                _descriptor_status(status_t(UNDECIDED)) {}
            
            k_cas_descriptor(const k_cas_descriptor&) = default;
            k_cas_descriptor &operator=(const k_cas_descriptor_status&) = default;

            ~k_cas_descriptor() {}

            /**
             * @brief Multi-word compare and swap method
             * 
             * @param desc A kCAS descriptor
             * @return true 
             * @return false 
             */
            inline
            bool kcas(const k_cas_descriptor& desc) const noexcept
            {
                if (std::atomic_load<word_t>(desc._descriptor_status) == UNDECIDED)
                {
                    this->_descriptor_status = SUCCESS;
                    for (unsigned i = 0; i < desc._n; ++i)
                    {
                        this->_entry = desc._entry;
                    }
                }
            }

            inline
            bool is_descriptor(const k_cas_descriptor& desc) const noexcept
            {
                return desc.is_desc;
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