# Pwnable Challenge 1 Write-up
**Just use GDB**
- `set` command is powerful gdb command to modify `memory value` and `register value`.
- We can't find `/dev/urandom/` value, so normally bypassing the `if` statement is impossible.
- But, in GDB, we can use `set` command.
- Just set breakpoint in `cmp` instruction, and modify target registers.
```
=> 0x40094b <auth+341>:	cmp    edx,eax
```
- `set $rdx=0` and `set $rax=0` -> rax == rdx ( eax == edx )
```
exploit-peda$ set $rdx=0
exploit-peda$ set $rax=0
exploit-peda$ c
Continuing.
GDB_1s_most_p0w3rful_to0l_in_the_world!![Inferior 1 (process 4937) exited normally]
```
