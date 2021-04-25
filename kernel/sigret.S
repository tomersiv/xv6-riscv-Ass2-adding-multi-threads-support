.globl sigret_start
.globl sigret_end

sigret_start:
  li a7, 24
  ecall
  ret              
sigret_end: