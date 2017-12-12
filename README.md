Project 2: The Return of the Mastermind
This project is due on Sunday, December 17, at 11:59:59 PM (Eastern standard time). You must use the submit command to turn in your assignment like so: submit cs421_jtang proj2 mastermind2.c mastermind2-test.c

Your driver code must be named mastermind2.c, and it will be compiled against a 4.12 Linux kernel source tree, via the Kbuild supplied below. It must not have any compilation warnings; warnings will result in grading penalties. This module code must be properly indented and have a file header comment, as described on the coding conventions page. Prior to submission, use the kernel's indentation script to reformat your code, like so:

    ~/linux/scripts/Lindent mastermind2.c
  
In addition, you will write a unit test program, mastermind2-test.c, and it will be compiled on Ubuntu 16.04 as follows:

  gcc ‐‐std=c99 ‐Wall ‐O2 ‐pthread ‐o mastermind2-test mastermind-test.c cs421net.c ‐lm
There must not be any compilation warnings in your submission; warnings will result in grading penalties. In addition, this code must also have a file header comment and be properly indented. You will submit this test code along with your driver code.
In the first project, you wrote a rudimentary implementation of the game Mastermind. In this project, you will expand that code to create a full-fledged game. This improved version adds game statistics, configuration option, a second player, and multiple simultaneous games.

Part 1: Obtain Necessary Files
All instructions henceforth assume you successfully completed the first project. If you have not done so, go back and finish that assignment before proceeding. You have been warned.
To begin, create a directory for your project and download the following files into that directory via wget:

http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/mastermind2.c
Skeleton code for your kernel driver.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/mastermind2-test.c
Skeleton code for your unit test code.
In addition, download these files into that same directory. You will not need to modify any of these files, nor should you submit any of them with your work.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/Kbuild
Read by Linux kernel's build system, defines what is being built.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/Makefile
Builds the kernel module and unit test program, by simply running make. Also included is a clean target to remove all built objects.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/xt_cs421net.c
A Netfilter module that simulates a network device.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/xt_cs421net.h
Header file that declares symbols defined in xt_cs421net.h.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/cs421net.c
Adds networking functions to your unit test code.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/cs421net.h
Header file that declares symbols defined in cs421net.c.
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/proj2_start.sh
Shell script that reconfigures network settings needed for this project. After downloading this file, mark the script executable (chmod u+x proj2_start.sh).
http://www.csee.umbc.edu/~jtang/cs421.f17/homework/proj2/proj2_stop.sh
Shell script that disables network settings that were needed for this project. After downloading this file, mark the script executable (chmod u+x proj2_stop.sh).
After downloading all of these files into a new directory, copy your work from Project 1 as follows:

Copy your implementation of mm_read(), mm_write(), mm_mmap(), and mm_ctl_write() into the top part of mastermind2.c. Also copy any preprocessor symbols, global variables, and helper functions you used. If you had attempted the extra credit, copy those routines as well.
Merge the contents of your mastermind_init() into mastermind_probe(). Observe how the supplied code performs another resource allocation via device_create_file(). You are responsible for integrating that call into your code, in that all resources must be properly released upon error.
Merge the contents of your mastermind_exit() into mastermind_remove(). Observe how the converse to device_create_file() is device_remove_file().
Overwrite mastemind2-test.c with your working code from mastermind-test.c.
Now run make to compile everything. Upon success, you should now have the kernel driver mastermind2.ko and user space program mastermind2-test. Run proj2_start.sh; it may ask for your password, as that it runs some commands under sudo. This will set up your VM for this project. Rerun this script if you ever need to reboot your VM. Do not run proj2_start.sh itself under sudo.

Unload the mastermind module, if was still loaded from the previous loaded. Then use insmod to load mastermind2.ko. You should (again) have the /dev/mm and /dev/mm_ctl device nodes. You should also have a new virtual file, /sys/devices/platform/mastermind/stats.

Part 2: Cleanup Existing Code
A few tasks were accidentally omitted from the first project. Add these minor features to your code.

First, multiple threads may access the device nodes. Add a global spin lock variable. Guard accesses to all four file operations callbacks (mm_read(), mm_write(), mm_mmap(), and mm_ctl_write()) with that spin lock.

Then detect when the user correctly guesses the target code. When that happens, set the status message to something appropriate and then end the game. As an example, if the target code is 0012:

    $ echo -n "start" > /dev/mm_ctl
    $ echo -n "0012" > /dev/mm
    $ cat /dev/mm
    Correct! Game over
    $ echo -n "0012" > /dev/mm
    bash: echo: write error: Invalid argument
  
Also update the contents of the memory map to say that the game is over.
Part 3: Add Game Settings
The next task is to add a new command to /dev/mm_ctl, that allows the number of colors to be set.

