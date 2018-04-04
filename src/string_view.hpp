/** zs-apc-spdu-ctl string_view.hpp
      a meta-include to declare a typedef zs::string_view
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <config.h>
#if defined(HAVE_CXXH_STRING_VIEW)
# include <string_view>
# define HAVE_STRING_VIEW
#elif defined(HAVE_CXXH_EX_STRING_VIEW)
# include <experimental/string_view>
# define HAVE_STRING_VIEW
#endif

namespace zs {
  typedef std::
#ifndef HAVE_STRING_VIEW
    string
#elif defined(HAVE_CXXH_EX_STRING_VIEW)
    experimental::string_view
#else
    string_view
#endif
      string_view;
}
