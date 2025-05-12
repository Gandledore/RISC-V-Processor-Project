addi t0, zero, 0        # t0 = 0
addi t1, t0, 1          # t1 = 1
addi t2, t1, -2         # t2 = -1
addi a0, zero, 5        # a0 = loop counter (5 iterations)
addi s2, zero, 0        # s2 = will hold final result

loop:
add t3, t1, t2          # t3 = t1 + t2
add t4, t3, t0          # t4 = t3 + t0
addi a0, a0, -1         # a0 -= 1
beq a0, zero, exit      # branch to exit if a0 == 0
addi t1, t1, 1          # t1 += 1
jal zero, loop          # unconditional jump to loop

exit:
addi t5, t4, 0xa9       # t5 = t4 + 0xa9 = 4 + 169 = 173 = 0xad
addi t6, zero, 0x2c     # t6 = 0x2c
addi s0, t6, 4          # s0 = 0x30 (address for data storage)

# Write t5 (0xad) to memory at 0x30
sw t5, 0(s0)            # mem[0x30] = 0xad

# Store a valid return address (0x70) at mem[0x70] (within bounds of 0x7C)
addi s7, zero, 0x70     # t7 = 0x70 (valid return address within DMEM range)
sw s7, 16(s0)           # mem[0x40] = 0x70 (used by jalr)

# Load return address (0x70) from 0x40
lw s1, 16(s0)           # s1 = mem[0x40] = 0x70

# Jump to the address loaded into s1 (0x70)
jalr ra, 0(s1)          # jump to 0x70 (safe address)

beq ra, t1, done        # branch if ra == t1 (0x70 == 1, this will not happen)

done:
addi s2, t4, 0          # s2 = final t4 value (s2 = 4)
