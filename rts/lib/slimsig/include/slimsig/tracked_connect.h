//
//  tracked_connect.h
//  slimsig
//
//  Created by Christopher Tarquini on 4/22/14.
//
//

#ifndef slimsig_tracked_connect_h
#define slimsig_tracked_connect_h
#include <memory>
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <type_traits>
#include <functional>
namespace slimsig {
  template <class T, class Observer, class BaseAllocator = std::allocator<T>>
  class trackable_allocator : private BaseAllocator {
    using allocator_traits = std::allocator_traits<BaseAllocator>;
    using base = BaseAllocator;
    template<class U>
    using parent_rebind_traits_t = typename allocator_traits::template rebind_traits<U>;
    template <class U>
    using parent_rebind_t = typename parent_rebind_traits_t<U>::allocator_type;
  public:
    using typename allocator_traits::value_type;
    using typename allocator_traits::pointer;
    using typename allocator_traits::const_pointer;
    using typename allocator_traits::void_pointer;
    using typename allocator_traits::const_void_pointer;
    using typename allocator_traits::difference_type;
    using typename allocator_traits::size_type;
    using typename allocator_traits::propagate_on_container_copy_assignment;
    using typename allocator_traits::propagate_on_container_move_assignment;
    using typename allocator_traits::propagate_on_container_swap;
    trackable_allocator() : base() {};
    template <class U, class O = Observer>
    trackable_allocator(const trackable_allocator<U, Observer>& other) : base(other), m_observer(other.m_observer)  {};
    using base::allocate;
    using base::deallocate;
    using base::construct;
    
    template <class U>
    void destroy(U* p) {
      m_observer(p);
      allocator_traits::destroy(*this, p);
    }
  private:
    Observer m_observer;
  };
  template <class T, class Observer, class Deleter = std::default_delete<T>>
  struct trackable_delete {
    constexpr trackable_delete() noexcept = default;
    template <class U, class OtherObserver = Observer, class OtherDeleter = Deleter, std::enable_if<std::is_convertible<U*, T*>::value, bool> = true>
    trackable_delete(const trackable_delete<U, OtherObserver, OtherDeleter>& other) : m_observer(other.m_observer), m_deleter(m_deleter) {};
    trackable_delete(Observer observer = Observer{}, Deleter deleter = Deleter{}) : m_observer(std::move(observer)), m_deleter(std::move(deleter)){};
    trackable_delete(const trackable_delete&) = default;
    trackable_delete(trackable_delete&&) = default;
    trackable_delete& operator=(const trackable_delete&) = default;
    trackable_delete& operator=(trackable_delete&&) = default;
    void operator()(T* ptr) const {
      m_observer(ptr);
      m_deleter(ptr);
    }
    const Deleter& get_deleter() const {
      return m_deleter;
    }
    Deleter& get_deleter() {
      return m_deleter;
    }
    const Observer& get_observer() const {
      return m_observer;
    }
    Observer& get_observer() {
      return m_observer;
    }
  private:
    Observer m_observer;
    Deleter m_deleter;
  };
  
  template <class T, class Observer, class Deleter = std::default_delete<T>>
  using trackable_ptr = std::unique_ptr<T, trackable_delete<T, Observer, Deleter>>;
  
  template <class T, class Observer, class Deleter = std::default_delete<T>, class TrackableDeleter = trackable_delete<T, Observer, Deleter>, class... Args>
  std::shared_ptr<T> make_trackable(TrackableDeleter deleter, Args&&... args) {
    return std::shared_ptr<T> ( deleter, T{std::forward<Args>(args)...} );
  }
  template <class T, class Observer, class Deleter = std::default_delete<T>, class TrackableDeleter = trackable_delete<T, Observer, Deleter>, class... Args>
  std::shared_ptr<T> make_trackable(Observer observer, Args&&... args) {
    return std::shared_ptr<T> ( TrackableDeleter { observer }, T{std::forward<Args>(args)...} );
  }
  template <class T, class Observer, class Deleter = std::default_delete<T>, class TrackableDeleter = trackable_delete<T, Observer, Deleter>, class... Args>
  std::shared_ptr<T> make_trackable(Observer observer, Deleter deleter, Args&&... args) {
    return std::shared_ptr<T> ( TrackableDeleter { observer, deleter }, T{std::forward<Args>(args)...} );
  }
  template <class T, class Observer, class Allocator = std::allocator<T>, class TrackableAllocator = trackable_allocator<T, Observer, Allocator>, class... Args>
  std::shared_ptr<T> allocate_trackable(TrackableAllocator allocator, Args&&... args) {
    return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
  }
  template <class T, class Observer, class Allocator = std::allocator<T>, class TrackableAllocator = trackable_allocator<T, Observer, Allocator>, class... Args>
  std::shared_ptr<T> allocate_trackable(Observer observer, Args&&... args) {
    return std::allocate_shared<T>(TrackableAllocator {observer}, std::forward<Args>(args)...);
  }
  template <class T, class Observer, class Allocator = std::allocator<T>, class TrackableAllocator = trackable_allocator<T, Observer, Allocator>, class... Args>
  std::shared_ptr<T> allocate_trackable(Observer observer, Allocator allocator, Args&&... args) {
    return std::allocate_shared<T>(TrackableAllocator {observer, allocator}, std::forward<Args>(args)...);
  }
  
 
  template <class T>
  class trackable_lock {
  public:
    using weak_ptr_type = std::weak_ptr<T>;
    trackable_lock(std::vector<std::weak_ptr<T>> trackable_objects) : m_tracking(std::move(trackable_objects)) {};
    trackable_lock(std::initializer_list<std::weak_ptr<T>> trackable_list) : m_tracking{std::move(trackable_list)} {}
    /*template <class Iterator>
    weak_ptr(Iterator begin, Iterator end) : m_tracking(begin, end) {}*/
    
    bool try_lock() {
      m_locked.reserve(m_tracking.size());
      
    }
  private:
    std::vector<std::weak_ptr<T>> m_tracking;
    std::vector<std::shared_ptr<T>> m_locked;
  };
}

#endif
