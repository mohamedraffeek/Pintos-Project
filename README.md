# 🔧 My Contribution  

This project extends the **PintOS synch primitives** (`semaphore`, `lock`, `condition variable`) to support **priority scheduling** and solve the **priority inversion problem**.  

## 🚀 What I Modified
- **Semaphore (`sema_down`, `sema_up`)**
  - Changed the waiters list to be **priority-ordered** instead of FIFO.
  - Always unblock the **highest-priority thread** when `sema_up` is called.
  - Added yielding logic so the CPU always runs the most important thread.

- **Lock**
  - Added a new field: `Max_virtual_priority` to keep track of the maximum priority among waiting threads.
  - Implemented **priority donation**:
    - When a high-priority thread waits for a lock, it donates its priority to the current lock holder.
    - When the lock is released, the holder’s priority is recalculated (priority restored).
  - Each thread maintains the list of locks it holds, making donation and rollback possible.

- **Condition Variable**
  - Modified `cond_wait` and `cond_signal` to also use **priority ordering** for waiter threads, not FIFO.

---

## 🎯 Problem Solved: Priority Inversion
**Priority inversion** occurs when:  
- A **high-priority thread** waits for a lock held by a **low-priority thread**.  
- A **medium-priority thread** keeps running, preventing the low-priority thread from releasing the lock.  
- Result: the high-priority thread is stuck, even though it should run first.  

✅ With **priority donation**, the low-priority thread temporarily inherits the higher priority, finishes its work quickly, and releases the lock. This guarantees that the system respects priority scheduling.  

---

## 📖 Example Scenario
- `T1` (Low priority = 10) holds a lock.  
- `T2` (High priority = 90) needs the lock → donates its priority to `T1`.  
- `T3` (Medium priority = 50) would normally preempt `T1`, but now `T1` has priority 90.  
- `T1` runs, releases the lock.  
- `T2` acquires it and continues.  
- `T1` priority returns to 10.  

---

## 📝 Notes
- These changes ensure **fair scheduling** under priority-based systems.  
- Core idea: *always unblock and run the highest-priority thread, and use donation to prevent inversion.*  
