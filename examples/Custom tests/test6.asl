func f(a : array[10] of int)
    var i : int 
    i = 0;
    while i < 10 do 
        a[i] = 0;
        i = i + 1;
    endwhile
endfunc

func main()
    var a : array[10] of int
    var i : int 
    i = 0;
    while i < 10 do 
        a[i] = i; 
        i = i + 1;
    endwhile 
    f(a);
    write "Despues de f: ";
    i = 0;
    while i < 10 do 
        write a[i]; 
        write " ";
        i = i + 1;
    endwhile
    write "\n";
endfunc