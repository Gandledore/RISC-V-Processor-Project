addi t2, zero, 0
addi t0, t2, 1          #depends on I1
addi t1, t0, -2         #depends on I2
loop: add t1, t0, t1    #depends on I2 and I3
      add t2, t1, t2    #depends on I4 (not I3)
      beq t1, t2, loop  #depends on I4 and I5, requires stall, (flush twice)
done: addi t3, t2, 73   #depends on I5
      sw t2, 0(t3)      #depends on I7
      addi t2, zero, 0
end:

#loop is completely meaningless, but tests some dependency and flushing (will need to add lw dependencies too)
#stores 0x3 to memory 0x4c

# 5
# 1 2 3 4 5 6 7
# offset = 2+1=3
# 