addi t2, zero, 0
addi t0, t2, 1          #depends on I1
addi t1, t0, -2         #depends on I2
loop: add t1, t0, t1    #depends on I2 and I3
      add t2, t1, t2    #depends on I4 (not I3)
      beq t1, t2, loop  #depends on I4 and I5, requires stall, (flush twice)
addi t2, t2 , 0xa9      #depends on I5
addi t3, zero, 0x2c     #no dependencies
done: addi t3, t3, 4    #depends on I8
      sw t2, 0(t3)      #depends on I7 and I9
      lw t4, 16(t3)     #depends on I7
      beq t4, t0 , done

#loop is completely meaningless, but tests some dependency and flushing (will need to add lw dependencies too)

# pipelined: 50 clock cycles