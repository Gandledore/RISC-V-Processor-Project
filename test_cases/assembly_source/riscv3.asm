	addi t0, zero, 12      	 # t0 = start address of array                          # no dependencies
        addi t1, zero, 0         # t1 = index                                           # no dependencies
        addi t2, zero, 5         # t2 = number of elements                              # no dependencies
        addi t3, zero, 0         # t3 = running sum                                     # no dependencies
loop:   beq  t1, t2, done        # if index == 3, exit loop                             # depends on I2 and I4
        lw   t4, 0(t0)           # load value into t4                                   # no dependencies
        jal  process             # call process function (input in t4, result in t5)    # no dependencies
        add  t3, t3, t5          # sum += result                                        # depends on I17 or I23
        addi t0, t0, 4           # next memory address                                  # no dependencies
        addi t1, t1, 1           # i++                                                  # no dependencies
        beq  zero, zero, loop    # unconditional jump (loop again)                      # no dependencies
done:   addi t6, zero, 100       # target address = 100                                 # no dependencies
        sw   t3, 0(t6)           # store final result                                   # depends on I12
        beq  zero, zero, end    # finish program                                        # no dependencies
process:andi t6, t4, 1           # check LSB: even if (val & 1 == 0)                    # depends on I6
        beq  t6, zero, is_even   # branch if even                                       # depends on I15
        add  t5, t4, zero       #odd path: return value unchanged                       # no dependencies
        jalr zero, ra, 0                                                                # no dependencies
is_even:andi t5, t4, 0xF0        # mask high bits                                       # no dependencies
        ori  t5, t5, 0x0F        # OR with 0x0F                                         # depends on I19
        addi t6, zero, 0x01                                                             # no dependencies
        sub  t5, t5, t6         # subtract 1                                            # depends on I20, I21
        and  t5, t5, t4         # combine with original via AND                         # depends on I22
        jalr zero, ra, 0                                                                # no dependencies
end:

# ------------------------------------
# Function: process
# Input:  t4
# Output: t5
# Description:
#   If even: mask with 0xF0, OR with 0x0F, then subtract 1
#   If odd:  return value unchanged

# piplined: 103 clock cycles