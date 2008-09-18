let fold_left f accu in_chan block_size =
  let string = String.create block_size in

  let rec loop accu =
    let n = input in_chan string 0 block_size in
      if n = 0 then
        accu
      else
        loop (f accu string n) in    

    loop accu
        
let copy src dst =
  let in_chan = open_in_bin src in
  let out_chan = open_out_bin dst in
  let f () string n = output out_chan string 0 n in
    fold_left f () in_chan 4096;
    close_in in_chan;
    close_out out_chan
      
let read src =
  let in_chan = open_in_bin src in
  let buffer = Buffer.create 4096 in
  let f () string n = Buffer.add_substring buffer string 0 n in
    fold_left f () in_chan 4096;
    close_in in_chan;
    Buffer.contents buffer
    
let move src dst =
  if src != dst then
    try
      Sys.rename src dst
    with Sys_error _ ->
      copy src dst;
      Sys.remove src
  else
    ()

let size path = (Unix.stat path).Unix.st_size

let make_dir path =
  if Sys.file_exists path then
    if Sys.is_directory path then
      ()
    else
      raise (Sys_error 
               (Printf.sprintf
                  "Error creating directory %s: File exists but is not a directory"
                  path))
  else
    try
      Unix.mkdir path 0o755;
    with
        Unix.Unix_error (error, _, _) ->
          raise (Sys_error
                   (Printf.sprintf
                      "Error creating directory %s: %s"
                      path
                      (Unix.error_message error)))
