#include "fungus_util_endian.h"

namespace fungus_util
{
	endian_registration::endian_type::~endian_type() = default;
	
    endian_registration::endian_registration(endian_type *et): et(et) {if (et && et->get_type() == typeid(void)) {delete et; et = nullptr;}}

    endian_registration::endian_registration(const endian_registration &h): et(h.et ? h.et->clone() : nullptr) {}
    endian_registration::~endian_registration() {if (et) delete et;}

    const std::type_info &endian_registration::get_type() const {return et ? et->get_type() : typeid(void);}
    const int endian_registration::get_net_endian() const {return et ? big_endian : 0;}

    endian_converter::endian_converter():
        m(),
#ifdef FUNGUSUTIL_CPP11_PARTIAL
        type_regs(100),
#else
        type_regs(),
#endif
        __lazy_init_numeric(false)
    {}

    endian_converter::endian_converter(endian_converter &&endian):
        m(),
#ifdef FUNGUSUTIL_CPP11_PARTIAL
        type_regs(std::move(endian.type_regs)),
#else
        type_regs(endian.type_regs),
#endif
        __lazy_init_numeric(endian.__lazy_init_numeric)
    {}

    void endian_converter::register_type(const endian_registration &et)
    {
        lock guard(m);
#ifdef FUNGUSUTIL_CPP11_PARTIAL
        type_regs.insert(__endian_reg_map_type::entry(et.get_type(), et));
#else
        type_regs[et.get_type()] = et;
#endif
    }

    void endian_converter::unregister_type(const type_info_wrap &info)
    {
        lock guard(m);
        type_regs.erase(info);
    }

    bool endian_converter::type_registered(const type_info_wrap &info, int &target_endian) const
    {
        lock guard(const_cast<mutex &>(m));
        auto it = type_regs.find(info);
        if (it != type_regs.end())
        {
#ifdef FUNGUSUTIL_CPP11_PARTIAL
            target_endian = it->value.get_net_endian();
#else
            target_endian = it->second.get_net_endian();
#endif
            return true;
        }
        else
            return false;
    }

    void endian_converter::lazy_register_numeric_types()
    {
        lock guard(m);
        if (!__lazy_init_numeric)
        {
            register_type<int16_t>();
            register_type<int32_t>();
            register_type<int64_t>();

            register_type<uint16_t>();
            register_type<uint32_t>();
            register_type<uint64_t>();

            register_type<float>();
            register_type<double>();

            __lazy_init_numeric = true;
        }
    }
}
