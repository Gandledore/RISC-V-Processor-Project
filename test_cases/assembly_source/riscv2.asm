        addi t0, zero, 0          # base address = 0
        addi t1, zero, 0          # i = 0
        addi t2, t0, 20           # array start address = base + 20
        addi t6, zero, 4          # word size
        addi s0, zero, 5          # array length = 5
        lw   t4, 0(t2)            # initialize result = array[0]
        addi t1, t1, 1            # i = 1
        addi t2, t2, 4            # move to next array element
loop:   beq  t1, s0, done         # if i == 5, done
        lw   t3, 0(t2)            # load array[i]
        jal  and_subroutine         # call function to AND t4 and t3, result in t4
        addi t1, t1, 1            # i++
        add  t2, t2, t6          # move to next array element
        beq  zero, zero, loop     # jump to loop
done:   addi t5, t0, 100          # address to store result
        sw   t4, 0(t5)            # store result
        beq  zero, zero, end     # halt (loop forever)
and_subroutine: and  t4, t4, t3 # Computes t4 = t4 & t3
        jalr zero, ra, 0          # return
end:
