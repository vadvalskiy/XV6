.globl thread_switch
thread_switch:
  # old = 4(%esp), new = 8(%esp)

  movl 4(%esp), %eax
  movl 8(%esp), %edx

  # save old registers
  movl %ebx, 12(%eax)
  movl %esi, 16(%eax)
  movl %edi, 20(%eax)
  movl %ebp, 8(%eax)

  # save old esp
  movl %esp, 4(%eax)

  # save return address as old eip
  movl (%esp), %ecx
  movl %ecx, 0(%eax)

  # load new registers
  movl 12(%edx), %ebx
  movl 16(%edx), %esi
  movl 20(%edx), %edi
  movl 8(%edx), %ebp
  movl 4(%edx), %esp

  # jump to new eip
  movl 0(%edx), %ecx
  jmp *%ecx