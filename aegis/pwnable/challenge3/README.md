# Pwnable Challenge 3
**Description about RTL( Return to Libc  Exploitation )**
- RTL is useful when `NX` mitigation is on and can't execute shellcode directly.
- RTL means using library to bypass above mitigation.
- Need to library address to get another library function.
- System load `shared object file == shared library` on runtime.
```
exploit-peda$ vmmap
Start      End        Perm	Name
0x08048000 0x08049000 r-xp	/shared/aegis/pwnable/challenge3/prertl
0x08049000 0x0804a000 r--p	/shared/aegis/pwnable/challenge3/prertl
0x0804a000 0x0804b000 rw-p	/shared/aegis/pwnable/challenge3/prertl
0xf7e09000 0xf7e0a000 rw-p	mapped
0xf7e0a000 0xf7fba000 r-xp	/lib/i386-linux-gnu/libc-2.23.so
0xf7fba000 0xf7fbc000 r--p	/lib/i386-linux-gnu/libc-2.23.so
0xf7fbc000 0xf7fbd000 rw-p	/lib/i386-linux-gnu/libc-2.23.so
0xf7fbd000 0xf7fc0000 rw-p	mapped
0xf7fc0000 0xf7fc3000 r-xp	/lib/i386-linux-gnu/libdl-2.23.so
0xf7fc3000 0xf7fc4000 r--p	/lib/i386-linux-gnu/libdl-2.23.so
0xf7fc4000 0xf7fc5000 rw-p	/lib/i386-linux-gnu/libdl-2.23.so
0xf7fd4000 0xf7fd6000 rw-p	mapped
0xf7fd6000 0xf7fd8000 r--p	[vvar]
0xf7fd8000 0xf7fd9000 r-xp	[vdso]
0xf7fd9000 0xf7ffb000 r-xp	/lib/i386-linux-gnu/ld-2.23.so
0xf7ffb000 0xf7ffc000 rw-p	mapped
0xf7ffc000 0xf7ffd000 r--p	/lib/i386-linux-gnu/ld-2.23.so
0xf7ffd000 0xf7ffe000 rw-p	/lib/i386-linux-gnu/ld-2.23.so
0xfffdd000 0xffffe000 rw-p	[stack]
```
- libc-2.23.so and libdl-2.23.so are libraries.
- Library has so many functions that are good for exploitation.
```
root@9eb26f7bbdb1:/shared/aegis/pwnable/challenge3# gdb /lib/i386-linux-gnu/libc-2.23.so
Reading symbols from /lib/i386-linux-gnu/libc-2.23.so...(no debugging symbols found)...done.
exploit-peda$ p system
$1 = {<text variable, no debug info>} 0x3ada0 <system>
exploit-peda$ p printf
$2 = {<text variable, no debug info>} 0x49670 <printf>
exploit-peda$ p open
$3 = {<text variable, no debug info>} 0xd56e0 <open>
exploit-peda$ p close
$4 = {<text variable, no debug info>} 0xd6280 <close>
exploit-peda$ p rand
$5 = {<text variable, no debug info>} 0x2f900 <rand>
```
- If binary hasn't `system` function, it doesn't matter if we have library address.
- We can call `system` in library.
- At first, we need to know about `libc_base` and `offset` definitions.
- `libc_base` means `0xf7e0a000 0xf7fba000 r-xp	/lib/i386-linux-gnu/libc-2.23.so` -> 0xf7e0a000
- Literally meaning `library base address == Bottom of library address ( loaded library in memory )`
- `offset` means `function address - library_base address`
- For example, see the following one.
```
exploit-peda$ p system
$1 = {<text variable, no debug info>} 0xf7e44da0 <system>
exploit-peda$ vmmap
Start      End        Perm	Name
0x08048000 0x08049000 r-xp	/shared/aegis/pwnable/challenge3/prertl
0x08049000 0x0804a000 r--p	/shared/aegis/pwnable/challenge3/prertl
0x0804a000 0x0804b000 rw-p	/shared/aegis/pwnable/challenge3/prertl
0xf7e09000 0xf7e0a000 rw-p	mapped
0xf7e0a000 0xf7fba000 r-xp	/lib/i386-linux-gnu/libc-2.23.so
0xf7fba000 0xf7fbc000 r--p	/lib/i386-linux-gnu/libc-2.23.so
0xf7fbc000 0xf7fbd000 rw-p	/lib/i386-linux-gnu/libc-2.23.so
exploit-peda$ p 0xf7e44da0 - 0xf7e0a000
$2 = 0x3ada0
```
- `offset` is `0x3ada0`
- Another method is analyzing library directly.
- Library ( libc.so.6 ) is compiled with `-fPIC -shared` options.
- So, before loaded in memory, library has just offset to each function.
```
root@9eb26f7bbdb1:/shared/aegis/pwnable/challenge3# gdb /lib/i386-linux-gnu/libc-2.23.so
Reading symbols from /lib/i386-linux-gnu/libc-2.23.so...(no debugging symbols found)...done.
exploit-peda$ p system
$1 = {<text variable, no debug info>} 0x3ada0 <system>
```
- Use this `offset` and add to `library base`

**Exploitation**
- x86 follow std calling convention which set arguments in stack.
- If i call `read(0, buf, 0x100);`, disassemble results are following one.
```
   0x080486c9 <+138>:	push   0x100
   0x080486ce <+143>:	lea    eax,[ebp-0x10c]
   0x080486d4 <+149>:	push   eax
   0x080486d5 <+150>:	push   0x0
   0x080486d7 <+152>:	call   0x8048490 <read@plt>
```
- `push 0x100` -> first argument, `push eax ( which is equal to [ebp - 0x10c] )` -> second argument, `push 0x0` -> third argument.
- And call `read` function.
- `call` instruction is abstract instruction, which is equal to `push [next $pc] + jmp [target address]`
- So, we need to overwrite stack return address with this chain.
```
| return address | ... 
| system address | dummy | "/bin/sh" address | ...
```
- Because in library functions, they reference arguments like following one.
```
exploit-peda$ pd read
Dump of assembler code for function read:
   0x000d5af0 <+0>:	cmp    DWORD PTR gs:0xc,0x0
   0x000d5af8 <+8>:	jne    0xd5b20 <read+48>
   0x000d5afa <+10>:	push   ebx
   0x000d5afb <+11>:	mov    edx,DWORD PTR [esp+0x10]
   0x000d5aff <+15>:	mov    ecx,DWORD PTR [esp+0xc]
   0x000d5b03 <+19>:	mov    ebx,DWORD PTR [esp+0x8]
   0x000d5b07 <+23>:	mov    eax,0x3
   0x000d5b0c <+28>:	call   DWORD PTR gs:0x10
```
- `read` is system call wrapper, so it set registers with arguments before calling `int 0x80( DWORD PTR gs:0x10 )`.
- `ebx` is first arg, `ecx` is second and `edx` is last one.
- They get arguments from stack, which places are same as above description.
- We do just return to shellcode address, which overwrite stack address once.
- This is just overwrite return address with `arbitrary function address` and set `argument(s)` about this function.
- Next exploitation upgraded version of RTL is ROP ( Return Oriented Programming ) which is similar to RTL, but use so called `gadgets`.
- Enjoy your exploit !
