	.text
	la	$t7,_a
	la	$t6,_i
	la	$t5,_sum
	lw	$s6,0($t6)
	lw	$s5,0($t5)
	addi	$t0,$0,100
	j	TEST
LOOP:	sll	$t1,$t6,2
	add	$s5,$s5,$t1
	add	$t2,$t1,$t7
	sw	$s5,0($t2)
	addi	$t6,$t6,1
TEST:	bne	$s6,$t0,LOOP
	sw	$s6,0($t6)
	sw	$s5,0($t5)
	.data
_a:	.space	400000
_i:	.word	1
_sum:	.word	0
