	la t0, vec        	# t0 = start address of array
        addi t1, zero, 0         # t1 = index
        addi t2, zero, 5         # t2 = number of elements
        addi t3, zero, 0         # t3 = running sum
loop:   beq  t1, t2, done        # if index == 3, exit loop
        lw   t4, 0(t0)           # load value into t4
        jal  process             # call process function (input in t4, result in t5)
        add  t3, t3, t5          # sum += result
        addi t0, t0, 4           # next memory address
        addi t1, t1, 1           # i++
        beq  zero, zero, loop    # unconditional jump (loop again)
done:   la t6, output       # target address = 100
        sw   t3, 0(t6)           # store final result
        beq  zero, zero, end    # halt (infinite loop)
process:andi t6, t4, 1           # check LSB: even if (val & 1 == 0)
        beq  t6, zero, is_even   # branch if even
        add  t5, t4, zero       #odd path: return value unchanged
        jalr zero, ra, 0
is_even:andi t5, t4, 0xF0        # mask high bits
        ori  t5, t5, 0x0F        # OR with 0x0F
        addi t6, zero, 0x01
        sub  t5, t5, t6         # subtract 1
        and  t5, t5, t4         # combine with original via AND
        jalr zero, ra, 0
end:

# ------------------------------------
# Function: process
# Input:  t4
# Output: t5
# Description:
#   If even: mask with 0xF0, OR with 0x0F, then subtract 1
#   If odd:  return value unchanged
