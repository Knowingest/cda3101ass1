	.text
	la	$t0,_i
	lw	$s0,0($t0)
	addi	$t1,$0,10
	j	TEST
LOOP:	addi	$s0,$s0,1
TEST:	bne	$s0,$t0,LOOP
	sw	$s0,0($t0)
	.data
_i:	.word	0
