# Start a program, save its pid
# Assumes only 1 of program is running
launch_ () 
{
    $*&
    echo "$!" > /tmp/.${1}_pid
}

# Kill program that was started with launch_
kill_ ()
{
    kill $(< /tmp/.${1}_pid)
}
    
