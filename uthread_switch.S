.text

.globl thread_switch
thread_switch:
	pushal

	movl 36(%esp), %eax
	movl %esp, (%eax)

	movl 40(%esp), %eax
	movl (%eax), %esp

	popal
	ret