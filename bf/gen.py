#!/usr/bin/env python3
import sys

print('''\
.section .text
.global _start
_start:
    mov $9, %rax            # mmap
    xor %rdi, %rdi          # addr = 0
    mov $1000000000, %rsi   # length = 10^9
    mov $3, %rdx            # prot = PROT_READ | PROT_WRITE
    mov $16418, %r10        # flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
    mov $-1, %r8            # fd = -1
    xor %r9, %r9            # offset = 0
    syscall
    test %rax, %rax
    js error

    mov %rax, %rbx
''')

iota = 0
stack = []

while True:
    c = sys.stdin.read(1)

    if c == '<':
        print('    dec %rbx')

    elif c == '>':
        print('    inc %rbx')

    elif c == '+':
        print('    incb (%rbx)')

    elif c == '-':
        print('    decb (%rbx)')

    elif c == '[':
        stack.append(iota)
        print(f'''\
.B{iota}:
    cmpb $0, (%rbx)
    je .E{iota}
''')
        iota += 1

    elif c == ']':
        loop_iota = stack.pop()
        print(f'''\
    jmp .B{loop_iota}
.E{loop_iota}:
''')

    elif c == '.':
        print('    call write')

    elif c == ',':
        print('    call read')

    elif not c:
        break

print('''\
    mov $60, %rax   # exit
    xor %rdi, %rdi  # code = 0
    syscall

error:
    mov $60, %rax   # exit
    mov $12, %rdi   # code = 12
    syscall

write:
    mov $1, %rax    # write
    mov $1, %rdi    # fd = 1
    mov %rbx, %rsi  # buf = rbx
    mov $1, %rdx    # count = 1
    syscall
    ret

read:
    xor %rax, %rax   # read
    xor %rdi, %rdi   # fd = 0
    mov %rbx, %rsi   # buf = rbx
    mov $1, %rdx     # count = 1
    syscall

    #
    # if (eax < 1) *(char *)rbx = 0;
    #

    neg %rax
    shr $24, %rax
    andb %al, (%rbx)

    ret
''')
