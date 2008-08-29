open ExtString

module Config = struct
  let spring_dir = Filename.concat (Unix.getenv "HOME") ".spring"
  let map_dir = Filename.concat spring_dir "maps"
  let mod_dir = Filename.concat spring_dir "mods"
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

  let to_help_text t = 
    match t with
        Map -> "Detected map archive"
      | Mod -> "Detected mod archive"
      | Unknown -> "Unable to detect archive type\nSelect from the menu"

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
  let install path dir =
    let basename = Filename.basename path in
    let dest = Filename.concat dir basename in
    let message = Printf.sprintf "%s was successfully installed" basename in
      FileSystem.move path dest;
      GToolbox.message_box ~title:"Success" ~ok:"Exit" message;
      exit 0
        
  let install_archive path combo_box =
    let int = combo_box#active in
    let archive = Archive.from_int int in
      match archive with
          Archive.Mod -> install path Config.mod_dir
        | Archive.Map -> install path Config.map_dir
        | Archive.Unknown ->
            GToolbox.message_box
              ~title:"Error"
              "Please select an archive type from the drop down list"
              
  let detect_archive_type path insert combo_box =
    let archive = Archive.from_path path in
    let basename = Filename.basename path in
    let help_text = Archive.to_help_text archive in
    let text = Printf.sprintf "Archive: %s\n%s\nClick 'Install' to move to your Spring dir" basename help_text in
      combo_box#set_active (Archive.to_int archive);
      insert text
        
  let main path =
    let window = GWindow.window
      ~title:"ArchiveMover"
      ~width:512
      ~height:240
      () in
      
    let vbox = GPack.vbox
      ~packing:window#add
      () in
      
    let out_text = GText.view
      ~wrap_mode:`WORD
      ~packing:(vbox#pack ~fill:true ~expand:true)
      ~editable:false
      () in
      
    let hbox = GPack.hbox
      ~packing:(vbox#pack ~fill:true ~expand:false)
      () in
      
    let (combo_box, _) = GEdit.combo_box_text
      ~strings:["Map"; "Mod"]
      ~packing:(hbox#pack ~fill:true ~expand:true)
      () in
      
    let button_box = GPack.button_box `HORIZONTAL
      ~packing:(hbox#pack ~fill:false ~expand:false)
      () in
      
    let install_button = GButton.button
      ~label:"Install"
      ~packing:button_box#pack
      () in
      
    let exit_button = GButton.button
      ~label:"Exit"
      ~packing:button_box#pack
      () in
      
    let destroy () = GMain.Main.quit () in
    let install () = install_archive path combo_box in
    let _ = window#connect#destroy ~callback:destroy in
    let _ = install_button#connect#clicked ~callback:install in
    let _ = exit_button#connect#clicked ~callback:destroy in
      window#show ();
      detect_archive_type path (out_text#buffer#insert) combo_box;
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
