#include "common.h"
#include "quickSearch.h"
#include "cleanup.h"

int shared_memory_new ( char **addr, int projectid, int size, char *comment ) {

    key_t key = ftok(GETEKYDIR, projectid);
    if ( key < 0 )
        err_exit("ftok error");

    int shmid;
    shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0664);
    if ( shmid == -1 ) {
        if ( errno == EEXIST ) {
            pbred("shared memeory already exist <%s>", comment);
            shmid = shmget(key ,0, 0);
            pbred("reference shmid = %d", shmid);
        } else {
            perror("errno");
            err_exit("shmget error");
        }
    } else {
        pbgreen("Obtained shared memory successful shmid=%d <%s>", shmid, comment);
    }

    /* Do not to specific the address to attach
     * and attach for read & write*/
    if ( (*addr = shmat(shmid, 0, 0) ) == (void*)-1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
            err_exit("shmctl error");
        else {
            pbred("Attach shared memory failed <%s>", comment);
            pbgreen("remove shared memory identifier successful <%s>", comment);
        }

        char buf[100] = "shmat error: ";
        strcat ( buf, comment );
        err_exit(buf);

    } else {
        pbgreen("Attach to %p <%s>", addr, comment);
    }

    return shmid;
}

int shared_memory_for_google_translate(char **addr) {

    return shared_memory_new ( addr, PROJECTID, SHMSIZE, "google" );
}


int shared_memory_for_baidu_translate(char **addr) {

    return shared_memory_new ( addr, PROJECTID2, SHMSIZE, "baidu" );
}

int shared_memory_for_selection(char **addr) {

    return shared_memory_new ( addr, PROJECTID2+1, 10, "selection" );
}

int shared_memory_for_quickSearch(char **addr) {

    return shared_memory_new ( addr, PROJECTID2+2, SHMSIZE, "quickSearch" );
}

int shared_memory_for_keyboard_event(char **addr) {

    return shared_memory_new ( addr, PROJECTID2+3, 100, "keyboard_event" );
}

int shared_memory_for_mysql(char **addr) {

    return shared_memory_new ( addr, PROJECTID2+4, SHMSIZE, "mysql" );
}

int shared_memory_for_pic(char **addr) {

    return shared_memory_new ( addr, PIC_PROJECT, SHMSIZE, "mysql" );
}
