let copy src dst =
  let string = String.create 4096 in
  let in_chan = open_in_bin src in
  let out_chan = open_out_bin dst in
    
  let rec f () =
    let n = input in_chan string 0 (String.length string) in
      if n = 0 then
        ()
      else
        begin
          output out_chan string 0 n;
          f ()
        end in
    
    f ();
    close_in in_chan;
    close_out out_chan
      
let move src dst =
  if src != dst then
    try
      Sys.rename src dst
    with Sys_error _ ->
      copy src dst;
      Sys.remove src
  else
    ()
