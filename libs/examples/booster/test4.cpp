#include "fungus_booster/fungus_booster.h"

using namespace fungus_util;
namespace conc = fungus_concurrency;

struct sumation_worker: conc::worker<uint64_t, uint64_t>
{
    inline void operator()(uint64_t m_in, uint64_t &m_out)
    {
        m_out = m_in;
                
        while (--m_in > 1)
            m_out += m_in;
    }
};

int main()
{
    conc::future<uint64_t> m_future(conc::promise<sumation_worker>(1000000));
    
    std::cout << "the sumation of 1000000 is " << (uint64_t)m_future << std::endl;
}

