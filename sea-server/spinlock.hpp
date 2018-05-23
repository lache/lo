#pragma once
namespace ss {
    class spinlock {
    private:
        typedef enum { Locked, Unlocked } LockState;
        boost::atomic<LockState> state_;

    public:
        spinlock() : state_(Unlocked) {}

        void lock() {
            while (state_.exchange(Locked, boost::memory_order_acquire) == Locked) {
                /* busy-wait */
            }
        }
        void unlock() {
            state_.store(Unlocked, boost::memory_order_release);
        }
    };
}