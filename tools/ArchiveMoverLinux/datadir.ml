open ExtString
let detect_home_dir () =
  try
    let home_dir = Sys.getenv "HOME" in
      Some home_dir
  with Not_found -> None
    
let detect_springrc home_dir =
  try
    let springrc = Filename.concat home_dir ".springrc" in
    let in_file = open_in springrc in
    let rec loop option =
      try 
        let line = input_line in_file in
          try
            let (key, value) = String.split line "=" in
              if key = "SpringData" then
                loop (Some value)
              else
                loop option
          with Invalid_string -> loop option
      with End_of_file -> option in
    let option = loop None in
      close_in in_file;
      option
  with Sys_error _ -> None
    
let detect_etc home_dir =
  try
    let in_file = open_in "/etc/spring/datadir" in
    let input = IO.input_channel in_file in
    let format = IO.read_line input in
    let (_, dir) = String.replace format "$HOME" home_dir in
      close_in in_file;
      Some dir
  with Sys_error _ -> None
    
let detect () =
  match detect_home_dir () with
      Some home_dir ->
        (match detect_springrc home_dir with
             Some spring_dir -> spring_dir
           | None ->
               (match detect_etc home_dir with
                    Some etc_dir -> etc_dir
                  | None -> Filename.concat home_dir ".spring"))
    | None -> ""
