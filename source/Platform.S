bits 64

section .text

global SKR_assert_write
global SKR_assert_abort

SKR_assert_write:
	mov rax, 1 ; write
	syscall
	ret

SKR_assert_abort:
	mov rax, 39 ; getpid
	syscall
	mov rdi, rax
	mov rsi, 6 ; SIGABRT
	mov rax, 62 ; kill
	syscall
