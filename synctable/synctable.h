#ifndef __SYNCHRONIZER__H__
#define __SYNCHRONIZER__H__

#include <unordered_map>
#include <mutex>

namespace synclock{
    class SyncTable{
        private:
            std::unordered_map<void *, std::mutex *> locks_table;
            std::mutex table_lock; 

        public:
            SyncTable(){}
            // I might remove the delete's but it seems like a bad idea
            // to have the same entries in multiple tables.  I can't think
            // of why you would want that on purpose
            SyncTable(const SyncTable &) = delete;
            SyncTable & operator=(const SyncTable &) = delete;
            ~SyncTable();
 
            std::mutex * get_lock_address(void *addr);
    };
    
    // This class is only for use by the synchronized/tablesynchronized blocks
    // and should not be used directly.  The name of the class is
    // intentionlly poorly formed.
    class _Table_Locker{
        private:
            std::lock_guard<std::mutex> var_lock_holder;

        public:
            bool finished;
            _Table_Locker(SyncTable &sync_table, void * addr);
            _Table_Locker(const _Table_Locker &) = delete;
            _Table_Locker & operator=(const _Table_Locker &) = delete;
            ~_Table_Locker();
    };

    // global table for use in synchronized blocks
    extern SyncTable globalsynctable;
}

// the _Table_Lockers have a bunch of capital letters on the end of them
// to (try to) ensure there are no collisions

// tablesynchronized(synctable, &var) { critical section }
//
// tablesynchronized block takes in a table and an address, locking the
// address for the body of the block, but only in the given table
// this is provided so that groups of unrelated threads do not result in a
// large, slow, globalsynctable.
// using a value in a local SyncTable will NOT add it to tho global synctable
// this is exception safe since the _Table_Locker releases the lock on
// destruction

#define tablesynchronized(TABLE, ADDR)  \
for(synclock::_Table_Locker _table_locker_obj_ABCDEFAOEUI(TABLE, (void*)(ADDR)); \
        !_table_locker_obj_ABCDEFAOEUI.finished; \
        _table_locker_obj_ABCDEFAOEUI.finished = true)


// synchronized(&var) { critical section }
//
// synchronized blocks construct a _Table_Locker on entry and destroy it
// on exit.  This results in a locking of var for the body of the block.
// It is also exception safe since destructon occurs when an exception
// causes the block to exit
#define synchronized(ADDR)  \
for(struct {const std::lock_guard<std::mutex> & lg; bool finished;} pair = \
        {std::lock_guard<std::mutex>( \
                *synclock::globalsynctable.get_lock_address( \
                    static_cast<void *>(ADDR))), \
            false}; \
        !pair.finished; \
        pair.finished = true)

#endif // __SYNCHRONIZER__H__
