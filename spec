0--------- OPIRS(3)
1000------ MOVIRS1(1)
1001------ MOVIRS2(1)
1010------ GOTO(6)
1011------ LOAD(8)
1100------ OP(4)
1101------ CMP(5)
1110------ unused
1111000000 MOVDATA1(2)
1111000001 MOVDATA1_(2)
1111000010 MOVDATA2(2)
1111000011 SIG(10)
1111000100 NEG(9)
1111000101 LOOP(7)



AD0 A AD1 B I WB 
AD0 A AD1 B I
AD0 A AD1 B I OP I2 
AD0 A AD1 B I ENIRS EXT MODE
AD0 A OFF I OP
X   A OFF B 
WB    OFF I
MD  A CONST




1: MOV RESULT, DATAx, LOOP  [IRS] 		=> code/4 wbreg/2 OFFSET/9 INC/1
   MOV [IRS]                RESULT, LOOP	=> code/4 A/1 B/1 OFFSET/9

2: MOV RESULT, DATAx, LOOP  [DATAx]  		=> code/12 WBREG/2 ADDR1/1 INC/1 (A/1,B/1,ADDR0/1)
   MOV [DATAx]              RESULT, LOOP 	=> code/12 ADDR1/1 A/1 B/1 INC/1 (ADDR0/1)
 
3: OP RESULT, [DATAx]  [IRS] 			=> code/1 ADDR0/1 A/1 OP/3 OFFSET/9 INC/1		

4: OP RESULT, [DATAx]  [DATAx]			=> code/8 ADDR0/1 A/1 ADDR1/1 OP/3 INC/2 (B/1)

* ADD,SUB,MUL,DIV,MAC,NEG

5: CMP RESULT  [IRS]				=> code/4 MODE/2 NOX_CY/1
		
* always exec bit, cond exec length bit
* LE,EQ,GT,NEQ

6: GOTO RESULT, CONST				=> code/6 CONST/10 (A/1) (MD/1)

7: LOOP RESULT	 				=> code/10 

8: LOAD RESULT CONST16, CONST32			=> code/4 MODE/2 SUB/1 X/9				------

9: NEG RESULT					=> code/16	(A/1)				----------------

10: WAIT/SIGNAL					=> code/15 SUB/1 -4

11: 

ADDR0: DATAx		A=*ADDR0 | RESULT
ADDR1: DATAx (+ IRS)	B=*ADDR1 | LOOP (for MEM only)

WBMEM=*ADDR1
WBREG= DATAx + LOOP + RESULT


pipeline:
fetch_code | dec1 fetch_mem1 | fetch_mem2 dec2 | exec (writeback)

loop_compare (fetch code) => nothing
loop_correct (decode 1) => clear fetch
pc+=const (exec) => clear fetch
pc+=reg (exec)
  => clear pipeline(3)


instruction depency:
same unit => if avail
different => if data ready (read result)
cmp => data ready (cmp signals)
conditional exec => clear pipeline

flush pipeline (disable execute + disable loop count dec)
  goto
stall pipeline (do not update pc + addr inc etc)
  same address access (dec2)
  wait for result (dec2)
  wait for unit (dec2)
  wait for external data (eg data reg write pending) (dec1)
not execute (local: dec2)
  conditional execute
  flush pipeline



goto: disable loop end detection for 1 cycle

read/write same address => block reading by one cycle
external read/write address valid after two cycles!!
fetch/dec1 stall and goto with flush

fetch: en
  cmp loop
  change PC

dec1: en+nop
  detect loop end flush
  mark DATAx/LOOP writes
  detect external data access stalls (addr register not loaded, already writing to this port)
  detect LOOP access stalls
  dec LOOP

dec2: 
  inc addresses (DATAx)
  detect unit/result stall
  detect register stall
  detect mem addr stall
  disable execute (local only undo address inc)
  detect goto flush

exec:
  write DATAx (falling)
  write LOOP (falling)
  write PC (rising)


Flush:
 - rising edge + comb connection to dec1 disable in current cycle (dec LOOP?? NOT reverted!!!)
 - dec1 -> enable nop counter

Stall:
 - rising edge for isStalled
 - comb for stalling enable => disable register 


02.04.2015 - 06.04.2015
  

forbidden:
goto 0
goto 1
empty loop

Testcase:
 -- GOTO followed by loop end



1 cycle add
1 cycle cmp x ultra fast cmp < 0.5cycles
1 cycle mov from/to int x




viktoria hentsch

petersilikum (petersilie + basilikum)
schucken (schauen + gucken)
kazebra (katze + zebra)





vicktoria bunny
cd seife
rätselhäfte
lippenstift
haizahn kette

ándale!
amorelie

Poolman Herren-Pullover mit Schal

menschen tiere
religion
kunst design/entwerfen/neue
mythos legende


