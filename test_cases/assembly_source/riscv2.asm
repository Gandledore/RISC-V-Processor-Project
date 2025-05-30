        addi t0, zero, 0          # base address = 0                                    # no dependencies
        addi t1, zero, 0          # i = 0                                               # no dependencies
        addi t2, t0, 20           # array start address = base + 20                     # depends on I1
        addi t6, zero, 4          # word size                                           # no dependencies
        addi s0, zero, 5          # array length = 5                                    # no dependencies
        lw   t4, 0(t2)            # initialize result = array[0]                        # depends on I3
        addi t1, t1, 1            # i = 1                                               # no dependencies
        addi t2, t2, 4            # move to next array element                          # no dependencies
loop:   beq  t1, s0, done         # if i == 5, done                                     # depends on I7 (once), I12 (in loop)
        lw   t3, 0(t2)            # load array[i]                                       # depends on I8 (once), I13 (in loop)
        jal  and_subroutine         # call function to AND t4 and t3, result in t4      # no dependencies
        addi t1, t1, 1            # i++                                                 # no dependencies
        add  t2, t2, t6          # move to next array element                           # no dependencies
        beq  zero, zero, loop     # jump to loop                                        # no dependencies (flush)
done:   addi t5, t0, 100          # address to store result                             # no dependencies
        sw   t4, 0(t5)            # store result                                        # depends on I15
        beq  zero, zero, end     # jump to the end                                      # no dependencies (flush)
and_subroutine: and  t4, t4, t3 # Computes t4 = t4 & t3                                 # depends on I10
        jalr zero, ra, 0          # return                                              # depends on I11
end:

# pipelined: 61 clock cycles
