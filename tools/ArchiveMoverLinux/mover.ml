open ExtString

module Config = struct
  let detect_spring_dir () = Filename.concat (Unix.getenv "HOME") ".spring"
end

module Archive = struct
  type t = Mod | Map | Unknown

  let from_int int =
    match int with
        0 -> Map
      | 1 -> Mod
      | _ -> Unknown

  let to_int t =
    match t with
        Map -> 0
      | Mod -> 1
      | Unknown -> -1

  let from_zip path =
    let in_zip = Zip.open_in path in
    let entries = Zip.entries in_zip in
    let has_file filename =
      let lower = String.lowercase filename in
        List.exists (fun entry -> entry.Zip.filename = lower) entries
    in
      Zip.close_in in_zip;
      if has_file "maps" then Map
      else if has_file "modinfo.tdf" then Mod
      else if has_file "modinfo.lua" then Mod
      else Unknown

  let from_path path  =
    if String.ends_with path ".sdz" then
      from_zip path
    else if String.ends_with path ".sd7" then
      Unknown
    else
      Unknown
end

module FileSystem = struct
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
    try
      Unix.rename src dst
    with Unix.Unix_error (Unix.EXDEV, _, _) ->
      copy src dst;
      Unix.unlink src
end


module GUI = struct
  let install path dest =
    print_endline dest;
    let message = Printf.sprintf "%s was successfully installed" dest in
      FileSystem.move path dest;
      GToolbox.message_box ~title:"Success" ~ok:"Exit" message;
      exit 0
        
  let install_archive path combo_box springdir_entry basename_entry =
    let int = combo_box#active in
    let archive = Archive.from_int int in
    let springdir = springdir_entry#text in
    let basename = basename_entry#text in
    let make dir = Filename.concat (Filename.concat springdir dir) basename in
      match archive with
          Archive.Mod -> install path (make "mods")
        | Archive.Map -> install path (make "maps")
        | Archive.Unknown ->
            GToolbox.message_box
              ~title:"Error"
              "Please select an archive type from the drop down list"
              
  let detect_spring_dir springdir_entry =
    let spring_dir = Config.detect_spring_dir () in
      springdir_entry#set_text spring_dir
        
  let detect_basename path basename_entry =
    let basename = Filename.basename path in
      basename_entry#set_text basename
        
  let detect_archive_type path combo_box =
    let archive = Archive.from_path path in
      combo_box#set_active (Archive.to_int archive)

  let detect_all path combo_box springdir_entry basename_entry =
    detect_spring_dir springdir_entry;
    detect_basename path basename_entry;
    detect_archive_type path combo_box
        
  let main path =
    let window = GWindow.window
      ~title:"ArchiveMover"
      ~width:512
      ~height:192
      ~position:`CENTER
      ~type_hint:`DIALOG
      () in
      
    let vbox = GPack.vbox
      ~packing:window#add
      () in

    let table = GPack.table
      ~col_spacings:5
      ~packing:vbox#add
      () in

    let _ = GMisc.label
      ~text:"Spring directory"
      ~packing:(table#attach ~left:0 ~top:0)
      () in

    let springdir_entry = GEdit.entry
      ~packing:(table#attach ~left:1 ~top:0 ~expand:`X)
      () in
      
    let _ = GMisc.label
      ~text:"Filename"
      ~packing:(table#attach ~left:0 ~top:1)
      () in

    let basename_entry = GEdit.entry
      ~packing:(table#attach ~left:1 ~top:1 ~expand:`X)
      () in
      
    let _ = GMisc.label
      ~text:"Archive type"
      ~packing:(table#attach ~left:0 ~top:2)
      () in

    let (combo_box, _) = GEdit.combo_box_text
      ~strings:["Map"; "Mod"]
      ~packing:(table#attach ~left:1 ~top:2 ~expand:`X)
      () in
      
    let button_box = GPack.button_box `HORIZONTAL
      ~packing:(vbox#pack ~fill:false ~expand:false)
      () in
      
    let exit_button = GButton.button
      ~label:"Exit"
      ~packing:button_box#pack
      () in
    
    let detect_button = GButton.button
      ~label:"Auto-detect"
      ~packing:button_box#pack
      () in

    let install_button = GButton.button
      ~label:"Install"
      ~packing:button_box#pack
      () in

    let destroy () = GMain.Main.quit () in
    let install () = install_archive path combo_box springdir_entry basename_entry in
    let detect () = detect_all path combo_box springdir_entry basename_entry in
    let _ = window#connect#destroy ~callback:destroy in
    let _ = exit_button#connect#clicked ~callback:destroy in
    let _ = detect_button#connect#clicked ~callback:detect in
    let _ = install_button#connect#clicked ~callback:install in
      window#show ();
      detect ();
      GtkMain.Main.main ()
end

let usage () =
  ()

let main () =
  if Array.length (Sys.argv) != 2 then
    (usage ();
     exit 1)
  else
    GUI.main Sys.argv.(1)
      
let _ = main ()
