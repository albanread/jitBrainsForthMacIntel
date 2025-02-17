: FACTORIAL ( +n1 -- +n2 )
   DUP 2 < IF DROP 1 EXIT THEN
   DUP
   BEGIN DUP 2 > WHILE
   1- SWAP OVER * SWAP
   REPEAT DROP
;

: fact dup 2 < if drop 1 exit then dup begin dup 2 > while 1- swap over * swap repeat drop ;

: rfact ( +n1 -- +n2) DUP 2 < IF DROP 1 EXIT THEN  DUP 1- RECURSE * ;


: testCase ( n -- )
  CASE
    1 OF
      ." One"
    ENDOF
    2 OF
      ." Two"
    ENDOF
    3 OF
      ." Three"
    ENDOF

    DEFAULT
      ." Other"
  ENDCASE ;