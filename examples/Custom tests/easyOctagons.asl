//Draws a octagon of size n
func drawOctagon(n : int)
    var i, j, nChars : int
    var c : char
    c = 'X';

    //Draw upper part
    i = 0;
    while (i < n) do
        j = 0;
        while (j < n-i-1) do
            write ' ';    
            j = j + 1;
        endwhile
        j = 0;
        while (j < n+i*2) do
            write c;
            j = j + 1;
        endwhile
        write "\n";
        i = i + 1;
    endwhile

    //Draw central part
    nChars = n * 3 - 2;
    i = 0;
    while (i < n - 2) do
        j = 0;
        while (j < nChars) do
            write c;
            j = j + 1;
        endwhile
        write "\n";
        i = i + 1;
    endwhile 

    
    //Draw lower part
    i = n-1;
    while (i >= 0) do
        j = 0;
        while (j < n-i-1) do
            write ' ';    
            j = j + 1;
        endwhile
        j = 0;
        while (j < n+i*2) do
            write c;
            j = j + 1;
        endwhile
        write "\n";
        i = i - 1;
    endwhile
endfunc

func main()
    var n : int
    read n;
    if (n < 2) then 
        write "Size must be at least 2!\n"; 
    else 
        drawOctagon(n);
    endif
endfunc