#include "fungus_booster/fungus_booster.h"

using namespace fungus_util;

// the source attributes
typedef attribute_array_no_deps_def<int, 1> number_attribute_def;
typedef attribute_array_no_deps_def<std::string, 2> string_attribute_def;
typedef attribute_array_no_deps_def<std::string, 3> combined_attribute_def;

// the recompute functor for the workspace
struct my_combiner
{
    typedef std::string return_type;
    
    inline std::string operator()(int m_number, const std::string &m_str)
    {
        return make_string(m_str, m_number);
    }
};

// the workspace attributes
typedef
    attribute_array_def
    <
        std::string, 4, my_combiner,
        attribute_array_dependency_def<number_attribute_def::ID, self_dependency>,
        attribute_array_dependency_def<string_attribute_def::ID, self_dependency>
    >
    work_combined_attribute_def;
   
// the source containers 
typedef attribute_array_container<number_attribute_def, string_attribute_def> component_source_container;
typedef attribute_array_container<combined_attribute_def> result_source_container;

// the workspace container
typedef
    attribute_array_container
    <
        number_attribute_def,
        string_attribute_def,
        work_combined_attribute_def
    >
    workspace_container;
    
typedef
    attribute_array_exchanger_batch
    <
        attribute_array_exchanger<number_attribute_def>,
        attribute_array_exchanger<string_attribute_def>
    >
    source_exchanger;
   
typedef
    non_uniform_attribute_array_exchanger<combined_attribute_def, work_combined_attribute_def>
    result_exchanger;
   
source_exchanger source_xchg;
result_exchanger result_xchg;

static constexpr const char *_test_strings[] =
{
    "hella", "wella", "wiggle", "jiggle",
    "nargle", "flarpy", "doobie", "dank"
};

int main()
{
	if (fungus_common::version_match())
    {
        std::cout << "using fungus_net version " << fungus_common::get_version_info().m_str << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.val==" << fungus_common::get_version_info().m_version.val << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.part.maj==" << (int)fungus_common::get_version_info().m_version.part.maj << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.part.min==" << (int)fungus_common::get_version_info().m_version.part.min << std::endl;
    }
    else
    {
        std::cout << "version mismatch between API and lib! O_o" << std::endl;
        return 1;
    }
	
	std::cout << "attribute_array_container unit tests" << std::endl << std::endl
	          << "testing construction...";

    component_source_container m_component_source;
    result_source_container    m_result_source;
    
    workspace_container m_workspace;
	
	std::cout << "good!" << std::endl << "testing resize (resize all to 8)...";

    m_component_source.resize(8);
    m_result_source.resize(8);
    m_workspace.resize(8);
	
	// just a little test for recursive_or thrown in.
	// this is not it's intended usage.  it's for variadic
	// classes or functions that want to perform an
	// or on all of their arguments without having to involve
	// that in a recursive instantiation.
	if (recursive_or(m_component_source.size() == 8, m_result_source.size() == 8, m_workspace.size() == 8))
		std::cout << "good!" << std::endl;
	else
	{
		std::cout << "fail. aborting." << std::endl;
		return 1;
	}

	std::cout << "not worrying about parent dependencies right now.";
    std::vector<dependency_index_pair> m_pairs;

    for (size_t i = 0; i < 8; ++i)
        m_pairs.push_back({i, i});
	
	std::cout << "good!" << std::endl
              << "initializing source attribute array containers...";
    for (size_t i = 0; i < 8; ++i)
    {
        m_component_source.get_attribute_array(number_attribute_def()).set(i, i);
        m_component_source.get_attribute_array(string_attribute_def()).set(i, _test_strings[i]);
    }
    
    std::cout << "good!" << std::endl
              << "exchanging sources in to workspace...";
              
    source_xchg(m_component_source, m_workspace);
    result_xchg(m_result_source, m_workspace);
    std::cout << "good!" << std::endl
              << "recomputing for all index pairs...";
              
    m_workspace.recompute_index_pairs(m_pairs, post_recompute_flag_action_reset());
    std::cout << "good!" << std::endl
              << "exchanging sources out from workspace...";
    
    source_xchg(m_component_source, m_workspace);
    result_xchg(m_result_source, m_workspace);
    std::cout << "good!" << std::endl << std::endl
              << "***********" << std::endl
              << "* D U M P *" << std::endl
              << "***********" << std::endl;

    for (size_t i = 0; i < 8; ++i)
    {
        const auto &number_elem = m_component_source.get_attribute_array(number_attribute_def()).get(i);
        const auto &string_elem = m_component_source.get_attribute_array(string_attribute_def()).get(i);
        const auto &combined_elem = m_result_source.get_attribute_array(combined_attribute_def()).get(i);
        
        std::cout << "i=" << i << " number=" << number_elem.second << " string=" << string_elem.second << " combined=" << combined_elem.second << std::endl;
    }    
    std::cout << m_component_source.get_attribute_array(string_attribute_def()).get(0).second << std::endl;
    //__debug_dump_attribute_array(std::cout, m_component_source, number_attribute_def());
    //__debug_dump_attribute_array(std::cout, m_component_source, string_attribute_def());
    //__debug_dump_attribute_array(std::cout, m_result_source, combined_attribute_def());

	std::cout << "unit test complete." << std::endl;

	return 0;
}
