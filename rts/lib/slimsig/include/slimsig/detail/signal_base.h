//
//  signal_base.h
//  slimsig
//
//  Created by Christopher Tarquini on 4/21/14.
//
//

#ifndef slimsig_signal_base_h
#define slimsig_signal_base_h

#include <functional>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cmath>
#include <cassert>

#include <slimsig/connection.h>
namespace slimsig {

template <class Handler>
struct signal_traits;

template <class R, class... Args>
struct signal_traits<R(Args...)> {
  using return_type = R;
  using slot_id_type = std::size_t;
  using depth_type = unsigned;
};

template <class Handler, class SignalTraits, class Allocator>
class signal;

template <class SignalTraits, class Allocator, class Handler>
class signal_base;

template <class Handler, class SignalTraits, class Allocator>
void swap(signal<Handler, SignalTraits, Allocator>& lhs, signal<Handler, SignalTraits, Allocator>& rhs);

namespace detail {
  template <class Container, class Callback>
  inline void each(const Container& container, typename Container::size_type begin, typename Container::size_type end, const Callback& fn)
  {
    for(; begin != end; begin++) {
      fn(container[begin]);
    }
  }
  template <class Container, class Callback>
  inline void each_n(const Container& container, typename Container::size_type begin, typename Container::size_type count, const Callback& fn)
  {
    return each<Container, Callback>(container, begin, begin + count, fn);
  }
}
template<class SignalTraits, class Allocator, class R, class... Args>
class signal_base<SignalTraits, Allocator, R(Args...)>
{
  struct emit_scope;
public:
  using signal_traits = SignalTraits;
  using return_type = typename signal_traits::return_type;
  using callback = std::function<R(Args...)>;
  using allocator_type = Allocator;
  using slot = basic_slot<R(Args...), typename signal_traits::slot_id_type>;
  using list_allocator_type = typename std::allocator_traits<Allocator>::template rebind_traits<slot>::allocator_type;
  using slot_list = std::vector<slot, list_allocator_type>;

  using connection_type = connection<signal_base>;
  using extended_callback = std::function<R(connection_type& conn, Args...)>;
  using slot_id = typename signal_traits::slot_id_type;
  using slot_reference = typename slot_list::reference;
  using const_slot_reference = typename slot_list::const_reference;
  using size_type = std::size_t;
public:
  static constexpr auto arity = sizeof...(Args);
  struct signal_holder {
    signal_holder(signal_base* p) : signal(p) {};
    signal_base* signal;
  };
  template <std::size_t N>
  struct argument
  {
    static_assert(N < arity, "error: invalid parameter index.");
    using type = typename std::tuple_element<N,std::tuple<Args...>>::type;
  };

  // allocator constructor
  signal_base(const allocator_type& alloc) :
    pending(alloc),
    m_self(nullptr),
    last_id(),
    m_size(0),
    m_offset(0),
    allocator(alloc),
    m_depth(0){};

  signal_base(size_t capacity, const allocator_type& alloc = allocator_type{})
  : signal_base(alloc) {
    pending.reserve(capacity);
  };

  signal_base(signal_base&& other) {
    this->swap(other);
  }
  signal_base& operator=(signal_base&& other) {
    this->swap(other);
    return *this;
  }
  // no copy
  signal_base(const signal_base&) = delete;
  signal_base& operator=(const signal_base&) = delete;
  void swap(signal_base& rhs) {
    using std::swap;
    using std::back_inserter;
    using std::copy_if;
    using std::not1;
    if (this != &rhs) {
    #if !defined(NDEBUG) || (defined(SLIMSIG_SWAP_GUARD) && SLIMSIG_SWAP_GUARD)
      if (is_running() || rhs.is_running())
        throw new std::logic_error("Signals can not be swapped or moved while emitting");
    #endif
      swap(pending, rhs.pending);
      swap(m_self, rhs.m_self);
      if (m_self) m_self->signal = this;
      if (rhs.m_self) rhs.m_self->signal = &rhs;
      swap(last_id, rhs.last_id);
      swap(m_size, rhs.m_size);
      swap(m_offset, rhs.m_offset);
      if (std::allocator_traits<allocator_type>::propagate_on_container_swap::value)
        swap(allocator, rhs.allocator);
      swap(m_depth, rhs.m_depth);
    }
  }

  return_type emit(Args... args) {
    using detail::each;
    // scope guard
    emit_scope scope { *this };

    auto end = pending.size();
    assert(m_offset <= end);
    if (end - m_offset == 0) return;
    assert(end > 0);
    --end;
    for(; m_offset != end; m_offset++) {
      const_slot_reference slot = pending[m_offset];
      if (slot) slot(args...);
    }
    auto& slot = pending[end];
    if (slot)  slot(std::forward<Args&&>(args)...);
  }
  return_type operator()(Args... args) {
    return emit(std::forward<Args&&>(args)...);
  }

  inline connection_type connect(callback slot)
  {
    auto sid = prepare_connection();
    emplace(sid, std::move(slot));
    return { m_self, sid };
  };

  inline connection_type connect_extended(extended_callback slot)
  {
    struct extended_slot {
      callback fn;
      connection_type connection;
      R operator()(Args&&... args) {
        return fn(connection, std::forward<Args>(args)...);
      }
    };
    return create_connection<extended_slot>(std::move(slot));
  }

