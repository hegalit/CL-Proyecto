func mul(a:float,b:float) : float
  return a*b;
endfunc

func main()
  var x,y:int
  read x;
  read y;
  write "x*y*2.1=";
  write mul(x,y)*2.1;
  write ".\n";
endfunc