1. strtok会改变传入的字符串，所以传入的字符串不能以char *a="xxx"方式定义，因为这是只读字符串。修改只读区域会segfault。
2. expr中规则前面两个转义\,第一个是c语言的，表示后面的\是\字符，后面的\是正则中的转义
3. make -p可以查看所有make内部变量，对于学习make有好处
4. 调试指令集实现时，遇到一个程序出错，则先简化，简化到出错与不出错边界

| | 被调用者不用，调用者用 | 被调用者用，调用者用 | 被调用者不用，调用者也不用 | 被调用者用，调用者不用 |
| - | - | - | - | - |
| 调用者保存寄存器(前后不保证一致) | 保存 | 保存 | 不保存 | 不保存 |
| 被调用者保存寄存器(前后保证一致) | 不保存 | 保存 | 不保存 | 保存 |

编译函数都是调用者视角，所以调用者还要用的寄存器建议使用被调用者保存寄存器，不用的建议使用调用者保存寄存器

riscv指令学习

- _start:
    - li s0, 0 加载0到s0，li是加载值到寄存器，可以用addi和lui实现
    - jal应该可以换成j，也不返回，可以试试
    - _start是汇编定义的符号而不是函数，所以大小是0，可以换成函数，然后ftrace可以捕捉
- _trm_init
    - 返回寄存器是调用者保存(前后不保证一致)，是否被调用者保存好点？？
    - 开始时就计算好所需要的栈大小，进行减栈
- halt
    - 编译时自动内联了，在halt前面加`__attribute_noinline__`后发现有这个函数了

- jal是保存下一指令地址，jal可能是函数最后一个指令吗？
- jal和jalr无本质区别，都是跳转到某一地址后把下一指令存到某寄存器，均可用作函数调用及返回
- 函数返回指令是特殊的jalr，即把ra作为跳转的地址，不保存下一指令地址

> recursion 不匹配的函数调用和返回:

观察代码及汇编，以f0函数为例。

```c
int f0(int n, int l) {
  if (l > lvl) lvl = l;
  rec ++;
  return n <= 0 ? 1 : func[3](n / 3, l + 1);
};
```

```asm
0000000080000010 <f0>:
    80000010:	00000797          	auipc	a5,0x0
    80000014:	2a078793          	addi	a5,a5,672 # 800002b0 <lvl>
    80000018:	0007a703          	lw	a4,0(a5)
    8000001c:	00b75463          	bge	a4,a1,80000024 <f0+0x14>
    80000020:	00b7a023          	sw	a1,0(a5)
    80000024:	00000717          	auipc	a4,0x0
    80000028:	29070713          	addi	a4,a4,656 # 800002b4 <rec>
    8000002c:	00072783          	lw	a5,0(a4)
    80000030:	0017879b          	addiw	a5,a5,1
    80000034:	00f72023          	sw	a5,0(a4)
    80000038:	00a05e63          	blez	a0,80000054 <f0+0x44>
    8000003c:	00300793          	li	a5,3
    80000040:	02f5453b          	divw	a0,a0,a5
    80000044:	0015859b          	addiw	a1,a1,1
    80000048:	00000797          	auipc	a5,0x0
    8000004c:	2607b783          	ld	a5,608(a5) # 800002a8 <func+0x18>
    80000050:	00078067          	jr	a5
    80000054:	00100513          	li	a0,1
    80000058:	00008067          	ret
```

当`n <= 0`时，直接从0x80000038跳到80000054，然后赋给a0为1，返回。
否则直接准备好参数并把函数地址放到a5，80000050的`jr a5`跳到f3，由f3来执行实际的返回操作。即把f3当成f0的一部分。不同于常规函数调用，只是转移了pc，没有保存ra，ra还是原来的。f3返回时返回的是f0该返回的地址，该地址读取的a0就是f3返回的值，也是f0所期望的。
从f0到f3的过程虽然转移了控制权，但未发生函数调用。则可以出现main调用f0，但最后却是从f3返回到main。所以会有不匹配。

