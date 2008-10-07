open ExtString
exception Error of string
  
type kind = Mod | Map | Unknown
    
let load path =
  try
    if String.ends_with path ".sdz" then
      Sdz.In.load path
    else if String.ends_with path ".sd7" then
      Sd7.In.load path
    else
      raise (Error (Printf.sprintf "Unknown archive type: %s" path))
  with
      Zip.Error (path, _, s) -> raise (Error (Printf.sprintf "%s: %s" path s))
    | Sevenzip.Error s -> raise (Error (Printf.sprintf "%s: %s" path s))
    | Sys_error s -> raise (Error s)      
      
let detect_kind path =
  let archive = load path in
  let entries = archive#entries in

  let test_files func =
    let f entry = func (String.lowercase entry#name) in
      List.exists f entries in

  let is_map path =
    let dirname = Filename.dirname path in
    let basename = Filename.basename path in
      if dirname = "maps" then
        if String.ends_with basename ".smf" then
          true
        else if String.ends_with basename ".sm3" then
          true
        else
          false
      else
        false in
    
    archive#unload;
    if test_files is_map then Map
    else if test_files (fun filename -> filename = "modinfo.tdf") then Mod
    else if test_files (fun filename -> filename = "modinfo.lua") then Mod
    else Unknown

