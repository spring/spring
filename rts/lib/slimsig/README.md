slimsig
=======
![slimsig](http://i.imgur.com/rpC5pot.png?1)

A light-weight alternative to Boost::Signals2 and SigSlot++.

## What makes this different from all the other Signal/Slot libraries out there?
 - Light-weight: Only one header file, <200 lines of code. No dependencies (unless you're running the unit tests)
 - Uses vectors as the underlying storage.
   
    This is a very important and disguinshing factor between slimsig and other libraries.
    The idea is that your signal will spend much more time iterating and executing slots than adding/removing them.
    Most implementations use a double or singly linked list which kill performance on modern CPUs by completely trashing the cache and eliminating the CPU's ability to optimize and use SIMD instructions. This is a big deal!

 - Less memory allocations: Because we use a vector instead of a list, we can re-use memory we've already allocated from deleted slots for future slots without hitting the heap again. STL's linked list allocates and deallocates every time you add/remove an item. Bad for cache locality, bad for fragmentation, bad for your soul!
 - Supports adding/removing slots while the signal is running (despite it being a vector)
 - Removing slots should still be very fast. It uses std::remove_if to iterate/execute slots
    Meaning that if there are disconnected slots they are removed quickly after iteration
 - Custom allocators for more performance tweaking

## How to use it
`slimgsig` is a header only library. Just add `include/slimsig/slimgsig.h` to your project and you're good to go. I'd reccomend adding the path to `include` to your header search directories so you can include it as `<slimgsig/slimgsig.h>` similiar to how you would use boost and other libraries.

## How to use with gyp
If you're using gyp you can simply add `"includes": ["path/to/slimsig.gypi"]` to your target and the appropriate header search paths will be added for you

## How to run the unit tests
![unit tests](http://i.imgur.com/5AriT6o.png)
If you using gyp just execute `$ gyp --depth=./ slimsig.gyp` from the command line to generate an project file for your platform. It defaults to xcode on OSX, Visual Studio on Windows and a makefile on linux/posix systems but you can choose which one you want with `-f [projecttype]`. See the gyp documentation for more details.

Once you've got your project just open it up and build the `test-runner` target and run it. We use bandit as a unit-test framework so you can execute a bunch of neat commands like `./test-runner --reporter=spec --only connection` to only run connection related tests and show the output as in a neat doc-like format. For more information see the [bandit documentation](http://banditcpp.org/).

## How do I get started with slimsignal? 
The usage is pretty much the same as Boost::Signals2. There's a `signal` class, `connection`, and `scoped_connection` that work in very much the same way minus a lot of the extra (but heavy) features boost includes such as object tracking or signals that return values.

I'll write up some proper documentation soon but for now check out `test/test.cpp` for some examples.
 
## While still being light-weight it still supports:
 - connections/scoped_connections
 - connection.disconnect()/connection.connected() can be called after the signal stops existing 
    There's a small overhead because of the use of a shared_ptr for each slot, however, my average use-case
    Involves adding 1-2 slots per signal so the overhead is neglible, especially if you use a custom allocator such as boost:pool

  To keep it simple I left out support for slots that return values, though it shouldn't be too hard to implement.
  I've also left thread safety as something to be handled by higher level libraries
  Much in the spirit of other STL containers. My reasoning is that even with thread safety sort of baked in
  the user would still be responsible making sure slots don't do anything funny if they are executed on different threads.
  All the mechanics for this would complicate the library, confuse users into thinking that your syncronization problems are magically sorted, and slow it down considerably even if you aren't using threads.
 Syncronization decisions are very application specific and simply don't belong in a basic building blocks library like this. 
 
## Inspiration 
- Boost::Signals2 - Beautiful, powerful, but heavy-weight Signal/Slot library that does everything but your taxes
- SigSlot++ 
- ssig - Another light-weight implementation, original inspiration for sigslim

## Benchmarks
- Coming soon! Maybe!
  
 
 
