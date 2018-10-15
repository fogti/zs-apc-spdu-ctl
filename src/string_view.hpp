/** zs-apc-spdu-ctl string_view.hpp
      a meta-include to declare a typedef zs::string_view
    (C) 2018 Erik Zscheile
    License: MIT
 **/

#pragma once
#include <config.h>
#ifdef HAVE_STRING_VIEW
# include <string_view>
#else
# include <string>
#endif

namespace zs {
  typedef std::
#ifdef HAVE_STRING_VIEW
    string_view
#else
    string
#endif
      string_view;
}
