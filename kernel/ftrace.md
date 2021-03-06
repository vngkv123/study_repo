# ftrace
**tracer for internal kernel function flow**
- `mount -t debugfs nodev /sys/kernel/debug`
```
root@6c7f37ea6051:/sys/kernel/debug# ls -l
total 0
drwxr-xr-x  2 root root 0 Oct 12 17:50 acpi
drwxr-xr-x  2 root root 0 Oct 12 17:50 aoe
drwxr-xr-x  2 root root 0 Oct 12 17:50 aufs
drwxr-xr-x 32 root root 0 Oct 12 17:51 bdi
drwxr-xr-x  2 root root 0 Oct 12 17:50 extfrag
-rw-r--r--  1 root root 0 Oct 12 17:50 fault_around_bytes
drwxr-xr-x  2 root root 0 Oct 12 17:50 hid
drwxr-xr-x  2 root root 0 Oct 12 17:50 kprobes
drwxr-xr-x  2 root root 0 Oct 12 17:50 nbd
drwxr-xr-x  2 root root 0 Oct 12 17:50 pm_qos
drwxr-xr-x  2 root root 0 Oct 12 17:50 regmap
-rw-r--r--  1 root root 0 Oct 12 17:50 sched_features
-r--r--r--  1 root root 0 Oct 12 17:50 sleep_time
--w-------  1 root root 0 Oct 12 17:50 split_huge_pages
-r--r--r--  1 root root 0 Oct 12 17:50 suspend_stats
dr-xr-xr-x  3 root root 0 Oct 12 17:50 tracing
drwxr-xr-x  2 root root 0 Oct 12 17:50 virtio-ports
-r--r--r--  1 root root 0 Oct 12 17:50 wakeup_sources
drwxr-xr-x  2 root root 0 Oct 12 17:50 x86

root@6c7f37ea6051:/sys/kernel/debug/tracing# cat current_tracer
nop
root@6c7f37ea6051:/sys/kernel/debug/tracing# cat available_tracers
blk mmiotrace function_graph function nop
```
- below is main file in /sys/kernel/debug/tracing
```
available_tracers
current_tracer
trace -> trace log file
README -> how to use
```
- event tracing( interrupt, scheduling, filesystem ... )
- kernel function tracing( all of kernel function, call graph, stack usage )
- latency tracing( wakeup, wakeup_rt, irqsoff, preemptoff, preemptirqsoff )

**function_graph**
```
 0)   1.013 us    |        }
 0)               |        unlock_page() {
 1)   0.023 us    |                  _raw_spin_lock_irqsave();
 0)               |          page_waitqueue() {
 0)   0.030 us    |            bit_waitqueue();
 1)   0.036 us    |                  _raw_spin_unlock_irqrestore();
 0)   0.270 us    |          }
 1)   0.029 us    |                  pcpu_chunk_addr();
 0)   0.028 us    |          __wake_up_bit();
 0)   0.756 us    |        }
 1)   0.028 us    |                  pcpu_chunk_addr();
 0)               |        alloc_set_pte() {
 0)   0.021 us    |          add_mm_counter_fast();
 0)               |          page_add_file_rmap() {
 1)   7.126 us    |                }
 0)   0.028 us    |            lock_page_memcg();
 1)   7.520 us    |              }
 0)   0.012 us    |            unlock_page_memcg();
 1)   0.048 us    |              _raw_spin_lock_irqsave();
 0)   0.526 us    |          }
 ```
# How to trace kernel function flow?
**Using gcc compiler's -pg option**
- with out -pg option is below one
```
   0x0000000000400526 <+0>:	push   rbp
   0x0000000000400527 <+1>:	mov    rbp,rsp
   0x000000000040052a <+4>:	sub    rsp,0x10
   0x000000000040052e <+8>:	movabs rax,0x6e6973756a65654c
   0x0000000000400538 <+18>:	mov    QWORD PTR [rbp-0x10],rax
   0x000000000040053c <+22>:	mov    BYTE PTR [rbp-0x8],0x0
   0x0000000000400540 <+26>:	lea    rax,[rbp-0x10]
   0x0000000000400544 <+30>:	mov    rsi,rax
   0x0000000000400547 <+33>:	mov    edi,0x4005e4
   0x000000000040054c <+38>:	mov    eax,0x0
   0x0000000000400551 <+43>:	call   0x400400 <printf@plt>
   0x0000000000400556 <+48>:	mov    eax,0x0
   0x000000000040055b <+53>:	leave
   0x000000000040055c <+54>:	ret
```
- with -pg option is below one
```
   0x0000000000400686 <+0>:	push   rbp
   0x0000000000400687 <+1>:	mov    rbp,rsp
   0x000000000040068a <+4>:	sub    rsp,0x10
   0x000000000040068e <+8>:	call   0x400520 <mcount@plt>
   0x0000000000400693 <+13>:	movabs rax,0x6e6973756a65654c
   0x000000000040069d <+23>:	mov    QWORD PTR [rbp-0x10],rax
   0x00000000004006a1 <+27>:	mov    BYTE PTR [rbp-0x8],0x0
   0x00000000004006a5 <+31>:	lea    rax,[rbp-0x10]
   0x00000000004006a9 <+35>:	mov    rsi,rax
   0x00000000004006ac <+38>:	mov    edi,0x40078c
   0x00000000004006b1 <+43>:	mov    eax,0x0
   0x00000000004006b6 <+48>:	call   0x4004f0 <printf@plt>
   0x00000000004006bb <+53>:	mov    eax,0x0
   0x00000000004006c0 <+58>:	leave
   0x00000000004006c1 <+59>:	ret
```
- call mcount is added in this binary.

# mcount hooking
**One of hooking mechnisms**
- complier level function -> ABI
- if compile with gcc -pg option, mcount brench code is inserted
- if developer can code some wanted routine in mcount function, hook is not hard.
- ftrace use this mechnism and, by revising mcount internal routine, it can trace 
internal kernel function.

