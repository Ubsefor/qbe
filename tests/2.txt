export function $sieve(w %n, l %prime) {
@fill0.cond
  %uuu =l alloc4 4
  %cond1 =w csgtw %n, 0
  jnz %cond1, @fill0.body, @fill2.header

@fill0.body
  storeb 0, %prime

@fill0.exit
  %n =w cast 2
  %uuu =l alloc8 2
  %uuu =l alloc16 1

@fill2.header
  ret
}
