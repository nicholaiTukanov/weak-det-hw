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
// Note:  threadid+1 is used as an argument to the PIN_GetLock()
//        routine as a debugging aid.  This is the value that
//        the lock is set to, so it must be non-zero.

// lock serializes access to the output file.
FILE * out;
PIN_LOCK pinLock, update_lock;
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

std::vector<void *> prg_locks;

// lock = # of times lock was seen
std::map<void *, bool> lock_available;

// lock = threads in queue
std::map<void *, std::vector<time_struct> > time_queue;

std::map<THREADID, void *> thread_to_lock;

// map lock -> queue
// queue each element -> struct(threadid, instruction_count)
// sorted based on instruction_count


void print_instruction_count() {
    for (int i = 0; i < total_threads; i++) {
        std::cout << "T:" << i << " " << icount.Count(i) << "\t";
    }
    std::cout << endl;
}

bool compare_time_struct(time_struct i1, time_struct i2) {
    //return i1.time < i2.time;
    return i1.threadid < i2.threadid;
}


// Note that opening a file in a callback is only supported on Linux systems.
// See buffer-win.cpp for how to work around this issue on Windows.
//
// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "thread begin %d\n",threadid);
    fflush(out);
    total_threads++;
    PIN_ReleaseLock(&pinLock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "thread end %d code %d\n",threadid, code);
    fflush(out);
    PIN_ReleaseLock(&pinLock);
}

// called before pthread_mutex_lock
VOID BeforeLOCK(void *lock, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "pthread_mutex_lock: thread %d attempts to grab lock %p \n", threadid, lock);
    fflush(out);
    cout << threadid << endl;
    PIN_ReleaseLock(&pinLock);
    //print_instruction_count();
    //cout << "T:" << threadid << " " << icount.Count(threadid) << endl;

    // put threads on a 2d queue (dim1 = lock ptr, dim2 = time) 
    // sort the threads based on logical time
    // when to pop queue? (1- lock has been released?)
    // in any case, we need to have a sorted queue where the method of sorting provides weak-det
    
    PIN_GetLock(&update_lock, 0);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock << "grabbed before updating data structures\n";
    if (time_queue.find(lock) != time_queue.end()) {

        time_struct new_struct;
        new_struct.threadid = threadid;
        new_struct.time = icount.Count(threadid);

        time_queue[lock].push_back(new_struct);
        std::sort(time_queue[lock].begin(), time_queue[lock].end(), compare_time_struct);
        
    }
    else {

        time_struct new_struct;
        new_struct.threadid = threadid;
        new_struct.time = icount.Count(threadid);

        std::vector<time_struct> new_queue;
        new_queue.push_back(new_struct);

        time_queue.insert(std::pair<void *, std::vector<time_struct> >(lock, new_queue));
    
    	cout << "Thread id: " << threadid << "  Actual lock: " << lock << " Added\n";
        lock_available.insert(std::pair<void *, bool> (lock, true));
    }
    thread_to_lock.insert(std::pair<THREADID, void *> (threadid, lock));
    PIN_ReleaseLock(&update_lock);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock << " released after updating data structures\n";


    int print_count = 0;
    while (true) {
        PIN_GetLock(&update_lock, 0);
        if (print_count < 5) {
        	cout << "(in loop) Thread id: " << threadid << " Lock: " << lock << endl;
        	print_count++;
        }
        //cout << "Thread id: " << threadid << " Lock: " << &update_lock << "grabbed before lock\n";
        if (lock_available[lock] == true) {
            //PIN_GetLock(&update_lock, 0);
            THREADID released_tid = time_queue[lock][0].threadid;
            time_queue[lock].erase(time_queue[lock].begin());
            //cout << "To be released tid: " << released_tid << "\tCurrent tid:  " << threadid << endl;
            //PIN_ReleaseLock(&update_lock);
            if (released_tid == threadid) {
            	PIN_ReleaseLock(&update_lock);
            	cout << "Thread id: " << threadid << " Lock: " << &update_lock << " released before lock\n";
                break;
            }
        }
        PIN_ReleaseLock(&update_lock);
        //cout << "Thread id: " << threadid << " Lock: " << &update_lock << "released before lock\n";
    }

    //std::cout << "Lock address: " << lock << endl;
    //for(unsigned i = 0; i < time_queue[lock].size(); i++) {
    //    std::cout << "Thread: " << time_queue[lock].at(i).threadid << " " << time_queue[lock].at(i).time << "\n";
    //}
    //std::cout << endl;


    //spinning logic
    // when no lock , pop the thread

    
}

// called after pthread_mutex_lock returns
VOID AfterLOCK(int ret_val, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "pthread_mutex_lock: thread %d returned %d \n", threadid, ret_val);
    fflush(out);
    PIN_ReleaseLock(&pinLock);

    PIN_GetLock(&update_lock, 0);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock << " grabbed in after lock\n";
    void *lock = thread_to_lock[threadid];
    //cout << "Lock: " << lock << " with thread id: " << threadid << endl;
    thread_to_lock.erase(threadid);
    cout << "Thread id: " << threadid << "  Actual lock: " << lock << " Set to false\n";
    lock_available[lock] = false;
    PIN_ReleaseLock(&update_lock);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock <<  " released in after lock\n";
}

// called before pthread_mutex_unlock
VOID BeforeUNLOCK(void *lock, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "pthread_mutex_unlock: thread %d releases lock %p \n", threadid, lock);
    fflush(out);
    PIN_ReleaseLock(&pinLock);

    PIN_GetLock(&update_lock, 0);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock << " grabbed in before unlock\n";
    cout << "Thread id: " << threadid << "  Actual lock: " << lock << " Set to true\n";
    lock_available[lock] = true;
    PIN_ReleaseLock(&update_lock);
    cout << "Thread id: " << threadid << " Lock: " << &update_lock << " released in before unlock\n";
}

// called after pthread_mutex_unlock returns
VOID AfterUNLOCK(int ret_val, THREADID threadid )
{
    PIN_GetLock(&pinLock, threadid+1);
    fprintf(out, "pthread_mutex_unlock: thread %d returned %d \n", threadid, ret_val);
    fflush(out);
    PIN_ReleaseLock(&pinLock);
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
    //    std::cout << '\t' << itr->first
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
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
