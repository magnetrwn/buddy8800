#ifndef BUDDY8800_SRC_UTIL_ITERATOR_HPP_
#define BUDDY8800_SRC_UTIL_ITERATOR_HPP_

#include <iterator>
#include <type_traits>

namespace buddy8800 {

template <typename iterator_type>
using enable_input_iterator =
    std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<iterator_type>::iterator_category,
                                           std::input_iterator_tag>>;

}

#endif
