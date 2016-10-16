/**
 * Slimmer Signals
 * Copyright (c) 2014 Christopher Tarquini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

/**
 * Slimmer Signals
 * Inspired by Boost::Signals2 and ssig
 *
 * Main Differences between this library and all the other signal libraries:
 * - Light-weight: No unessarry virtual calls besides the ones inherit with std::function
 * - Uses vectors as the underlying storage
 *    The theory is that you'll be emitting signals far more than you'll be adding slots
 *    so using vectors will improve performance greatly by playing nice with the CPUs cache and taking
 *    advantage of SIMD
 * - Supports adding/removing slots while the signal is running (despite it being a vector)
 * - Removing slots should still be fast as it uses std::remove_if to iterate/execute slots
 *    Meaning that if there are disconnected slots they are removed quickly after iteration
 * - Custom allocators for more performance tweaking
 *
 * While still being light-weight it still supports:
 * - connections/scoped_connections
 * - connection.disconnect()/connection.connected() can be called after the signal stops existing
 *    There's a small overhead because of the use of a shared_ptr for each slot, however, my average use-case
 *    Involves adding 1-2 slots per signal so the overhead is neglible, especially if you use a custom allocator such as boost:pool
 *
 *  To keep it simple I left out support for slots that return values, though it shouldn't be too hard to implement.
 *  I've also left thread safety as something to be handled by higher level libraries
 *  Much in the spirit of other STL containers. My reasoning is that even with thread safety sort of baked in
 *  The user would still be responsible making sure slots don't do anything funny if they are executed on different threads
 *  All the mechanics for this would complicate the library and really not solve much of anything while also slowing down
 *  the library for applications where signals/slots are only used from a single thread
 *
 *  Plus it makes things like adding slots to multiple signals really slow because you have to lock each and every time
 *  You'd be better off using your own syncronization methods to setup all your singals at once and releasing your lock after
 *  you're done
 */
#ifndef slimsignals_h
#define slimsignals_h

#include <slimsig/detail/signal_base.h>

namespace slimsig {
  template <class Handler, class SignalTraits = signal_traits<Handler>, class Allocator = std::allocator<std::function<Handler>>>
  class signal : private signal_base<SignalTraits, Allocator, Handler> {
  public:
    using base = signal_base<SignalTraits, Allocator, Handler>;
    using typename base::return_type;
    using typename base::callback;
    using typename base::allocator_type;
    using typename base::slot;
    using typename base::slot_list;
    using typename base::list_allocator_type;
    using typename base::const_slot_reference;
    using typename base::connection_type;
    using base::arity;
    using base::argument;
    // allocator constructor
    using base::base;

    // default constructor
    signal() : signal(allocator_type()) {};
    using base::emit;
    using base::connect;
    using base::connect_once;
    using base::connect_extended;
    using base::disconnect_all;
    using base::slot_count;
    using base::get_allocator;
    using base::empty;
    using base::swap;
    using base::max_size;
    using base::max_depth;
    using base::get_depth;
    using base::is_running;
    using base::remaining_slots;

  };
  template <
    class Handler,
    class SignalTraits = signal_traits<Handler>,
    class Allocator = std::allocator<std::function<Handler>>
  > using signal_t = signal<Handler, SignalTraits, Allocator>;

}
#endif
