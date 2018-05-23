/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/

#include "qczmq.h"

///
//  Copy-construct to return the proper wrapped c types
QZloop::QZloop (zloop_t *self, QObject *qObjParent) : QObject (qObjParent)
{
    this->self = self;
}


///
//  Create a new zloop reactor
QZloop::QZloop (QObject *qObjParent) : QObject (qObjParent)
{
    this->self = zloop_new ();
}

///
//  Destroy a reactor
QZloop::~QZloop ()
{
    zloop_destroy (&self);
}

///
//  Register socket reader with the reactor. When the reader has messages, 
//  the reactor will call the handler, passing the arg. Returns 0 if OK, -1
//  if there was an error. If you register the same socket more than once, 
//  each instance will invoke its corresponding handler.                   
int QZloop::reader (QZsock *sock, zloop_reader_fn handler, void *arg)
{
    int rv = zloop_reader (self, sock->self, handler, arg);
    return rv;
}

///
//  Cancel a socket reader from the reactor. If multiple readers exist for
//  same socket, cancels ALL of them.                                     
void QZloop::readerEnd (QZsock *sock)
{
    zloop_reader_end (self, sock->self);
    
}

///
//  Configure a registered reader to ignore errors. If you do not set this,
//  then readers that have errors are removed from the reactor silently.   
void QZloop::readerSetTolerant (QZsock *sock)
{
    zloop_reader_set_tolerant (self, sock->self);
    
}

///
//  Register low-level libzmq pollitem with the reactor. When the pollitem  
//  is ready, will call the handler, passing the arg. Returns 0 if OK, -1   
//  if there was an error. If you register the pollitem more than once, each
//  instance will invoke its corresponding handler. A pollitem with         
//  socket=NULL and fd=0 means 'poll on FD zero'.                           
int QZloop::poller (zmq_pollitem_t *item, zloop_fn handler, void *arg)
{
    int rv = zloop_poller (self, item, handler, arg);
    return rv;
}

///
//  Cancel a pollitem from the reactor, specified by socket or FD. If both
//  are specified, uses only socket. If multiple poll items exist for same
//  socket/FD, cancels ALL of them.                                       
void QZloop::pollerEnd (zmq_pollitem_t *item)
{
    zloop_poller_end (self, item);
    
}

///
//  Configure a registered poller to ignore errors. If you do not set this,
//  then poller that have errors are removed from the reactor silently.    
void QZloop::pollerSetTolerant (zmq_pollitem_t *item)
{
    zloop_poller_set_tolerant (self, item);
    
}

///
//  Register a timer that expires after some delay and repeats some number of
//  times. At each expiry, will call the handler, passing the arg. To run a  
//  timer forever, use 0 times. Returns a timer_id that is used to cancel the
//  timer in the future. Returns -1 if there was an error.                   
int QZloop::timer (size_t delay, size_t times, zloop_timer_fn handler, void *arg)
{
    int rv = zloop_timer (self, delay, times, handler, arg);
    return rv;
}

///
//  Cancel a specific timer identified by a specific timer_id (as returned by
//  zloop_timer).                                                            
int QZloop::timerEnd (int timerId)
{
    int rv = zloop_timer_end (self, timerId);
    return rv;
}

///
//  Register a ticket timer. Ticket timers are very fast in the case where   
//  you use a lot of timers (thousands), and frequently remove and add them. 
//  The main use case is expiry timers for servers that handle many clients, 
//  and which reset the expiry timer for each message received from a client.
//  Whereas normal timers perform poorly as the number of clients grows, the 
//  cost of ticket timers is constant, no matter the number of clients. You  
//  must set the ticket delay using zloop_set_ticket_delay before creating a 
//  ticket. Returns a handle to the timer that you should use in             
//  zloop_ticket_reset and zloop_ticket_delete.                              
void * QZloop::ticket (zloop_timer_fn handler, void *arg)
{
    void * rv = zloop_ticket (self, handler, arg);
    return rv;
}

///
//  Reset a ticket timer, which moves it to the end of the ticket list and
//  resets its execution time. This is a very fast operation.             
void QZloop::ticketReset (void *handle)
{
    zloop_ticket_reset (self, handle);
    
}

///
//  Delete a ticket timer. We do not actually delete the ticket here, as    
//  other code may still refer to the ticket. We mark as deleted, and remove
//  later and safely.                                                       
void QZloop::ticketDelete (void *handle)
{
    zloop_ticket_delete (self, handle);
    
}

///
//  Set the ticket delay, which applies to all tickets. If you lower the   
//  delay and there are already tickets created, the results are undefined.
void QZloop::setTicketDelay (size_t ticketDelay)
{
    zloop_set_ticket_delay (self, ticketDelay);
    
}

///
//  Set hard limit on number of timers allowed. Setting more than a small  
//  number of timers (10-100) can have a dramatic impact on the performance
//  of the reactor. For high-volume cases, use ticket timers. If the hard  
//  limit is reached, the reactor stops creating new timers and logs an    
//  error.                                                                 
void QZloop::setMaxTimers (size_t maxTimers)
{
    zloop_set_max_timers (self, maxTimers);
    
}

///
//  Set verbose tracing of reactor on/off. The default verbose setting is
//  off (false).                                                         
void QZloop::setVerbose (bool verbose)
{
    zloop_set_verbose (self, verbose);
    
}

///
//  By default the reactor stops if the process receives a SIGINT or SIGTERM 
//  signal. This makes it impossible to shut-down message based architectures
//  like zactors. This method lets you switch off break handling. The default
//  nonstop setting is off (false).                                          
void QZloop::setNonstop (bool nonstop)
{
    zloop_set_nonstop (self, nonstop);
    
}

///
//  Start the reactor. Takes control of the thread and returns when the 0MQ  
//  context is terminated or the process is interrupted, or any event handler
//  returns -1. Event handlers may register new sockets and timers, and      
//  cancel sockets. Returns 0 if interrupted, -1 if canceled by a handler.   
int QZloop::start ()
{
    int rv = zloop_start (self);
    return rv;
}

///
//  Self test of this class.
void QZloop::test (bool verbose)
{
    zloop_test (verbose);
    
}
/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
