//
//  connection.h
//  slimsig
//
//  Created by Christopher Tarquini on 4/21/14.
//
//

#ifndef slimsig_connection_h
#define slimsig_connection_h
#include <memory>
#include <functional>
#include <algorithm>
#include <limits>
#include <slimsig/detail/slot.h>

namespace slimsig {
template <class Signal>
class connection;

template <class ThreadPolicy, class Allocator, class F>
class signal_base;
 // detail

  template <class Signal>
  class connection {
  using slot = typename Signal::slot;
  using slot_type = slot;
  using slot_list_iterator = typename Signal::slot_list::iterator;
  using slot_storage = typename Signal::slot_list;
  using slot_id = typename slot::slot_id;
  using signal_holder = typename Signal::signal_holder;
  connection(std::weak_ptr<signal_holder> slots, const slot_type& slot) : connection(std::move(slots), slot.m_slot_id)  {};
  connection(std::weak_ptr<signal_holder> slots, unsigned long long slot_id) : m_slots(std::move(slots)), m_slot_id(slot_id) {};
  public:
    connection() {}; // empty connection
    connection(const connection& other) : m_slots(other.m_slots), m_slot_id(other.m_slot_id) {};
    connection(connection&& other) : m_slots(std::move(other.m_slots)), m_slot_id(other.m_slot_id) {};

    connection& operator=(connection&& rhs) {
      this->swap(rhs);
      return *this;
    }
    connection& operator=(const connection& rhs) {
      m_slot_id = rhs.m_slot_id;
      m_slots = rhs.m_slots;
      return *this;
    }

    void swap(connection& other) {
      using std::swap;
      swap(m_slots, other.m_slots);
      swap(m_slot_id, other.m_slot_id);
    }
#ifdef __GNUC__
    __attribute__((always_inline))
#endif
    inline explicit operator bool() const { return connected(); };
    bool connected() const {
      const auto slots = m_slots.lock();
      if (slots && slots->signal != nullptr) {
        return  slots->signal->connected(m_slot_id);
        //auto slot = (*slots)->find(m_slot_id);
        //return slot != (*slots)->cend() ? slot->connected() : false;
      }
      return false;
    }

    void disconnect() {
      std::shared_ptr<signal_holder> slots = m_slots.lock();
      if (slots != nullptr && slots->signal != nullptr) {
        slots->signal->disconnect(m_slot_id);
        //auto slot = *slots->find(m_slot_id);
        //if (slot != slots->end()) slot->disconnect();
      }
    }
    template <class ThreadPolicy, class Allocator, class F>
    friend class signal_base;
    template < class T, class IDGenerator, class FlagType, class Allocator>
    friend class slot_list;


  private:
    std::weak_ptr<signal_holder> m_slots;
    slot_id m_slot_id;
  };

  template <class connection>
  class scoped_connection {
  public:
    scoped_connection() : m_connection() {};
    scoped_connection(const connection& target) : m_connection(target) {};
    scoped_connection(const scoped_connection&) = delete;
    scoped_connection(scoped_connection&& other) : m_connection(other.m_connection) {};

    scoped_connection& operator=(const connection& rhs) {
      m_connection = rhs;
    };
    scoped_connection& operator=(scoped_connection&& rhs) {
      this->swap(rhs);
      return *this;
    };
    void swap(scoped_connection& other) {
      m_connection.swap(other.m_connection);
    };
    connection release() {
      connection ret{};
      m_connection.swap(ret);
      return ret;
    }
    ~scoped_connection() {
      m_connection.disconnect();
    }
  private:
    connection m_connection;
  };

  template <class connection>
  scoped_connection<connection> make_scoped_connection(connection&& target) {
    return scoped_connection<connection>(std::forward<connection>(target));
  };

}

#endif
