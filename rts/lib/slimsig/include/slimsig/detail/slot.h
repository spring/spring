//
//  signal_set.h
//  slimsig
//
//  Created by Christopher Tarquini on 4/23/14.
//
//

#ifndef slimsig_signal_set_h
#define slimsig_signal_set_h

#include <vector>
#include <iterator>
#include <algorithm>
#include <type_traits>

namespace slimsig {
namespace detail {

template <class T>
#ifdef __GNUC__
__attribute__((always_inline))
#endif
inline T default_value() { return T(); }
template<>
#ifdef __GNUC__
__attribute__((always_inline))
#endif
inline void default_value<void>() {};
};

template <class Callback, class SlotID>
class basic_slot;

template <class R, class... Args, class SlotID>
class basic_slot<R(Args...), SlotID> {
public:
  using callback = std::function<R(Args...)>;
  using slot_id = SlotID;

  basic_slot(slot_id sid, callback fn) : m_fn(std::move(fn)), m_slot_id(sid), m_is_connected(bool(m_fn)), m_is_running(false) {};
  basic_slot() : basic_slot(0, nullptr) {};
  template <class... Arguments>
  basic_slot(slot_id sid, Arguments&&... args) : m_fn(std::forward<Arguments>(args)...), m_slot_id(sid), m_is_connected(bool(m_fn)), m_is_running(false) {};
  basic_slot(basic_slot&&) = default;
  basic_slot(const basic_slot&) = default;
  inline basic_slot& operator=(const basic_slot&) = default;
  inline basic_slot& operator=(basic_slot&&) = default;
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator==(const basic_slot& other) const{
    return m_slot_id == other.m_slot_id;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator==(const slot_id& other) const {
    return m_slot_id == other;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator <(const basic_slot& other) const {
    return m_slot_id < other.m_slot_id;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator <(const slot_id& other) const {
    return m_slot_id < other;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator >(const basic_slot& other) const {
    return m_slot_id > other.m_slot_id;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator >(const slot_id& other) const {
    return m_slot_id > other;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator <=(const slot_id& other) const {
    return m_slot_id <= other;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool operator >=(const slot_id& other) const {
    return m_slot_id >= other;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline explicit operator bool()  const{
   return m_is_connected;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline bool connected() const {
    return m_is_connected;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline void disconnect() {
    m_is_connected = false;
    if (!m_is_running) {
      m_fn = nullptr;
    }
  }
#ifdef __GNUC__
  __attribute__((always_inline, deprecated("Use operator()")))
#endif
  inline const callback& operator*() const{
    return m_fn;
  };
#ifdef __GNUC__
  __attribute__((always_inline, deprecated("Use operator()")))
#endif
  inline const callback* operator->() const {
    return &m_fn;
  }
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline R operator() (Args... args) const{
    struct invoke_guard
    {
      const basic_slot& slot;
      ~invoke_guard() { slot.m_is_running = false;}
    } guard { *this };
    m_is_running = true;
    return m_fn(std::forward<Args>(args)...);
  }
  callback m_fn;
  slot_id m_slot_id;
  bool m_is_connected;
  mutable bool m_is_running;
};

/**
 *  Specialized container adaptor for slot/callback values
 *  Because of some assumptions we can make about how signals will use
 *  this list, we can achieve great performance boosts over using a standard containers alone
 *  Requirements:
 *  T must meet all requirements of Container
 *  SlotID must be post-incrementable, equality comparable and less than comparable
 *  SlotID must always be unique for new values added to the list
 *  SlotID must always increase in value for new values (new slots must compare greater than old slots)
 *  SlotID must support default initilization
 *  The return of SlotID++ will be stored with each slot for access
 *
 *  We use this type of lookup for slots because we need a way to access slots after the vectors
 *  iterators have been invalidated, but we need to do this quickly.
 *
 *  To accomplish this we use a simple linear search if the container contains less than 4 elements
 *  Otherwise we use std::lower_bound to quickly locate the slot with this ID in the vector
 *  This requires the vector to be sorted with lower IDs (older slots) first
 *
 *  Because generally slots aren't removed or queried nearly as much as they are emitted
 *  Applications will see huge performance gains over using a list or a vector of shared_ptr's
 *  due to the memory being contigious. Additions are pretty snappy in most cases because we don't
 *  need to make allocations for every slot, only when we've exceeded our underlying vectors capacity
 *
 *  In multithreaded applications it's reccomended that SlotIDGenerator be an atomic type or use mutexes to syncronize.
 *
 *  This would be better with some sort of Optional type
 *
 */
template < class T,
           class IDGenerator = std::size_t,
           class FlagType = bool,
           class Allocator = std::allocator<std::function<T>>
         >
class slot_list : public std::enable_shared_from_this<slot_list<T, IDGenerator, FlagType, Allocator>>{
  using id_generator = IDGenerator;
  using flag = FlagType;
  static auto slot_id_helper(IDGenerator gen) -> decltype(gen++);
public:
  using slot_id = decltype(slot_id_helper(std::declval<IDGenerator>()));
  using slot = basic_slot<T, slot_id>;
  using callback = typename slot::callback;
  using allocator_type = typename std::allocator_traits<Allocator>::template rebind_traits<slot>::allocator_type;
  using container_type = std::vector<slot, allocator_type>;
  using value_type = typename container_type::value_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using size_type = typename container_type::size_type;
  using pointer = typename container_type::pointer;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  slot_list() : active(), pending(), last_id(1), is_locked(false)  {};
  slot_list(allocator_type alloc) : active{alloc}, pending{std::move(alloc)}, last_id(1), is_locked(false) {};
  slot_list(slot_list&&) = default;
  slot_list(const slot_list&) = default;

  bool try_lock() { lock(); return true; }
  void lock() { is_locked = true; }


  void unlock() {
    using std::make_move_iterator;
    using std::sort;

    struct guard_t{
      flag& locked;
      container_type& pending;
      ~guard_t() { locked = false; pending.clear();};
    } guard { is_locked, pending };
    // sort offset in case of error
    auto offset = active_size() - !active_empty();
    try {
      active.insert(active.end(), make_move_iterator(pending.begin()), make_move_iterator(pending.end()));
    } catch (...) {
      // make sure we are still sorted properly
      sort(active.begin() + offset, active.end());
      throw;
    }
  }

  bool locked() const noexcept (noexcept(bool(std::declval<FlagType>()))) { return is_locked; }

  slot_id push(const value_type& value) {
    using std::move;
    auto& queue = !is_locked ? active : pending;
    auto sid = last_id++;
    queue.emplace_back({ sid, move(value)});
    return sid;
  }

  slot_id push(value_type&& value) {
    using std::move;
    auto& queue = !is_locked ? active : pending;
    auto sid = last_id++;
    queue.emplace_back({ sid, move(value)});
    return sid;
  }

  template <class... Args>
  slot_id emplace(Args&&... args) {
    auto& queue = !is_locked ? active : pending;
    auto sid = last_id++;
    queue.emplace_back(sid, std::forward<Args>(args)...);
    return sid;
  }

  template <class U, class... Args>
  slot_id emplace_extended(Args&&... args) {
    auto& queue = !is_locked ? active : pending;
    auto sid = last_id++;
    queue.emplace_back(sid, U{std::forward<Args>(args)..., {this->shared_from_this(), sid}});
    return sid;
  }



  inline iterator erase(const_iterator position) { return active.erase(position); }
  inline iterator erase(const_iterator begin, const_iterator end) { return active.erase(begin, end); }

  inline iterator begin() { return active.begin(); }
  inline iterator end() { return active.end(); }
  inline const_iterator cbegin() const { return active.cbegin(); }
  inline const_iterator cend() const { return active.cend(); }
  inline size_type active_size() const { return active.size(); }
  inline reference back() { return active.back(); }
  inline const_reference cback() const { return active.cback(); }
  inline size_type pending_size() const { return pending_size(); }
  inline size_type total_size() const { return active.size() + pending.size(); }
  inline void clear() { active.clear(); pending.clear(); }
  inline bool active_empty() const { return active_size() == 0; }
  inline bool pending_empty() const { return pending_size() == 0; }
  inline bool empty() const { return total_size() == 0; }

  iterator find(slot_id index) noexcept {
    auto end = active.end();
    auto slot = find(index, active.begin(), end);
    if (slot == end) {
      auto pend = pending.end();
      slot = find(index, pending.begin(), pend);
      if (slot == pend) slot = end;
    }
    return slot;
  };

  const_iterator find(slot_id index) const noexcept{
    auto end = active.cend();
    auto slot = find(index, active.cbegin(), end);
    if (slot == end) {
      auto pend = pending.cend();
      slot = find(index, pending.cbegin(), pend);
      if (slot == pend) slot = end;
    }
    return slot;
  };
private:
  // for internal use only
  template <class Iterator>
  static Iterator find(slot_id index, Iterator begin, Iterator end) noexcept{
    using std::distance;
    using std::lower_bound;
    using std::find_if;
    using std::bind;
    using std::less_equal;

    begin = find_if(lower_bound(begin, end, index), end, [&] (const_reference slot){
      return index >= index;
    });
    return begin != end && *begin == index ? begin : end;
  };
  container_type active;
  container_type pending;
  id_generator last_id;
  flag is_locked;
};

}


#endif
