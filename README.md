# MIDI IPC

Get MIDI messages from some library calls (e.g., jack, coremidi, windows?), send
them to another application via FIFO. This can then send messages back through a
different FIFO and they will be sent out (e.g., to MIDI hardware, another
application, etc.).

The part receiving messages over a FIFO has some machinery to properly schedule
output events. The events received from the other application have a time field.
The time can be observed from the time stamp of an incoming event, so outgoing
events simply have some time added to them. Late events (those whose time stamps
are less than the current time) are played immediately. (TODO) If an application
wishes only to synthesize MIDI events, then it can receive MIDI clocks from a
dummy input (an application that only synthesizes these).
