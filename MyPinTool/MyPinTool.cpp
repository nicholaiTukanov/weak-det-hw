/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <stdio.h>

#include "pin.H"
#include "instlib.H"
using std::string;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "mytool.out", "specify output file name");

//==============================================================
//  Analysis Routines
//==============================================================
// Note:  threadid is used as an argument to the PIN_GetLock()
//        routine as a debugging aid.  This is the value that
//        the lock is set to, so it must be non-zero.

// lock serializes access to the output file.
FILE * out;
PIN_LOCK pinLock;

PIN_LOCK check_avail; // serialize access to lock_available vector
PIN_LOCK update_table; // serialize access to time_queue vector

INSTLIB::ICOUNT icount;
int total_threads = 0;


/* our defines */
#include <vector>
#include <map>
#include <iostream>
#include <iterator>
#include <algorithm>

typedef struct {
    THREADID threadid;
    UINT64 time;
} time_struct;

std::vector< void * > prg_locks;

// lock = # of times lock was seen
std::map< void *, int > lock_counter;

// lock = threads in queue
std::map< void *, std::vector<time_struct> > time_queue;

// lock availability
std::map< void *, bool > lock_available;

// thread to lock
std::map< THREADID, void *> thread_to_lock;

void print_instruction_count() {
    for (int i = 0; i < total_threads; i++) {
        // cout << "T:" << i << " " << icount.Count(i) << "\t";
    }
    // cout << endl;
}

/*
    comparision function

    the queues are ordered by the weak-det definition in kendo

    the thread that comes first in the queue statisfies the following:
    1. the thread is the oldest
    2. the thread has the lowest thread id

*/
bool compare_time_struct(time_struct i1, time_struct i2) {
    if (i1.time == i2.time)
        return i1.threadid < i2.threadid;
    else
        return i1.time > i2.time;
}

void print_time_struct(time_struct thread_ts) {
    // cout << "THREAD ID = " << thread_ts.threadid << "\tINSTR COUNT = " << thread_ts.time << endl;
}

THREADID get_thread_to_be_released(void *lock) {


    PIN_GetLock(&update_table, 0);
    // get the element that is at the front of the queue for the given lock
    time_struct thread_time_struct = time_queue[lock][0];

    // print_time_struct(thread_time_struct);

    // this thread is the one that is allowed
    THREADID threadid = thread_time_struct.threadid;

    // delete the thread time struct off the queue since the thread will not acquire the lock
    time_queue[lock].erase(time_queue[lock].begin());

    PIN_ReleaseLock(&update_table);
    return threadid;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// Note that opening a file in a callback is only supported on Linux systems.
// See buffer-win.cpp for how to work around this issue on Windows.
//
// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "thread begin %d\n",threadid);
    fflush(out);
    total_threads++;
    PIN_ReleaseLock(&pinLock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    // check if thread holds a lock in the map
    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "thread end %d code %d\n",threadid, code);
    fflush(out);
    PIN_ReleaseLock(&pinLock);
}

// called before pthread_mutex_lock
VOID BeforeLOCK(void *lock, THREADID threadid )
{

    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "pthread_mutex_lock: thread %d attempts to grab lock %p \n", threadid, lock);
    fflush(out);
    PIN_ReleaseLock(&pinLock);


    // table lookup, grab table lock
    PIN_GetLock(&update_table, threadid);
    // cout << "thread " << threadid << " has grabbed update_table" << endl;
    bool lock_in_map = (time_queue.find(lock) != time_queue.end());
    // cout << "thread " << threadid << " is about to release update_table" << endl;
    PIN_ReleaseLock(&update_table);
    

    // lock is in map
    if (lock_in_map) {

        // cout << "lock exists in table" << endl;
        // lock_counter[lock] += 1;

        // table entry, grab table lock
        PIN_GetLock(&update_table, threadid);
        time_struct new_struct;
        new_struct.threadid = threadid;
        new_struct.time = icount.Count(threadid);
        time_queue[lock].push_back(new_struct);
        std::sort(time_queue[lock].begin(), time_queue[lock].end(), compare_time_struct);
        PIN_ReleaseLock(&update_table);
    }


    // lock is NOT in map
    else {

        // cout << "lock doesn't exists in table" << endl;
        // lock_counter.insert(std::pair<void *, int>(lock, 1));

        // first time we have seen the lock, thus add it to lock available map
        // grab lock since lookup is made
        PIN_GetLock(&check_avail, threadid);
        lock_available.insert(std::pair<void *, bool>(lock, true));
        PIN_ReleaseLock(&check_avail);


        // entry will be added, grab lock
        PIN_GetLock(&update_table, threadid);
        time_struct new_struct;
        new_struct.threadid = threadid;
        new_struct.time = icount.Count(threadid);

        std::vector<time_struct> new_queue;
        new_queue.push_back(new_struct);

        time_queue.insert(std::pair<void *, std::vector<time_struct> >(lock, new_queue));
        PIN_ReleaseLock(&update_table);

        return;
    }

    thread_to_lock.insert(std::pair<THREADID, void *>(threadid, lock));

    // all threads will wait at the spin lock if the lock is NOT available
    // this code will be run by many threads
    while(true){
        
        // make sure that only 1 thread checks the 
        PIN_GetLock(&check_avail, threadid);
        bool lock_released = lock_available[lock];
        
        PIN_ReleaseLock(&check_avail);
        
        // lock has been released
        // all threads waiting for the lock check if they can be released
        if(lock_released) {
            
            THREADID thread_to_be_released = get_thread_to_be_released(lock);
            
            // cout << "Current Thread " << threadid << endl;

            // only break the thread that is allowed to pass through
            if (threadid == thread_to_be_released)
                break;
        }
    }

}