The syntax for this command is colors X, where X must be a number between 2 and 9, inclusive.
The command is exactly eight characters; it does not include a trailing null nor newline character.
Only permit a system administrator (that is, a user with CAP_SYS_ADMIN capability) to use this command. If the calling process lacks it, return -EACCES.
If the new number of colors is less than two or greater than nine, return -EINVAL.
When the mastermind2 module is first loaded, set the number of colors to six.
Changes to the number of colors is silent, in that active games stay active without any notification that the range has changed. This could cause a game to not be winnable if the target code contains a color that is no longer guessable by the user.
If extra credit from Proj1 was implemented, modify the routine that generates the target code to use the new color range whenever a new game is started.
Then add a new global variable that tracks the number of games started. Then implement the function mm_stats_show(), as per its comments. This function generates a human-readable message that contains these game statistics:

Current number of colors
Number of pegs in the target code
Number of games started.
Don't forget to guard this function with the spin lock.
Test the new colors command from the command line like so: sudo sh -c 'echo -n "colors 7" > /dev/mm_ctl'. If everything above works:

    $ echo -n "colors 1" > /dev/mm_ctl
    bash: echo: write error: Permission denied
    $ echo -n "colors 5" > /dev/mm_ctl
    bash: echo: write error: Permission denied
    $ sudo sh -c 'echo -n "colors 1" > /dev/mm_ctl'
    sh: echo: I/O error
    $ sudo sh -c 'echo -n "colors 5" > /dev/mm_ctl'
    $ cat /sys/devices/platform/mastermind/stats
    CS421 Mastermind Stats
    Number of pegs: 4
    Number of colors: 5
    Number of games started: 0
  
Part 4: Add Interrupt Handling
Thus far, the secret code has always been 0012 (unless you implemented the extra credit, in which it is a random number). To better simulate the real Mastermind game, a remote opponent will be permitted to set the target code. Read the comments in the file xt_cs421net.c. When this Netfilter module is installed, via the proj2_start.sh script, it will raise an interrupt every time your computer receives network packets on TCP port 4210. It is your driver's responsibility to handle those interrupts and treat the packets as a remote attempt to set the target code.

In mastermind_probe(), install a threaded interrupt handler for interrupt number CS421NET_IRQ. Remove that handler in mastermind_remove(). This is a resource allocation, and thus mastermind_probe() must properly handle error conditions.

Then implement cs421net_top() and cs421net_bottom(). The network payload must be exactly 4 bytes and contain only valid colors; ignore all other payloads. Set the target code to the new code. As with setting the number of colors, this too is a silent operation; the player receives no notification that the code changed, and all previous status messages read from /dev/mm, as well as the memory mapping, are now obsolete.

Add two more global variables, one that counts the number of times the code was remotely changed, and another that counts the number of invalid attempts at changing the code. Report these counters in mm_stats_show().

As that cs421net_bottom() will be modifying global variables, be sure to guard the code with a spin lock.

Once you are confident your ISR works, call cs421net_enable() in mastermind_probe(), and likewise disable network integration via cs421net_disable() in mastermind_remove(). Install your module, and check /proc/interrupts to ensure the ISR was registered.

To test this part, ensure you earlier ran proj2_start.sh. Send a remote code change with the handy nc command:

    $ echo -n 'start' > /dev/mm_ctl
    $ echo -n '0000' > /dev/mm
    $ cat /dev/mm
    Guess 1: 2 black peg(s), 0 white peg(s)
    $ echo -n '4442' | nc localhost 4210
    $ echo -n '0000' > /dev/mm
    $ cat /dev/mm
    Guess 2: 0 black peg(s), 0 white peg(s)
    $ echo -n '444A' | nc localhost 4210
    $ echo -n '4442BCD' | nc localhost 4210
    $ cat /sys/devices/platform/mastermind/stats
    CS421 Mastermind Stats
    Number of pegs: 4
    Number of colors: 6
    Number of times code was changed: 1
    Number of invalid code change attempts: 2
    Number of games started: 1
  
Part 5: Support Multiple Games
You are about to make numerous changes to your code. Make a backup of your files prior to tackling this part.
The final task, and the most challenging for this project, is to add support for multiple players. The goal is allow each user to play his own game. Thus far, all of the game states are stored in multiple global variables. Follow these instructions carefully to convert those global variables into a linked list of variables.

Declare a new struct called mm_game. Move these global variables into fields within that struct mm_game:

game_active,
target_code,
num_guesses,
game_status, and
user_view.
If you had other global variables that affected game play, move those into the struct as well. Define a single global variable of type struct mm_game. Create a new function, find_game(), which simply returns a pointer to that single global variable. Then update all of your callbacks to call find_game() to retrieve a pointer to a struct mm_game; use that pointer to access what used to be global variables. Recompile and test these changes so far.
The next step is to dynamically allocate a pointer of type struct mm_game. Replace that single global variable of type struct mm_game with a global pointer to a struct mm_game. In find_game(), if that pointer is NULL, then dynamically allocate (with kmalloc() or kzalloc()) a struct mm_game and store the resulting value to the pointer. Still in that function, dynamically allocate space for the user_view field (with vmalloc()). In mastermind_probe(), remove the allocation to user_view, adjusting that function's error handling to avoid any memory leaks. Then update mastermind_remove() to free user_view (with vfree()) and the global pointer itself (with kfree()) , but only if that pointer is not NULL. Ensure there are no race conditions during the memory allocations. Recompile and test these changes so far.

