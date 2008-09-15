let title = "Spring Installer"

let die message = 
  GToolbox.message_box ~title:title ~ok:"Exit" message;
  GMain.Main.quit ()

let warn message = 
  GToolbox.message_box ~title:title ~ok:"Ok" message

let archive_mover path kind datadir =
  (* Window *)

  let window = GWindow.window
    ~title:title
    ~width:512
    ~height:160
    ~position:`CENTER
    () in
    
  let _ = window#connect#destroy ~callback:GMain.Main.quit in

  (* VBox *)

  let vbox = GPack.vbox
    ~packing:window#add
    () in
    
  (* Table *)

  let table = GPack.table
    ~col_spacings:5
    ~packing:vbox#add
    () in
    
  (* Spring Directory *)

  let _ = GMisc.label
    ~text:"Spring directory"
    ~packing:(table#attach ~left:0 ~top:0)
    () in
    
  let spring_dir_entry = GEdit.entry
    ~packing:(table#attach ~left:1 ~top:0 ~expand:`X)
    () in
  let spring_dir_entry_defaults () = spring_dir_entry#set_text datadir in
  let spring_dir_entry_get () = spring_dir_entry#text in

  (* Filename *)

  let _ = GMisc.label
    ~text:"Filename"
    ~packing:(table#attach ~left:0 ~top:1)
    () in
    
  let basename_entry = GEdit.entry
    ~packing:(table#attach ~left:1 ~top:1 ~expand:`X)
    () in

  let basename_entry_defaults () = basename_entry#set_text (Filename.basename path) in
  let basename_entry_get () = basename_entry#text in

  (* Archive Kind *)

  let _ = GMisc.label
    ~text:"Archive kind"
    ~packing:(table#attach ~left:0 ~top:2)
    () in

  let (kind_combobox, _) = GEdit.combo_box_text
    ~strings:["Map"; "Mod"]
    ~packing:(table#attach ~left:1 ~top:2 ~expand:`X)
    () in

  let kind_combobox_defaults () =
    try
      let int = match kind with
          Archive.Map -> 0
        | Archive.Mod -> 1
        | Archive.Unknown -> -1 in
        kind_combobox#set_active int
    with
        Archive.Error s -> warn s in

  let kind_combobox_get () =
    let int = kind_combobox#active in
      match int with
          0 -> Archive.Map
        | 1 -> Archive.Mod
        | _ -> Archive.Unknown in

  let button_box = GPack.button_box `HORIZONTAL
    ~packing:(vbox#pack ~fill:false ~expand:false)
    () in
    
  (* Exit *)

  let exit_button = GButton.button
    ~label:"Exit"
    ~packing:button_box#pack
    () in

  let _ = exit_button#connect#clicked ~callback:GMain.Main.quit in

  (* Defaults *)
    
  let defaults ()=
    spring_dir_entry_defaults ();
    basename_entry_defaults ();
    kind_combobox_defaults () in
    
  let defaults_button = GButton.button
    ~label:"Defaults"
    ~packing:button_box#pack
    () in

  let _ = defaults_button#connect#clicked ~callback:defaults in

  (* Install *)

  let install () =
    let spring_dir = spring_dir_entry_get () in
    let basename = basename_entry_get () in
    let kind = kind_combobox_get () in

    let install_dir sub_dir =
      let dest_dir = Filename.concat spring_dir sub_dir in
      let dest = Filename.concat dest_dir basename in
        try
          FileSystem.make_dirs [spring_dir; dest_dir];
          FileSystem.move path dest;
          die (Printf.sprintf "%s was successfully installed" dest)
        with
            Sys_error s -> die s in
          
      if spring_dir = "" then
        warn "You must specify the Spring directory"
      else
        match kind with
            Archive.Map -> install_dir "maps"
          | Archive.Mod -> install_dir "mods"
          | Archive.Unknown -> warn "You must specify the Archive kind" in

  let install_button = GButton.button
    ~label:"Install"
    ~packing:button_box#pack
    () in

  let _ = install_button#connect#clicked ~callback:install in
  let () = window#focus#set (Some install_button#coerce) in

    (* Body *)

    defaults ();
    window#show ()

let fail message =
  GToolbox.message_box
    ~title:title
    ~ok:"Exit"
    message

let usage () = fail (Printf.sprintf "Usage: %s <archive>" Sys.argv.(0))

let parse_argv () =
  if Array.length (Sys.argv) = 2 then
    try
      let path = Sys.argv.(1) in
      let kind = Archive.detect_kind path in
      let datadir = Datadir.detect () in
        archive_mover path kind datadir;
        GMain.Main.main ()
    with
        Archive.Error s -> fail s
  else
    usage ()

let () = parse_argv ()

