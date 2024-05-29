func fibonacci(i:int) : int
    var n0, n1, aux : int
    if (i == 0) then return 0; endif
    if (i == 1) then return 1; endif

    n0 = 0; n1 = 1;
    while (i > 1) do
        aux = n1; 
        n1 = aux + n0; 
        n0 = aux; 
        i = i - 1;
    endwhile
    return n1;
endfunc

func main()
    var i : int
    read i;
    write fibonacci(i);
    write "\n";
endfunc