Now add to struct mm_game a field of type kuid_t. Change find_game() to take a parameter of type kuid_t. Update all callers to find_game() to pass in the user ID (UID) via the macro current_uid(). Store the UID within the struct mm_game after the pointer was allocated. Because there is no current UID when mastermind_probe() and mastermind_remove() are invoked, you will need to rewrite those functions. Restructure your code so that mastermind_probe() no longer uses the global pointer or invokes find_game(). Update mastermind_remove() to avoid calling find_game(), instead having it access the global pointer directly. Recompile and test these changes so far.

Finally, add to struct mm_game a field of type struct list_head. Replace the global pointer with a variable of type struct list_head (using the LIST_HEAD macro). In find_game(), search for a linked list node whose UID matches the passed in parameter. If one exists, return a pointer to that node. Otherwise, allocate a new node, set that node's uid field to the passed in parameter, append the node to the global linked list, then return a pointer to that node. Update cs421net_bottom() to change all games' target_codes. In mastermind_remove(), safely free all memory associated with the linked list, including the dynamically allocated user_view within.

Be aware of various blog posts describing how to compare UIDs. Kernel UIDs have changed; many older blog posts are inaccurate. There is an explicit function to compare UIDs in modern kernels.

If everything works, each user has an independent game:

    $ echo -n 'start' > /dev/mm_ctl
    $ cat /dev/mm
    Starting game
    $ sudo cat /dev/mm
    No game yet
    $ sudo sh -c 'echo -n "start" > /dev/mm_ctl'
    $ echo -n '3322' > /dev/mm
    $ sudo sh -c 'echo -n "0123" > /dev/mm'
    $ cat /dev/mm
    Guess 1: 1 black peg(s), 0 white peg(s)
    $ sudo cat /dev/mm
    Guess 1: 1 black peg(s), 2 white peg(s)
    $ cat /sys/devices/platform/mastermind/stats
    CS421 Mastermind Stats
    Number of pegs: 4
    Number of colors: 6
    Number of times code was changed: 0
    Number of invalid code change attempts: 0
    Number of games started: 2
  
Be sure to try other UIDs besides yourself and root. Use the adduser command to create new user accounts. Then use su to log in as another user.
Part 6: Testing and Documentation
Now that you have (in theory) a working driver, you must then write your own unit tests. Read the networking code in cs421net.h. You can use cs421net_send() to send arbitrary messages to the CS421Net server. Modify mastermind2-test.c to exercise all functionality of this assignment. This includes a mix of inputs when writing to the device nodes and network socket, confirming that only system administrators can set the number of colors, and that each user can play his/her own game. Two functions that may be of use are getuid() and getresuid(). You should avoid using system(), and instead learn how to perform I/O yourself.

The unit tests must have comments that explain what things are being tested. As before, your goal is to test boundary conditions of your driver's interfaces. You will be graded based upon the thoroughness of the tests.

Assume that grader will run your unit tests from a "fresh" installation. That is, the grader will have just loaded your driver prior to running your unit tests. If your unit test program requires to be run as the root user (i.e., under sudo), indicate as such within a comment at the top of the file. Also specify if you require additional accounts to be created prior to running your test code.

Other Hints and Notes
Ask plenty of questions on the Blackboard discussion board.
At the top of your submission, list any help you received as well as web pages you consulted. Please do not use any URL shorteners, such as goo.gl or tinyurl.
Use the Linux Cross-Reference website to quickly search through kernel source code.
You may modify any of the provided code. You may need to add more functions and global variables than those listed above.
Make sure all resources are released in all error paths. This includes unlocking spin locks, freeing kernel memory, freeing the IRQ, and deregistering miscellaneous devices.
Make sure you indent your code one last time prior to submission. Unindented kernel code will result in grading penalties.
Extra Credit
You may earn an additional 10% credit for this assignment by tracking another statistic. Record the fastest completion of a game and that player's UID, then display this high score in /sys/devices/platform/mastermind/stats You will need to store the current timestamp whenever a game starts, then calculate the time spent playing when the game is won.

Update mastermind-test.c to test this new functionality. Explicitly add a unit test that finishes the game in some (mostly) deterministic amount of time, then check that mm_stats_show() calculated the correct time. Then switch to another user and beat the game faster. Check that mm_stats_show() returns an updated time and UID.

If you choose to perform this extra credit, put a comment at the top of your file, alerting the grader.