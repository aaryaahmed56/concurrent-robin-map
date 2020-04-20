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
        using alloc_type = typename std::size_t;
        using state_type = typename std::uintptr_t;

        static constexpr alloc_type S_KCAS_BIT = 0x1, S_RDCSS_BIT = 0x2;
        
        static constexpr state_type UNDECIDED = 0, SUCCESS = 1, FAILED = 2;

    private:
        /**
         * @brief General descriptor control for the restricted double
         * compare and single swap method
         * 
         * @tparam WordType A word of specified bit size
         * @tparam AddrType A control or data address
         */
        template< class WordType,
                  class AddrType >
        class rdcss_descriptor
        {
        private:
            const WordType _expected_c_value, _expected_d_value, _new_w_value;
            
            std::atomic<bool> is_desc;
            
            std::unique_ptr<AddrType> _control_address, _data_address;

        public:
            inline
            rdcss_descriptor() : is_desc(true) {}

            explicit
            rdcss_descriptor(const AddrType& control_address,
                const AddrType& data_address) :
                _control_address(std::make_unique<AddrType>(control_address)),
                _data_address(std::make_unique<AddrType>(data_address)) {}

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
             * @return WordType A word of specified bit size
             */
            static
            inline
            constexpr
            WordType cas_1(WordType a, WordType o, WordType n) noexcept
            {
                std::unique_ptr<WordType> a_ptr = std::make_unique<WordType>(a);
                
                WordType old = a_ptr.release();
                
                if (old == o) a_ptr = std::make_unique<WordType>(n);
                
                return old;
            }

            /**
             * @brief The restricted double compare
             * single swap method
             * 
             * @param desc An descriptor representing the
             * expected and new control and data addresses 
             * and values
             * @return WordType A word of specified bit size
             */
            static
            constexpr
            WordType rdcss(const rdcss_descriptor& desc) noexcept
            {
                WordType r;

                do
                {
                    r = cas_1(desc._data_address, desc._expected_d_value, desc);
                } while(is_descriptor(desc));
                
                if (r == desc._expected_d_value) complete(desc);
                
                return r;
            }

            static
            constexpr
            WordType read(AddrType addr) noexcept
            {
                std::unique_ptr<AddrType> addr_ptr = std::make_unique<AddrType>(addr);

                WordType r;
                do
                {
                    r = std::atomic_load<AddrType>(addr_ptr.release());
                    if (is_descriptor(r)) complete(r);
                } while(is_descriptor(r));
                
                return r;
            }

            static
            inline
            constexpr
            void complete(const rdcss_descriptor& desc) noexcept
            {
                WordType v = std::atomic_load<WordType>(desc._control_address.get());
                
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
        
        template< typename WordType,
                  typename AddrType >
        union descriptor_union
        {
        public:
            using rdcss_descriptor = typename harris_kcas<Allocator, MemReclaimer>
                ::rdcss_descriptor<WordType, AddrType>;
        
        private:
            state_type _bits;

            WordType _val;

            std::unique_ptr<rdcss_descriptor> _descriptor;

        public:
            inline
            descriptor_union() : _bits(0) {}

            inline
            explicit
            descriptor_union(const state_type& bits) : _bits(bits) {}

            inline
            explicit
            descriptor_union(const WordType& val) : _val(val) {}
            
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
        
        template< typename WordType,
                  typename AddrType >
        class entry_payload
        {
        public:
            using data_loc_type = typename harris_kcas<Allocator, MemReclaimer>
                ::descriptor_union<WordType, AddrType>;

        private:
            AddrType _addr;

            data_loc_type _old_val, _new_val;

            std::unique_ptr<data_loc_type> _data_location;

        public:
            entry_payload() :
                _old_val(data_loc_type()),
                _new_val(data_loc_type()),
                _data_location(std::make_unique<data_loc_type>()) {}

            explicit
            entry_payload(const WordType& old_val,
                const WordType& new_val) :
                _old_val(data_loc_type(old_val)),
                _new_val(data_loc_type(new_val)),
                _data_location(std::make_unique<data_loc_type>()) {}

            entry_payload(const entry_payload&) = default;
            entry_payload &operator=(const entry_payload&) = default;

            ~entry_payload() {}
        };
        
        template< typename WordType,
                  typename AddrType >
        class k_cas_descriptor
        {
        public:
            using entry_t = typename harris_kcas<Allocator, MemReclaimer>
                ::entry_payload<WordType, AddrType>;

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

            k_cas_descriptor(const WordType& old_val, const WordType& new_val) :
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
                if (std::atomic_load<WordType>(desc._descriptor_status) == UNDECIDED)
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