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