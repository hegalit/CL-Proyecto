func fizzbuzz(n : int) 
    var i : int
    i = 0;
    while i < n do
        if i % 15 == 0 then
            write "FizzBuzz!\n";
        else if i % 3 == 0 then
                write "Fizz\n";
            else if i % 5 == 0 then
                    write "Buzz\n";
                else 
                    write i;
                    write "\n";
                endif
            endif
        endif
        i = i + 1;
    endwhile
endfunc

func main()
    var n : int
    read n;
    fizzbuzz(n);
endfunc