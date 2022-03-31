export function w $go(w %t0, w %t1, w %t2) {
@l0
	%k =l alloc4 4
	storew %t0, %k
	%x =l alloc4 4
	storew %t1, %x
	%y =l alloc4 4
	storew %t2, %y
	%i =l alloc4 4
	%j =l alloc4 4
	%b =l alloc8 8
	%N =l alloc4 4
	%t4 =w loadw %k
	%t8 =l loadl %b
	%t9 =w loadw %x
	%t10 =l extsw %t9
	%t11 =l mul 8, %t10
	%t7 =l add %t8, %t11
	%t6 =l loadl %t7
	%t12 =w loadw %y
	%t13 =l extsw %t12
	%t14 =l mul 4, %t13
	%t5 =l add %t6, %t14
	storew %t4, %t5
	%t16 =w loadw %k
	%t15 =w ceqw %t16, 64
	jnz %t15, @l1, @l2
@l1
	%t19 =w loadw %x
	%t18 =w cnew %t19, 2
	jnz %t18, @l8, @l5
@l8
	%t22 =w loadw %y
	%t21 =w cnew %t22, 0
	jnz %t21, @l7, @l5
@l7
	%t28 =w loadw %x
	%t27 =w sub %t28, 2
	%t26 =w call $abs(w %t27, ...)
	%t31 =w loadw %y
	%t30 =w call $abs(w %t31, ...)
	%t25 =w add %t26, %t30
	%t24 =w ceqw %t25, 3
	jnz %t24, @l4, @l5
@l4
	%t33 =w call $board(...)
	%t35 =w loadw %N
	%t34 =w add %t35, 1
	storew %t34, %N
	%t37 =w loadw %N
	%t36 =w ceqw %t37, 10
	jnz %t36, @l9, @l10
@l9
	%t39 =w call $exit(w 0, ...)
@l10
@l5
	jmp @l3
@l2
	%t42 =w sub 0, 2
	storew %t42, %i
@l12
	%t46 =w loadw %i
	%t45 =w cslew %t46, 2
	jnz %t45, @l13, @l14
@l13
	%t49 =w sub 0, 2
	storew %t49, %j
@l15
	%t53 =w loadw %j
	%t52 =w cslew %t53, 2
	jnz %t52, @l16, @l17
@l16
	%t58 =w loadw %i
	%t57 =w call $abs(w %t58, ...)
	%t60 =w loadw %j
	%t59 =w call $abs(w %t60, ...)
	%t56 =w add %t57, %t59
	%t55 =w ceqw %t56, 3
	jnz %t55, @l21, @l19
@l21
	%t64 =w loadw %x
	%t65 =w loadw %i
	%t63 =w add %t64, %t65
	%t67 =w loadw %y
	%t68 =w loadw %j
	%t66 =w add %t67, %t68
	%t62 =w call $chk(w %t63, w %t66, ...)
	jnz %t62, @l18, @l19
@l18
	%t71 =w loadw %k
	%t70 =w add %t71, 1
	%t74 =w loadw %x
	%t75 =w loadw %i
	%t73 =w add %t74, %t75
	%t77 =w loadw %y
	%t78 =w loadw %j
	%t76 =w add %t77, %t78
	%t69 =w call $go(w %t70, w %t73, w %t76, ...)
@l19
	%t80 =w loadw %j
	%t79 =w add %t80, 1
	storew %t79, %j
	jmp @l15
@l17
	%t82 =w loadw %i
	%t81 =w add %t82, 1
	storew %t81, %i
	jmp @l12
@l14
@l3
	%t88 =l loadl %b
	%t89 =w loadw %x
	%t90 =l extsw %t89
	%t91 =l mul 8, %t90
	%t87 =l add %t88, %t91
	%t86 =l loadl %t87
	%t92 =w loadw %y
	%t93 =l extsw %t92
	%t94 =l mul 4, %t93
	%t85 =l add %t86, %t94
	storew 0, %t85
	ret 0
}
