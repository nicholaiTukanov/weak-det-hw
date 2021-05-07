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

INSTLIB::ICOUNT icount;
int total_threads = 0;


/* our defines */
#include <vector>
#include <map>
#include <iostream>
#include <iterator>
#include <algorithm>


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


#include <pthread.h>
#include <list>


PIN_LOCK table_lock;
PIN_LOCK check_availability_lock;


/* 
    software implementation of hardware lock table.
    table uses ptr pthread_mutex_t to index into a vector of THREADIDs.
    the vector of THREADIDs represents a priorty queue.
    for now, priority is determined by the THREADID.
    this can be changed in the future but this is what gives us determinism.
    in other words, weak deteminism can be guarenteed by how THREADIDs 
    are sorted in the queue to grab the lock.
    henceforth, even though we don't know the order in which threads will
    attempt to grab a lock, but if we had the set of threads grabbing the lock
    then we can determine the exact order the threads will grab the lock.
*/
std::map< pthread_mutex_t *, std::vector<THREADID>> lock_table;

/* 
    this helps us know if a lock can be grabbed or not
    this structure tells the scheduler whether a thread can be let through
    or not.

    lock_availability[pthread_mutex_t *] == true, then some thread holds that lock
    otherwise, the lock is free to grab.
*/
std::map< pthread_mutex_t *, std::pair < THREADID, bool> > lock_availability;




/*

    This data structure represents our hardware table

    Each access to the structure is serialized. (global table)

    The only time a thread should be accessing this struct is
    1. adding itself to the queue
    2. removing itself from the queue

*/

bool compare_func(THREADID tid_1, THREADID tid_2) {
    return tid_1 < tid_2;
}

void print_queue(std::vector<THREADID> lock_queue) {

    int queue_size = lock_queue.size();
    if (queue_size > 0)
    {
        cout << " [ " << lock_queue[0];

        for (int i = 1; i < queue_size; i++)
            cout << ", " << lock_queue[i];
        cout << " ];";
    }
    else 
        cout << " [];";
    
}

void print_table()
{
    cout << endl;

    for (auto queue : lock_table)
    {
        cout << queue.first << " : ";
        print_queue((queue.second));
        cout << endl;
    }

    cout << endl;
}

void print_locks_avail()
{
    for (auto pair : lock_availability)
    {
        if (pair.second.second)
            cout << "lock " << pair.first << " is available" << endl;
        else {
            cout << "lock " << pair.first << " is NOT available and is held by thread " << pair.second.first << endl;
        }
    }
}


/*
    this function replaces pthread_mutex_lock

    ...
*/
int PTHREAD_LOCK(pthread_mutex_t *__mutex)
{

    THREADID tid = PIN_ThreadId();

    // we are about to do table look up
    // we must serialize the access
    PIN_GetLock(&table_lock, 0);

    // cout << "thread " << tid << " has called PTHREAD_LOCK" << endl;
    
    // cout << "thread " << tid << " has grabbed table_lock" << endl;

    // lock already exists in the map
    // thread must add themselves to the queue
    if (lock_table.find(__mutex) != lock_table.end())
    {
        // cout << "lock " << __mutex << " does exists in table" << endl;

        // grab respective queue
        std::vector<THREADID> *locks_queue = &lock_table[__mutex];

        // add new thread to queue
        locks_queue->push_back(tid);

        // resort queue
        // locks_queue->sort(compare_func);
        std::sort(locks_queue->begin(), locks_queue->end(), compare_func);

    }

    // no queue exists
    // create one and add the thread grabbing the lock to it
    else 
    {

        // cout << "lock " << __mutex << " does NOT exists in table" << endl;

        // create new queue for unseen lock
        std::vector<THREADID> new_queue;

        // add current thread to new queue
        new_queue.push_back(tid);

        // add queue to lock table
        lock_table.insert(std::pair< pthread_mutex_t *, std::vector<THREADID> > (__mutex, new_queue));

        // have a seperate data structure that tells us when a lock is available or not
        lock_availability.insert(std::pair<pthread_mutex_t*, std::pair<THREADID, bool>> (__mutex, std::pair<THREADID, bool>(tid,true)) );

    }

    
    // print_table();

    PIN_ReleaseLock(&table_lock);
    // //cout << "thread " << tid << " has released table_lock" << endl;

    // wait for a group of threads

    // long time_count = 0;
    // while(time_count < 1) {
    //     time_count++;
    // }

    PIN_Sleep(1);

    // possible ideas:
    // have a quanta for while loop
    // if quanta is hit, then have the thread holding the lock release it


    // scheduler goes here
    while(true) {

        PIN_GetLock(&check_availability_lock, 0);

        // cout << "thread " << tid << " is waiting to grab lock " << __mutex << endl;

        bool lock_avail = lock_availability[__mutex].second;

        if (lock_avail)
        {
            PIN_GetLock(&table_lock, 0);

            // lookup
            THREADID tid_to_release = *(lock_table[__mutex].begin());

            if (tid == tid_to_release)
            {
                // cout << "thread " << tid << " is trying to be released\n";
                lock_table[__mutex].erase(lock_table[__mutex].begin());
                
                lock_availability[__mutex].first = tid;
                lock_availability[__mutex].second = false;

                // print_locks_avail();
                // print_table();

                PIN_ReleaseLock(&check_availability_lock);
                PIN_ReleaseLock(&table_lock);
                return 0;
                // break;
                
            }

            PIN_ReleaseLock(&table_lock);
        }


        PIN_ReleaseLock(&check_availability_lock);
        PIN_Yield();
    }


    PIN_ReleaseLock(&table_lock);
    PIN_ReleaseLock(&check_availability_lock);
    return 0;
}


/* 
    this function replaces pthread_mutex_unlock

    ...
*/
int PTHREAD_UNLOCK(pthread_mutex_t *__mutex) 
{

    // THREADID tid = PIN_ThreadId();

    PIN_GetLock(&check_availability_lock, 0);

    // cout << "thread " << tid << " has called PTHREAD_UNLOCK" << endl;

    lock_availability[__mutex].first = -1;
    lock_availability[__mutex].second = true;
    // cout << "thread " << tid << " has released lock " << __mutex << endl;

    // print_locks_avail();

    PIN_ReleaseLock(&check_availability_lock);


    return 0;
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
        
        RTN_Replace(rtn1, AFUNPTR(PTHREAD_LOCK));

        RTN_Close(rtn1);
    }

    if ( RTN_Valid( rtn2 ))
    {
        RTN_Open(rtn2);
        
        RTN_Replace(rtn2, AFUNPTR(PTHREAD_UNLOCK));

        RTN_Close(rtn2);
    }

}

// This routine is executed once at the end.
VOID Fini(INT32 code, VOID *v)
{
    // fclose(out);
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
    PIN_InitLock(&table_lock);
    PIN_InitLock(&check_availability_lock);
    
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();
    
    // file that the output will be stored int
    out = fopen(KnobOutputFile.Value().c_str(), "w");
    // icount.Activate(INSTLIB::ICOUNT::ModeNormal);

    // Register ImageLoad to be called when each image is loaded.
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Analysis routines to be called when a thread begins/ends
    // PIN_AddThreadStartFunction(ThreadStart, 0);
    // PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // cout << "starting to run program" << endl;
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
