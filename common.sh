# Start a program, save its pid
launch_ () 
{
    $*&
    _pid=$!
    echo "$_pid" >> /tmp/.$$_pids
    echo "$1 $_pid"
}

# Kill programs that were started with launch_
kill_all ()
{
    for pid in $(< /tmp/.$$_pids); do
        kill $pid
    done
    rm /tmp/.$$_pids
}
    
