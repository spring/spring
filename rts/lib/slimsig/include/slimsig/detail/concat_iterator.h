//
//  concat_iterator.h
//  slimsig
//
//  Created by Christopher Tarquini on 4/22/14.
//
// based on this stackoverflow post:
// http://stackoverflow.com/questions/757153/concatenating-c-iterator-ranges-into-a-const-vector-member-variable-at-constru/757328#757328

#ifndef slimsig_concat_iterator_h
#define slimsig_concat_iterator_h
#include <iterator>
#include <type_traits>
#include <pair>
// compacted syntax for brevity...
template <typename Iterator, typename Traits = std::iterator_traits<Iterator>>
struct concat_iterator : std::iterator<
  typename Traits::iterator_category,
  typename Traits::value_type,
  typename Traits::difference_type,
  typename Traits::pointer_type,
  typename Traits::reference_type
  >
{
  using iterator_traits = std::iterator_traits<concat_iterator>;
  using iterator = Iterator;
public:
   using size_type = std::size_t;
   using range = std::pair<Iterator, Iterator>;
   using reference = typename iterator_traits::reference_type;
   using pointer = typename iterator_traits::pointer_type;
  
   concat_iterator(range first, range second)
      : m_range1(std::move(first)), m_range2(std::move(second)), m_current(first.first), m_is_first(true) {};
   concat_iterator(const concat_iterator& other) = default;
   concat_iterator(concat_iterator&&) = default;
   concat_iterator& operator=(const concat_iterator&) = default;
   concat_iterator& operator=(concat_iterator&&) = default;
   inline bool operator ==(const concat_iterator& other) const {
      return m_current == m_current;
   }
   inline bool operator !=(const concat_iterator& other) const{
    return !(*this == other);
   }
   inline bool operator >(const concat_iterator& other) const{
      return m_current > other.m_current;
   }
   inline bool operator >(const concat_iterator& other) const{
    return !(*this < other);
   }
   inline bool operator <=(const concat_iterator& other) const{
    return (*this == other) || (*this < other);
   }
   inline bool operator >=(const concat_iterator& other) const{
    return (*this == other) || (*this > other);
   }
  
   concat_iterator& operator++() {
      m_current++;
      if (m_is_first && m_current == range1.second) {
        m_current = range2.first;
        m_is_first = false;
      }
      return this;
   }
   concat_iterator operator++(int) {
    concat_iterator it(*this);
    ++(*this);
    return it;
   }
   concat_iterator& operator--() {
      using std::prev;
      if (!m_is_first && m_current == range2.first) {
        m_current = prev(range1.second);
        m_is_first = true;
      } else {
        m_current--;
      }
      return *this;
   }
   concat_iterator operator--(int) {
    concat_iterator it(*this);
    --(*this);
    return it;
   }
   concat_iterator& operator+=(difference_type offset)
   {
     if (offset > 0) {
        auto delta = m_is_first ? offset - std::distance(m_current, range1.second) : -1;
        if (delta >= 0) {
          m_is_first = false;
          m_current = m_range2.first + delta;
        } else {
          m_current += offset;
        }
        return *this;
     } else {
      return *this -= -offset;
     }
   }
   concat_iterator& operator-=(difference_type offset)
   {
    if (offset > 0) {
      auto delta = !m_is_first ? offset - (std::distance(m_current, range2.first)) : -1;
      if (delta >= 1) {
        m_is_first = true;
        m_current = range1.second - delta;
      } else {
        m_current -= offset;
      }
      return *this;
    } else {
      return *this += -offset;
    }
   }
   inline reference operator*() const{
     return *m_current;
   }
   inline pointer operator->() const{
      return m_current.operator->();
   }
   concat_iterator operator+(difference_type offset) const{
      concat_iterator it(*this);
      it += offset;
      return it;
   }
   concat_iterator operator-(difference_type offset) const {
      concat_iterator it(*this);
      it -= offset;
      return it;
   }
   reference operator[](size_type offset) const{
      concat_iterator it(*this);
      it += offset;
      return *it;
   }
  
  
private:
   range m_range1;
   range m_range2;
   iterator m_current;
   bool m_is_first;
};

template <typename T1, typename T2>
concat_iterator<T1,T2> concat_begin( T1 b1, T1 e1, T2 b2, T2 e2 )
{
   return concat_iterator<T1,T2>(b1,e1,b2,e2);
}
template <typename T1, typename T2>
concat_iterator<T1,T2> concat_end( T1 b1, T1 e1, T2 b2, T2 e2 )
{
   return concat_iterator<T1,T2>(e1,e1,e2,e2);
}

#endif
