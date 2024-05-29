func main()
    var a : array[10] of int
    var b : array[10] of int
    var i : int 
    i = 0;
    while i < 10 do 
        a[i] = i; 
        i = i + 1;
    endwhile 
    b = a; 
    i = 0;
    while i < 10 do 
        write b[i]; 
        write " ";
        i = i + 1;
    endwhile
    write "\n";
endfunc