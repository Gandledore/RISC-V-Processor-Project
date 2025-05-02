main:	addi t0, zero, 0          # base address = 0
        addi t1, zero, 0          # sum = 0
        addi t2, zero, 0          # i = 0
        addi t3, t0, 12           # address = base + 12
        addi t5, zero, 5          # end = 5
        addi t6, zero, 4          # word size (offset increment)
loop:	beq t2, t5, done          # if i == 5, exit
        lw   t4, 0(t3)            # load word at current address
        add  t1, t1, t4          # sum += value
        addi t2, t2, 1            # i += 1
        add  t3, t3, t6          # address += 4
        beq zero, zero, loop      # unconditional jump to loop
done:	addi t4, t0, 92           # address = base + 92
        sw   t1, 0(t4)            # store sum at address 92

# Registers:
# t0 - base address (assumed to be 0)
# t1 - running sum
# t2 - loop counter (i)
# t3 - current address pointer
# t4 - loaded value from array
# t5 - end index (5)
# t6 - offset increment (4)