	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:

    // 进入个函数时，由于C函数调用规约，已经有函数返回值在栈上了！
	pushal

	//将当前线程的esp保存
	movl current_thread, %eax
	// 能够这样做是因为 sp 指针在thread 结构体的最下方
	movl %esp, (%eax)

	// 使current_thread指向next_thread
	movl next_thread, %eax
	movl %eax, current_thread

	// 恢复next_thread的状态
	movl current_thread, %eax
	// 能够这样做是因为 sp 指针在thread 结构体的最下方
	movl (%eax), %esp
	popal

	// set next_thread to 0
	movl $0, next_thread // 感觉不加也行
	ret				/* pop return address from stack */
