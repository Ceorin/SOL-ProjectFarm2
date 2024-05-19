#include "signal_utils.h"
#include "utils.h"

// masks every signal; returns old mask
sigset_t mask_all() {
    sigset_t mask, oldmask;
    test_error(sigfillset(&mask), -1, "Creating signal mask");
    test_error(pthread_sigmask(SIG_SETMASK, &mask, &oldmask), -1, "Setting signal mask");
    return oldmask;
}

// unmask all, probably don't use
void unmask_all() {
    sigset_t mask;
    test_error(sigemptyset(&mask), -1, "Creating unmask_all");
    test_error(pthread_sigmask(SIG_SETMASK, &mask, NULL), -1, "Setting unmask_all");
}

// set oldmask. Usually call after mask_all() or the behaviour might be undefined
void return_mask(sigset_t oldmask) {
    test_error(pthread_sigmask(SIG_SETMASK, &oldmask, NULL), -1, "Resetting mask");
}