// called after pthread_mutex_lock returns
VOID AfterLOCK(int ret_val, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "pthread_mutex_lock: thread %d returned %d \n", threadid, ret_val);
    fflush(out);
    PIN_ReleaseLock(&pinLock);

    PIN_GetLock(&check_avail, threadid);
    void* lock = thread_to_lock[threadid];
    thread_to_lock.erase(threadid);
    lock_available[lock] = false; // since we have returned from pthread_mutex_lock, we know that the lock is NOT available
    PIN_ReleaseLock(&check_avail);
    
    //thread_to_lock.erase(threadid);

    // while(true) {



    //     // if lock was released
    //     if (lock_available[lock] == true) {
    //         //// cout << "Lock status: " << lock_available[lock] << " ";
    //         lock_available[lock] = false; // thread grabs lock
    //         // cout << "Lock: " << lock << " Thread: " << threadid << " Instructions: " << time_queue[lock].at(0).time << " aquired  lock" << endl;
    //         time_queue[lock].erase(time_queue[lock].begin());
    //         break;
    //     }
    // }


    
}

// called before pthread_mutex_unlock
VOID BeforeUNLOCK(void *lock, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "pthread_mutex_unlock: thread %d releases lock %p \n", threadid, lock);
    fflush(out);
    PIN_ReleaseLock(&pinLock);

    PIN_GetLock(&check_avail, threadid);
    lock_available[lock] = true;
    PIN_ReleaseLock(&check_avail);
}

// called after pthread_mutex_unlock returns
VOID AfterUNLOCK(int ret_val, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid);
    fprintf(out, "pthread_mutex_unlock: thread %d returned %d \n", threadid, ret_val);
    fflush(out);
    PIN_ReleaseLock(&pinLock);

    
    //void* lock = thread_to_lock[threadid];
    //thread_to_lock.erase(threadid);


    // cout  << " Thread: " << threadid << " Released lock" << endl;

    
    
}



//====================================================================
// Instrumentation Routines
//====================================================================


typedef struct{
    RTN rtn;
    string func_name;
}RTN_info;



// This routine is executed for each image.
VOID ImageLoad(IMG img, VOID *)
{

    RTN rtn1 = RTN_FindByName(img, "pthread_mutex_lock"),
        rtn2 = RTN_FindByName(img, "pthread_mutex_unlock");

    if ( RTN_Valid( rtn1 ))
    {
        RTN_Open(rtn1);
        
        RTN_InsertCall(rtn1, IPOINT_BEFORE, AFUNPTR(BeforeLOCK),
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_THREAD_ID, IARG_END);

        RTN_InsertCall(rtn1, IPOINT_AFTER, AFUNPTR(AfterLOCK),
                       IARG_FUNCRET_EXITPOINT_VALUE, 
                       IARG_THREAD_ID, IARG_END);

        RTN_Close(rtn1);
    }

    if ( RTN_Valid( rtn2 ))
    {
        RTN_Open(rtn2);
        
        RTN_InsertCall(rtn2, IPOINT_BEFORE, AFUNPTR(BeforeUNLOCK),
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_THREAD_ID, IARG_END);

        RTN_InsertCall(rtn2, IPOINT_AFTER, AFUNPTR(AfterUNLOCK),
                       IARG_FUNCRET_EXITPOINT_VALUE, 
                       IARG_THREAD_ID, IARG_END);

        RTN_Close(rtn2);
    }

}

// This routine is executed once at the end.
VOID Fini(INT32 code, VOID *v)
{

    // print map out
    //std::ap<int, int>::iterator itr;
  //   for (auto itr = lock_counter.begin(); itr != lock_counter.end(); ++itr) {
    //    std::// cout << '\t' << itr->first
    //         << '\t' << itr->second << '\n';
   // }


    fclose(out);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of pthread_mutex_lock calls in the guest application\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(INT32 argc, CHAR **argv)
{
    // Initialize the pin lock
    PIN_InitLock(&pinLock);
    
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();
    
    // file that the output will be stored int
    out = fopen(KnobOutputFile.Value().c_str(), "w");
    icount.Activate(INSTLIB::ICOUNT::ModeNormal);

    // Register ImageLoad to be called when each image is loaded.
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Analysis routines to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // cout << "starting to run program" << endl;
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
