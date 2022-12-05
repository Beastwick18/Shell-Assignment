# Shell-Assignment
- This is my Shell Assignment for my Operating Systems class (CSE3320).
- In this assignment, I implemented a shell program that takes in a given command of up to 10 arguments and 255 characters.
- My implementation uses my implementation of a static array structure to keep track of command history and previously spawned proccess PIDs.
  - A maximum of 20 PIDs and 15 commands can be stored in history.
- Upon running the program, the user is prompted with the shell `msh>` where they can enter their commands.
- Valid commands are as follows:
  - history
  - listpids
  - any valid executable within the paths:
    - `./`
    - `/usr/local/bin/`
    - `/usr/bin/`
    - `/bin/`
  - Paths are searched in above order
- The shell will continue once the spawned process exits
