# OS IPC (Interprocess Communication)

Just a basic semaphore &amp; shared memory project to understand IPC

--------------------------------
How to compile and run
--------------------------------

    make run

Its easy to understand how processes communicate with this small project. We have a txt file with many lines of text and this text is divided in segments. For example if we have 1000 lines of text and we want to split it in 10 segment then it will be 10 segments of 100 lines each. The child processes making a request for a segment and a line in this segment, they are waiting the parent process to copy it to the shared memory and give them access to it when his done. There will be some more txt file at the end (storing the process history) 1 for each child and 1 for the parent process.
