/*************************************************************/
/* Assumption                                                */
/* 1. There is only one condition variable and only one lock */
/* The Queue is specific to this condition variable          */
/* which means that threads that are waiting on different    */
/* condition variables will be added to different Queues     */
/*                                                           */
/* 2. Assuming that load_signal() and set_signals()          */
/* are internal mechanism that can be called to notifiy the  */
/* waiting thread that the signal is available. If we do not */
/* assume this, we need to write the implementation          */
/* to notify the wait method that the signal is available    */
/*************************************************************/

/*************************************************************/
/* The queue holds the threads that are waiting              */
/* on the condition variable.                                */
/*************************************************************/
class Queue
{
public:
    int front, rear, size;
    unsigned capacity;
    int *array;
};

/*************************************************************/
/* CreateQueue operation will return a queue with given size */
/*************************************************************/
Queue *createQueue(unsigned capacity)
{
    Queue *queue = new Queue();
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1; // This is important, see the enqueue
    queue->array = new int[(queue->capacity * sizeof(int))];
    return queue;
}

/*************************************************************/
/* enqueue operation checks if the queue is full if not adds */
/* the element to the queue                                  */
/*************************************************************/
void enqueue(Queue *queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    cout << item << " enqueued to queue\n";
}

/*************************************************************/
/* dequeue operation checks if the queue is empty if not     */
/* removes the element form the queue                        */
/*************************************************************/
int dequeue(Queue *queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

/*************************************************************/
/* wait operation on the condition                           */
/*************************************************************/
thread_wait_new(pthread_cond_t *cond, pthread_mutex_t *mutex)
{

    /*************************************************************/
    /* Adds the calling thread to the waiters list               */
    /*************************************************************/
    thread_id = pthread_self();
    Queue *waiters_queue = createQueue(thread_id).test_and_set();
    waiters_queue.enqueue();

    /*************************************************************/
    /* Since the thread is in the waiters queue now it is safe   */
    /* to release the mutex on the thread. However, releasing    */
    /* the mutex has to be an atomic operation                   */
    /*************************************************************/
    thread_unlock_internal(mutex, 0);

    /*************************************************************/
    /* We load all the signals received at this point            */
    /* load_signals() catches any signals set by singnal method  */
    /*************************************************************/
    signals = load_signals().test_and_set();

    /*************************************************************/
    /* We wait to receive the signal on the condition variable   */
    /*************************************************************/
    while (1)
    {
        if (signals != 0)
        {
            /*****************************************************/
            /* Once the signal is received dequeue each thread   */
            /* elements from the Queue acquire mutex for them    */
            /* and wake them up                                  */
            /*****************************************************/
            while (Queue.size() > 0)
            {
                /*************************************************/
                /* Remove the thread from the waiting queue      */
                /*************************************************/
                threadId = Queue.dequeue();

                /*************************************************/
                /* Now is the perfect time to acquire the lock   */
                /* on the thread. However, it has to be an       */
                /* atomic operation.                             */
                /*************************************************/
                thread_lock_internal(threadId, mutex);

                /*************************************************/
                /* wake up the thread for execution              */
                /*************************************************/
                wake_up_thread(threadId);
            }
        }

        /*********************************************************/
        /* At this point we load the new signals we may've       */
        /* received                                              */
        /*********************************************************/
        signals = load_signals().test_and_set();
    }
}

/****************************************************************/
/* signal operation on the condition                            */
/****************************************************************/
thread_signal_new(pthread_cond_t *cond)
{
    /************************************************************/
    /* A buffer to have the memory for signal operations        */
    /************************************************************/
    Array signal_buffer;

    /********************************************************/
    /* Spawn a thread to see if signal_buffer gets populated*/
    /********************************************************/
    pthread_create(check_for_signal_buffer());

    /************************************************************/
    /* If already there are threads waiting on the conditions   */
    /* variable in the Queue. notify them by setting signal     */
    /************************************************************/
    if (Queue.size() != 0)
    {
        /********************************************************/
        /* Notifying the load_signals() method in wait thread   */
        /********************************************************/
        set_signals();
    }
    else
    {

        /********************************************************/
        /* If currently there are no threads waiting on the     */
        /* condition variable which means that the Queue is     */
        /* empty then save the signal and deliver it later      */
        /* when there is a thread wait on the condition variable*/
        /* in the future.                                       */
        /********************************************************/
        signal_buffer.add(signals)
            .test_and_set();
    }
}

check_for_signal_buffer()
{
    /***********************************************************/
    /* Spint to check if the signal buffer ever gets           */
    /* populated                                               */
    /***********************************************************/
    while (1)
    {
        if (signal_buffer.size() != 0)
        {
            set_signals().test_and_set();
            /****************************************************/
            /* Swap context to the original thread that spawned */
            /* this thread                                      */
            /****************************************************/
            swapcontext();
        }
    }
}

/*--------------------------------------------------------------*/
#include <mutex>
#include <condition_variable>
std::mutex mtx;
std::condition_variable _cond;

void Player(ThreadID tid, GameBarrier gb, ConsoleBarrier cb)
{
    gb.waitToPlay();
    cb.waitAtConsole();
    play();
    gb.donePlaying();
}

class GameBarrier
{
    /****************************************************/
    /* Declare all possible states for GameBarrier      */
    /* as specified in the question                     */
    /****************************************************/
    int GAME_NOTREADY = 0;
    int GAME_FILLING = 1;
    int GAME_FILLED = 2;
    int waitingQueueCount;
    int playerQueueCount;
    int barrier_state;

    GameBarrier()
    {
        barrier_state = GAME_NOTREADY;
        waitingQueueCount = 0;
        playerQueueCount = 0;
    }

    void waitToPlay()
    {
        /****************************************************/
        /* Player arrives we wait till we get 4 waiters     */
        /****************************************************/
        mtx.lock(); // Lock.acquire
        waitingQueueCount++;
        if (playerQueueCount >= 4 && barrier_state == GAME_NOTREADY)
        {
            /****************************************************/
            /* Update the barrier state as per the question     */
            /****************************************************/
            barrier_state = GAME_FILLING;

            /****************************************************/
            /* As soon as we haave 4 waiters we notify          */
            /* all threads                                      */
            /* noify call unblocks all of them                  */
            /****************************************************/
            _cond.notify_all(); // equivalent broadcast
        }

        /****************************************************/
        /* Untill we dont have 4 waiters the threads are    */
        /* blocked in waiting                               */
        /****************************************************/
        while (barrier_state != GAME_FILLING)
        {
            _cond.wait(mtx);
        }

        /****************************************************/
        /* If the thread was unblocked which means that     */
        /* 4 waiters                                        */
        /* were promoted as players we need to decrement    */
        /* the waiting queue                                */
        /* and increment the player queye                   */
        /****************************************************/
        waitingQueueCount--;
        playerQueueCount++;

        /****************************************************/
        /* As soon as there are 4 players update barrier    */
        /* state at this point console barrier will take    */
        /* control of all the varibles so release the lock * /
        /****************************************************/
        if (playerQueueCount == 4)
        {
            barrier_state = GAME_FILLED;
            mtx.unlock() // Lock.release()
        }
    }

    void donePlaying()
    {

        mtx.lock(); // Lock.acquire
        /****************************************************/
        /* Reduce player count for each thread when the game*/
        /* has finished                                     */
        /****************************************************/
        playerQueueCount--;
        if (playerQueueCount = 0)
        {
            /****************************************************/
            /* Update the barrier state to not ready again      */
            /****************************************************/
            barrier_state = GAME_NOTREADY;

            /****************************************************/
            /* As 4 players are done playing we check meanwhile */
            /* were new player threads spawned and if they are  */
            /* 4 in count update barrier_state and unblock all  */
            /* the waiting player thereads                      */
            /****************************************************/
            if (waitingQueueCount >= 4)
            {
                barrier_state = GAME_FILLING;
                _cond.notify_all(); // equivalent broadcast
            }
        }
        mutex.release();
    }
};

class ConsoleBarrier
{
    int CONSOLE_WAIT = 0;
    int CONSOLE_ALLOW = 1;
    int console_state;
    int playersCount;

    ConsoleBarrier()
    {
        console_state = CONSOLE_WAIT;
        playersCount = 0;
    }

    void waitAtConsole()
    {
        mtx.lock(); // Lock.acquire
        playersCount++;

        /****************************************************/
        /* Four player threads have arrived to console      */
        /****************************************************/
        if (playersCount == 4)
        {
            /************************************************/
            /* Unblock all threads waiting at the console   */
            /************************************************/
            state = CONSOLE_ALLOW;
            _cond.notify_all(); // equivalent broadcast
        }
        /************************************************/
        /* Player thread waits at console unless        */
        /* 4 players arrive                             */
        /************************************************/
        while (state == CONSOLE_WAIT)
        {
            _cond.wait(mtx);
        }

        /****************************************************/
        /* Player thread entered the game reduce the count  */
        /****************************************************/
        playersCount--;
        if (playersCount == 0)
            state = CONSOLE_WAIT;
        mutex.unlock(); // Lock.release()
    }
};