tools/spike-diff/difftest.cc定义了一些函数如difftest_memcpy等

捕捉死循环:两个时刻状态完全相同

设备框架原理：

```c
// CONFIG_RTC_MMIO是基地址
// rtc_port_base是基地址对应的寄存器地址，访问CONFIG_RTC_MMIO就是访问rtc_port_base
// rtc_io_handler是访问该地址操作，主要是操作rtc_port_base
// am层对设备读写操作最后就是调用rtc_io_handler，操作rtc_port_base，然后写入或者返回
// 更上层写的是am的抽象寄存器，又是另外一种
add_mmio_map("rtc", CONFIG_RTC_MMIO, rtc_port_base, 8, rtc_io_handler);
```

键盘捕捉是在`device_update`进行

PA3

操作系统需防止应用程序直接通过跳转执行，例如操作系统调用uboot行为

思考为何一些中断状态由硬件保存？？

有时候trap后应回到epc下一个指令，在哪里判断是下一个？这里采用在__am_irq_handle里判断

do_syscall中先备份syscall参数，是否是因为防止函数调用导致被覆盖？？？

```c
intptr_t _syscall_(intptr_t type, intptr_t a0, intptr_t a1, intptr_t a2) {
  register intptr_t _gpr1 asm (GPR1) = type;
  register intptr_t _gpr2 asm (GPR2) = a0;
  register intptr_t _gpr3 asm (GPR3) = a1;
  register intptr_t _gpr4 asm (GPR4) = a2;
  register intptr_t ret asm (GPRx);
  asm volatile (SYSCALL : "=r" (ret) : "r"(_gpr1), "r"(_gpr2), "r"(_gpr3), "r"(_gpr4));
  return ret;
}
```

register是啥意思



TODO: 可以从宏获得名字，从而使调试的代码更好看。如lseek的SEEKEND可以直接打印
      使printf支持更多格式

pa4

kcontext()要求内核线程不能从entry返回, 否则其行为是未定义的.因为entry是该线程第一个调用的函数

// context->gpr[2] = (uintptr_t)context; 初始化context的sp是不用设置，因为上下文恢复是用__am_irq_handle返回值

用户态程序context_uload，用户态调度的时候首先还是从内核栈上恢复上下文，然后跳到start时就设成用户栈了。所以ucontext时候就把GPRx设成用户栈,这是与kcontext不同的地方

用户态程序也要使用内核栈，包括存context，存进入系统调用后的信息。需要了解如何控制sp在内核和用户态正确轮转。当前是初始context在内核栈，其余包括用户态系统调用后的context也是在用户栈


这个是navy-apps中设置用户栈指针的代码
```c
#elif defined(__riscv)
  mv s0, zero
  mv sp, a0
  jal call_main
```

用户程序的初始context中a0设置成heap.end

```c
Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *context = (Context *)(kstack.end - sizeof(Context));
  context->mepc = (uintptr_t)entry;
  context->gpr[1] = (uintptr_t)NULL; // ra
  // context->gpr[2] = (uintptr_t)context;
  context->mcause = 0xa00001800; // corresponding to difftest
  context->mcause |= (1 << 7);   // set MPIE

#ifdef HAS_VME
#define USTACK_PAGES 4
  void *ustack = pgalloc_usr(USTACK_PAGES * PGSIZE);
  for(int i = 0; i < USTACK_PAGES; i++) {
    map(as, pcb->as.area.end - USTACK_PAGES * STACK_SIZE + i*PGSIZE, ustack + i*PGSIZE, MMAP_WRITE | MMAP_READ);
  }
  context->GPRx = pcb->as.area.end;//这里设置用户栈指针，_start栈指针切到这
#else
  context->GPRx = (uintptr_t)heap.end;
#endif

  context->pdir = as->ptr;
  return context;
}

```

pa4.3栈易错点: context改了但CONTEXT_SIZE没改导致系统调用前后栈错位，该错误是打印mret时的栈指针发现