//
//  slot_iterator.h
//  slimsig
//
//  Created by Christopher Tarquini on 5/20/14.
//
//

#ifndef slimsig_slot_iterator_h
#define slimsig_slot_iterator_h
#include <iterator>
#include <type_traits>
#include <algorithm>
namespace slimsig {


template <class Container, class Traits = std::iterator_traits<typename Container::iterator>>
class offset_iterator : public std::iterator<
                          typename Traits::iterator_category,
                          typename Traits::value_type,
                          typename Traits::difference_type,
                          typename Traits::pointer,
                          std::forward_iterator_tag>
{
using traits = std::iterator_traits<offset_iterator>;
public:
  using container_type = typename std::decay<Container>::type;
  using size_type = typename Container::size_type;
  using reference = typename traits::reference;
  using pointer = typename traits::pointer;
  using difference_type = typename traits::difference_type;
  using value_type = typename traits::value_type;
  
  offset_iterator() : p_container(nullptr), m_offset(0) {};
  offset_iterator(const offset_iterator&) = default;
  offset_iterator(offset_iterator&&) = default;
  offset_iterator& operator=(const offset_iterator&) = default;
  offset_iterator& operator=(offset_iterator&&) = default;
  bool operator ==(const offset_iterator& rhs) const {
    return m_offset == rhs.m_offset && p_container == rhs.p_container;
  }
  bool operator !=(const offset_iterator rhs) const{
    return !(*this == rhs);
  }
  bool operator >(const offset_iterator rhs) const{
    return m_offset > rhs.m_offset && p_container > p_container;
  }
  bool operator <(const offset_iterator rhs) const{
    return m_offset < rhs.m_offset && p_container < p_container;
  }
  bool operator <=(const offset_iterator rhs) const {
    return m_offset <= rhs.m_offset && p_container <= p_container;
  }
  bool operator >=(const offset_iterator rhs) const {
    return m_offset >= rhs.m_offset && p_container >= p_container;
  }
  offset_iterator(container_type& container, size_type offset = 0) : p_container(&container), m_offset(offset) {};
  reference operator*() {
    return std::begin(*p_container) + m_offset;
  }
  pointer operator->() {
    return &(std::begin(*p_container) + m_offset);
  }
  offset_iterator& operator++() {
    ++m_offset;
    return *this;
  }
  offset_iterator operator++(int) {
    offset_iterator it(*this);
    ++m_offset;
    return it;
  }
  size_type size() const {
    return p_container == nullptr ? 0 : p_container->size();
  };
  operator bool() const {
    return p_container != nullptr && m_offset < p_container->size();
  }
  const container_type* p_container;
  size_type m_offset;
};
template <class Container, class Traits = std::iterator_traits<typename Container::iterator>>
class slot_iterator : private offset_iterator<Container, Traits>
{
using base = offset_iterator<Container, Traits>;
public:
  using base::container_type;
  using base::size_type;
  using base::reference;
  using base::pointer;
  using base::difference_type;
  using base::value_type;
  
  using base::base;
  using base::operator ==;
  using base::operator !=;
  using base::operator >;
  using base::operator <;
  using base::operator >=;
  using base::operator <=;
  operator bool() const {
    return bool(*this) && bool(**this);
  }
  slot_iterator& operator++() {
    do  {
      base::operator++();
    } while (bool(*this) && !(bool(**this)));
    return *this;
  }
  slot_iterator& operator++(int) {
    slot_iterator it(*this);
    ++(*this);
    return it;
  }
};

}

#endif