  template <class TP, class Alloc>
  inline connection_type connect(std::shared_ptr<signal<R(Args...), TP, Alloc>> signal) {
    using signal_type = slimsig::signal<R(Args...), TP, Alloc>;
    struct signal_slot {
      std::weak_ptr<signal_type> handle;
      connection_type connection;
      R operator()(Args&&... args) {
        auto signal = handle.lock();
        if (signal) {
          return signal->emit(std::forward<Args>(args)...);
        } else {
          connection.disconnect();
          return;
        }
      }
    };
    return create_connection<signal_slot>(std::move(signal));
  }

  connection_type connect_once(callback slot)  {
    struct fire_once {
      callback fn;
      connection_type conn;
      R operator() (Args&&... args) {
        auto scoped_connection = make_scoped_connection(std::move(conn));
        return fn(std::forward<Args>(args)...);
      }
    };
    return create_connection<fire_once>(std::move(slot));
  }


  void disconnect_all() {
    using std::for_each;
    if (is_running()) {
      m_offset = pending.size();
      m_size = 0;
    } else {
      pending.clear();
      m_size = 0;
    }
    // if we've used up a lot of id's we can take advantage of the fact that
    // all connections are based on the current signal_holder pointer
    // and reset our last_id to 0 without causing trouble with old connections
    if (m_self && last_id > std::numeric_limits<slot_id>::max() / 3) {
      m_self->signal = nullptr;
      m_self.reset();
      last_id = slot_id();
    }
  }

  const allocator_type& get_allocator() const {
    return allocator;
  }
  inline bool empty() const {
    return m_size == 0;
  }
  inline size_type slot_count() const {
    return m_size;
  }
  inline size_type max_size() const {
    return std::min(std::numeric_limits<slot_id>::max(), pending.max_size());
  }
  inline size_type remaining_slots() const {
    return max_size() - last_id;
  }

  size_type max_depth() const {
    return std::numeric_limits<unsigned>::max();
  }
  size_type get_depth() const {
    return m_depth;
  }
  bool is_running() const {
    return m_depth > 0;
  }

  ~signal_base() {
    if (m_self) m_self->signal = nullptr;
  }
  template <class FN, class TP, class Alloc>
  friend class signal;
  template <class Signal>
  friend class slimsig::connection;
private:
  struct emit_scope{
    signal_base& signal;
    emit_scope(signal_base& context) : signal(context) {
      signal.m_depth++;
    }
    emit_scope() = delete;
    emit_scope(const emit_scope&) = delete;
    emit_scope(emit_scope&&) = delete;
    ~emit_scope() {
      using std::move;
      using std::for_each;
      using std::remove_if;
      auto depth = --signal.m_depth;
      // if we completed iteration (depth = 0) collapse all the levels into the head list
      if (depth == 0) {
        auto m_size = signal.m_size;
        auto& pending = signal.pending;
        // if the size is different than the expected size
        // we have some slots we need to remove
        if (m_size != pending.size()) {
          // remove slots from disconnect_all
          pending.erase(pending.begin(), pending.begin() + signal.m_offset);
          pending.erase(remove_if(pending.begin(), pending.end(), &is_disconnected), pending.end());
        }
        signal.m_offset = 0;
        assert(m_size == pending.size());
      }
    }
  };

  static bool is_disconnected(const_slot_reference slot) {  return !bool(slot); };

  inline bool connected(slot_id index)
  {
    using std::lower_bound;
    auto end = pending.cend();
    auto slot = lower_bound(pending.cbegin() + m_offset, end, index, [] (const_slot_reference slot, const slot_id& index) {
      return slot < index;
    });
    if (slot != end && slot->m_slot_id == index) return slot->connected();
    return false;
  };

  inline void disconnect(slot_id index)
  {
    using std::lower_bound;
    auto end = pending.end();
    auto slot = lower_bound(pending.begin() + m_offset, end, index, [] (slot_reference slot, const slot_id& index) {
      return slot < index;
    });
    if (slot != end && slot->m_slot_id == index) {
      if (slot->connected()) {
       slot->disconnect();
       m_size -= 1;
      }

    }
  };

  template<class C, class T>
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline connection_type create_connection(T&& slot)
  {
    auto sid = prepare_connection();
    emplace(sid, C { std::move(slot), {m_self, sid} });
    return connection_type { m_self, sid };
  };
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline slot_id prepare_connection()
  {
    // lazy initialize to put off heap allocations if the user
    // has not connected a slot
    if (!m_self) m_self = std::make_shared<signal_holder>(this);
    assert((last_id < std::numeric_limits<slot_id>::max() - 1) && "All available slot ids for this signal have been exhausted. This may be a sign you are misusing signals");
    return last_id++;
  };

  template <class... SlotArgs>
#ifdef __GNUC__
  __attribute__((always_inline))
#endif
  inline void emplace(SlotArgs&&... args)
  {
    pending.emplace_back(std::forward<SlotArgs>(args)...);
    m_size++;
  };
protected:
  std::vector<slot> pending;
private:
  std::shared_ptr<signal_holder> m_self;
  slot_id last_id;
  std::size_t m_size;
  std::size_t m_offset;
  allocator_type allocator;
  unsigned m_depth;

};

  template <class Handler, class ThreadPolicy, class Allocator>
  void swap(signal<Handler, ThreadPolicy, Allocator>& lhs, signal<Handler, ThreadPolicy, Allocator>& rhs) {
    using std::swap;
    lhs.swap(rhs);
  }
}

#endif
