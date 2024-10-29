#ifndef DEFINES_HPP_
#define DEFINES_HPP_

#define T_ITERATOR_SFINAE typename = typename std::enable_if<std::is_convertible<typename std::iterator_traits<T>::iterator_category, std::input_iterator_tag>::value>::type

#endif