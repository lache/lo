################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################

module CZMQ
  module FFI

    # process configuration and status
    # @note This class is 100% generated using zproject.
    class Zproc
      # Raised when one tries to use an instance of {Zproc} after
      # the internal pointer to the native object has been nullified.
      class DestroyedError < RuntimeError; end

      # Boilerplate for self pointer, initializer, and finalizer
      class << self
        alias :__new :new
      end
      # Attaches the pointer _ptr_ to this instance and defines a finalizer for
      # it if necessary.
      # @param ptr [::FFI::Pointer]
      # @param finalize [Boolean]
      def initialize(ptr, finalize = true)
        @ptr = ptr
        if @ptr.null?
          @ptr = nil # Remove null pointers so we don't have to test for them.
        elsif finalize
          @finalizer = self.class.create_finalizer_for @ptr
          ObjectSpace.define_finalizer self, @finalizer
        end
      end
      # @param ptr [::FFI::Pointer]
      # @return [Proc]
      def self.create_finalizer_for(ptr)
        Proc.new do
          ptr_ptr = ::FFI::MemoryPointer.new :pointer
          ptr_ptr.write_pointer ptr
          ::CZMQ::FFI.zproc_destroy ptr_ptr
        end
      end
      # @return [Boolean]
      def null?
        !@ptr or @ptr.null?
      end
      # Return internal pointer
      # @return [::FFI::Pointer]
      def __ptr
        raise DestroyedError unless @ptr
        @ptr
      end
      # So external Libraries can just pass the Object to a FFI function which expects a :pointer
      alias_method :to_ptr, :__ptr
      # Nullify internal pointer and return pointer pointer.
      # @note This detaches the current instance from the native object
      #   and thus makes it unusable.
      # @return [::FFI::MemoryPointer] the pointer pointing to a pointer
      #   pointing to the native object
      def __ptr_give_ref
        raise DestroyedError unless @ptr
        ptr_ptr = ::FFI::MemoryPointer.new :pointer
        ptr_ptr.write_pointer @ptr
        __undef_finalizer if @finalizer
        @ptr = nil
        ptr_ptr
      end
      # Undefines the finalizer for this object.
      # @note Only use this if you need to and can guarantee that the native
      #   object will be freed by other means.
      # @return [void]
      def __undef_finalizer
        ObjectSpace.undefine_finalizer self
        @finalizer = nil
      end

      # Create a new zproc.                                        
      # NOTE: On Windows and with libzmq3 and libzmq2 this function
      # returns NULL. Code needs to be ported there.               
      # @return [CZMQ::Zproc]
      def self.new()
        ptr = ::CZMQ::FFI.zproc_new()
        __new ptr
      end

      # Destroy zproc, wait until process ends.
      #
      # @return [void]
      def destroy()
        return unless @ptr
        self_p = __ptr_give_ref
        result = ::CZMQ::FFI.zproc_destroy(self_p)
        result
      end

      # Setup the command line arguments, the first item must be an (absolute) filename
      # to run.                                                                        
      #
      # @param args [Zlistx, #__ptr]
      # @return [void]
      def set_args(args)
        raise DestroyedError unless @ptr
        self_p = @ptr
        args = args.__ptr if args
        result = ::CZMQ::FFI.zproc_set_args(self_p, args)
        result
      end

      # Setup the environment variables for the process.
      #
      # @param args [Zhashx, #__ptr]
      # @return [void]
      def set_env(args)
        raise DestroyedError unless @ptr
        self_p = @ptr
        args = args.__ptr if args
        result = ::CZMQ::FFI.zproc_set_env(self_p, args)
        result
      end

      # Connects process stdin with a readable ('>', connect) zeromq socket. If
      # socket argument is NULL, zproc creates own managed pair of inproc      
      # sockets.  The writable one is then accessbile via zproc_stdin method.  
      #
      # @param socket [::FFI::Pointer, #to_ptr]
      # @return [void]
      def set_stdin(socket)
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_set_stdin(self_p, socket)
        result
      end

      # Connects process stdout with a writable ('@', bind) zeromq socket. If 
      # socket argument is NULL, zproc creates own managed pair of inproc     
      # sockets.  The readable one is then accessbile via zproc_stdout method.
      #
      # @param socket [::FFI::Pointer, #to_ptr]
      # @return [void]
      def set_stdout(socket)
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_set_stdout(self_p, socket)
        result
      end

      # Connects process stderr with a writable ('@', bind) zeromq socket. If 
      # socket argument is NULL, zproc creates own managed pair of inproc     
      # sockets.  The readable one is then accessbile via zproc_stderr method.
      #
      # @param socket [::FFI::Pointer, #to_ptr]
      # @return [void]
      def set_stderr(socket)
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_set_stderr(self_p, socket)
        result
      end

      # Return subprocess stdin writable socket. NULL for
      # not initialized or external sockets.             
      #
      # @return [::FFI::Pointer]
      def stdin()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_stdin(self_p)
        result
      end

      # Return subprocess stdout readable socket. NULL for
      # not initialized or external sockets.              
      #
      # @return [::FFI::Pointer]
      def stdout()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_stdout(self_p)
        result
      end

      # Return subprocess stderr readable socket. NULL for
      # not initialized or external sockets.              
      #
      # @return [::FFI::Pointer]
      def stderr()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_stderr(self_p)
        result
      end

      # Starts the process.
      #
      # @return [Integer]
      def run()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_run(self_p)
        result
      end

      # process exit code
      #
      # @return [Integer]
      def returncode()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_returncode(self_p)
        result
      end

      # PID of the process
      #
      # @return [Integer]
      def pid()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_pid(self_p)
        result
      end

      # return true if process is running, false if not yet started or finished
      #
      # @return [Boolean]
      def running()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_running(self_p)
        result
      end

      # wait or poll process status, return return code
      #
      # @param hang [Boolean]
      # @return [Integer]
      def wait(hang)
        raise DestroyedError unless @ptr
        self_p = @ptr
        hang = !(0==hang||!hang) # boolean
        result = ::CZMQ::FFI.zproc_wait(self_p, hang)
        result
      end

      # return internal actor, usefull for the polling if process died
      #
      # @return [::FFI::Pointer]
      def actor()
        raise DestroyedError unless @ptr
        self_p = @ptr
        result = ::CZMQ::FFI.zproc_actor(self_p)
        result
      end

      # send a signal to the subprocess
      #
      # @param signal [Integer, #to_int, #to_i]
      # @return [void]
      def kill(signal)
        raise DestroyedError unless @ptr
        self_p = @ptr
        signal = Integer(signal)
        result = ::CZMQ::FFI.zproc_kill(self_p, signal)
        result
      end

      # set verbose mode
      #
      # @param verbose [Boolean]
      # @return [void]
      def set_verbose(verbose)
        raise DestroyedError unless @ptr
        self_p = @ptr
        verbose = !(0==verbose||!verbose) # boolean
        result = ::CZMQ::FFI.zproc_set_verbose(self_p, verbose)
        result
      end

      # Returns CZMQ version as a single 6-digit integer encoding the major
      # version (x 10000), the minor version (x 100) and the patch.        
      #
      # @return [Integer]
      def self.czmq_version()
        result = ::CZMQ::FFI.zproc_czmq_version()
        result
      end

      # Returns true if the process received a SIGINT or SIGTERM signal.
      # It is good practice to use this method to exit any infinite loop
      # processing messages.                                            
      #
      # @return [Boolean]
      def self.interrupted()
        result = ::CZMQ::FFI.zproc_interrupted()
        result
      end

      # Returns true if the underlying libzmq supports CURVE security.
      #
      # @return [Boolean]
      def self.has_curve()
        result = ::CZMQ::FFI.zproc_has_curve()
        result
      end

      # Return current host name, for use in public tcp:// endpoints.
      # If the host name is not resolvable, returns NULL.            
      #
      # @return [::FFI::AutoPointer]
      def self.hostname()
        result = ::CZMQ::FFI.zproc_hostname()
        result = ::FFI::AutoPointer.new(result, LibC.method(:free))
        result
      end

      # Move the current process into the background. The precise effect     
      # depends on the operating system. On POSIX boxes, moves to a specified
      # working directory (if specified), closes all file handles, reopens   
      # stdin, stdout, and stderr to the null device, and sets the process to
      # ignore SIGHUP. On Windows, does nothing. Returns 0 if OK, -1 if there
      # was an error.                                                        
      #
      # @param workdir [String, #to_s, nil]
      # @return [void]
      def self.daemonize(workdir)
        result = ::CZMQ::FFI.zproc_daemonize(workdir)
        result
      end

      # Drop the process ID into the lockfile, with exclusive lock, and   
      # switch the process to the specified group and/or user. Any of the 
      # arguments may be null, indicating a no-op. Returns 0 on success,  
      # -1 on failure. Note if you combine this with zsys_daemonize, run  
      # after, not before that method, or the lockfile will hold the wrong
      # process ID.                                                       
      #
      # @param lockfile [String, #to_s, nil]
      # @param group [String, #to_s, nil]
      # @param user [String, #to_s, nil]
      # @return [void]
      def self.run_as(lockfile, group, user)
        result = ::CZMQ::FFI.zproc_run_as(lockfile, group, user)
        result
      end

      # Configure the number of I/O threads that ZeroMQ will use. A good  
      # rule of thumb is one thread per gigabit of traffic in or out. The 
      # default is 1, sufficient for most applications. If the environment
      # variable ZSYS_IO_THREADS is defined, that provides the default.   
      # Note that this method is valid only before any socket is created. 
      #
      # @param io_threads [Integer, #to_int, #to_i]
      # @return [void]
      def self.set_io_threads(io_threads)
        io_threads = Integer(io_threads)
        result = ::CZMQ::FFI.zproc_set_io_threads(io_threads)
        result
      end

      # Configure the number of sockets that ZeroMQ will allow. The default  
      # is 1024. The actual limit depends on the system, and you can query it
      # by using zsys_socket_limit (). A value of zero means "maximum".      
      # Note that this method is valid only before any socket is created.    
      #
      # @param max_sockets [Integer, #to_int, #to_i]
      # @return [void]
      def self.set_max_sockets(max_sockets)
        max_sockets = Integer(max_sockets)
        result = ::CZMQ::FFI.zproc_set_max_sockets(max_sockets)
        result
      end

      # Set network interface name to use for broadcasts, particularly zbeacon.    
      # This lets the interface be configured for test environments where required.
      # For example, on Mac OS X, zbeacon cannot bind to 255.255.255.255 which is  
      # the default when there is no specified interface. If the environment       
      # variable ZSYS_INTERFACE is set, use that as the default interface name.    
      # Setting the interface to "*" means "use all available interfaces".         
      #
      # @param value [String, #to_s, nil]
      # @return [void]
      def self.set_biface(value)
        result = ::CZMQ::FFI.zproc_set_biface(value)
        result
      end

      # Return network interface to use for broadcasts, or "" if none was set.
      #
      # @return [String]
      def self.biface()
        result = ::CZMQ::FFI.zproc_biface()
        result
      end

      # Set log identity, which is a string that prefixes all log messages sent
      # by this process. The log identity defaults to the environment variable 
      # ZSYS_LOGIDENT, if that is set.                                         
      #
      # @param value [String, #to_s, nil]
      # @return [void]
      def self.set_log_ident(value)
        result = ::CZMQ::FFI.zproc_set_log_ident(value)
        result
      end

      # Sends log output to a PUB socket bound to the specified endpoint. To   
      # collect such log output, create a SUB socket, subscribe to the traffic 
      # you care about, and connect to the endpoint. Log traffic is sent as a  
      # single string frame, in the same format as when sent to stdout. The    
      # log system supports a single sender; multiple calls to this method will
      # bind the same sender to multiple endpoints. To disable the sender, call
      # this method with a null argument.                                      
      #
      # @param endpoint [String, #to_s, nil]
      # @return [void]
      def self.set_log_sender(endpoint)
        result = ::CZMQ::FFI.zproc_set_log_sender(endpoint)
        result
      end

      # Enable or disable logging to the system facility (syslog on POSIX boxes,
      # event log on Windows). By default this is disabled.                     
      #
      # @param logsystem [Boolean]
      # @return [void]
      def self.set_log_system(logsystem)
        logsystem = !(0==logsystem||!logsystem) # boolean
        result = ::CZMQ::FFI.zproc_set_log_system(logsystem)
        result
      end

      # Log error condition - highest priority
      #
      # @param format [String, #to_s, nil]
      # @param args [Array<Object>] see https://github.com/ffi/ffi/wiki/examples#using-varargs
      # @return [void]
      def self.log_error(format, *args)
        result = ::CZMQ::FFI.zproc_log_error(format, *args)
        result
      end

      # Log warning condition - high priority
      #
      # @param format [String, #to_s, nil]
      # @param args [Array<Object>] see https://github.com/ffi/ffi/wiki/examples#using-varargs
      # @return [void]
      def self.log_warning(format, *args)
        result = ::CZMQ::FFI.zproc_log_warning(format, *args)
        result
      end

      # Log normal, but significant, condition - normal priority
      #
      # @param format [String, #to_s, nil]
      # @param args [Array<Object>] see https://github.com/ffi/ffi/wiki/examples#using-varargs
      # @return [void]
      def self.log_notice(format, *args)
        result = ::CZMQ::FFI.zproc_log_notice(format, *args)
        result
      end

      # Log informational message - low priority
      #
      # @param format [String, #to_s, nil]
      # @param args [Array<Object>] see https://github.com/ffi/ffi/wiki/examples#using-varargs
      # @return [void]
      def self.log_info(format, *args)
        result = ::CZMQ::FFI.zproc_log_info(format, *args)
        result
      end

      # Log debug-level message - lowest priority
      #
      # @param format [String, #to_s, nil]
      # @param args [Array<Object>] see https://github.com/ffi/ffi/wiki/examples#using-varargs
      # @return [void]
      def self.log_debug(format, *args)
        result = ::CZMQ::FFI.zproc_log_debug(format, *args)
        result
      end

      # Self test of this class.
      #
      # @param verbose [Boolean]
      # @return [void]
      def self.test(verbose)
        verbose = !(0==verbose||!verbose) # boolean
        result = ::CZMQ::FFI.zproc_test(verbose)
        result
      end
    end
  end
end

################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
