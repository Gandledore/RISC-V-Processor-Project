main:	addi t0, zero, 0          # base address = 0             #  no dependencies
        addi t1, t0, 0            # sum = 0                      #  depends on I1
        add t2, t0, t1            # i = 0                        #  depends on I1, I2
        addi t3, t0, 12           # address = base + 12          #  depends on I1
        addi t5, t0, 5            # end = 5                      #  no depencies
        addi t6, zero, 4          # word size (offset increment) #  no dependencies
loop:	beq t2, t5, done          # if i == 5, exit              #  dependson I5 (once), I10 (in loop)
        lw   t4, 0(t3)            # load word at current address #  depends on I11 (in loop)
        add  t1, t1, t4          # sum += value                  #  depends on I8
        addi t2, t2, 1            # i += 1                       #  no dependencies
        add  t3, t3, t6          # address += 4                  #  no dependencies
        beq zero, zero, loop      # unconditional jump to loop   #  no dependencies
done:	addi t4, t0, 92           # address = base + 92          #  no dependencies
        sw   t1, 0(t4)            # store sum at address 92      #  depends on I13

# Registers:
# t0 - base address (assumed to be 0)
# t1 - running sum
# t2 - loop counter (i)
# t3 - current address pointer
# t4 - loaded value from array
# t5 - end index (5)
# t6 - offset increment (4)