let ca_installer () =
  (* Window *)
    
  let window = GWindow.window
    ~title:"CA Installer"
    ~width:400
    ~height:320
    ~position:`CENTER
    () in

  let _ = window#connect#destroy ~callback:GMain.Main.quit in
  let () = window#show () in

  (* VBox *)

  let vbox = GPack.vbox
    ~packing:window#add
    () in

  let branches_box = GPack.hbox
    ~packing:vbox#pack
    () in

  let branches_table = GPack.table
    ~col_spacings:4
    ~row_spacings:4
    ~packing:branches_box#pack
    () in

  let downloads_frame = GBin.frame
    ~label:"downloads"
    ~border_width:4
    ~shadow_type:`NONE
    ~label_xalign:0.5
    ~packing:vbox#add
    () in

  (* Queue *)

  let scrolled_window = GBin.scrolled_window
    ~packing:downloads_frame#add
    ~hpolicy:`AUTOMATIC
    ~vpolicy:`AUTOMATIC
    () in

  let queue_table = GPack.table
    ~col_spacings:5
    ~packing:scrolled_window#add_with_viewport
    () in

  let current = ref false in
  let queue = Queue.create () in

  let queue_add revision =
    let row = queue_table#rows in

    let _ = GMisc.label
      ~text:revision
      ~packing:(queue_table#attach ~left:0 ~top:row)
      () in
 
    let progress_bar = GRange.progress_bar
      ~packing:(queue_table#attach ~left:1 ~top:row ~expand:`X)
      () in
     
    let rec add () =
      let start = install revision in
        if !current then
          (status "Queued";
           Queue.add start queue)
        else
          start ()

    and install_revision () =
      Download.install_revision revision fail progress status success
      

    and install revision () =
      current := true;
      ignore (Thread.create install_revision ())

    and fail error =
      status error;
      next ()
        
    and success () =
      status "Complete";
      progress 1.0;
      next ()
              
    and next () =
      try
        (Queue.take queue) ()
      with
          Queue.Empty -> current := false

    and status = progress_bar#set_text
    and progress = progress_bar#set_fraction in

      add () in

  (* Branches *)

  let add_install_branch branch =
    let rows = branches_table#rows in
    let enabled = ref true in

    let _ = GMisc.label
      ~text:(Printf.sprintf "%s revision" branch)
      ~packing:(branches_table#attach ~top:rows ~left:0)
      () in
      
    let entry_frame = GBin.frame
      ~packing:(branches_table#attach ~top:rows ~left:1)
    () in

    let hbox = GPack.hbox
      ~packing:entry_frame#add
      () in

    let entry = GMisc.label
      ~packing:hbox#add
      () in
      
    let button = GButton.check_button
      ~label:"Install"
      ~packing:hbox#add
      () in
  
    let install () =
      if !enabled && button#active then
        queue_add (entry#text)
      else 
        () in

    let fail message = 
      enabled := false;
      button#misc#set_sensitive false;      
      entry#set_text message in

    let success revision =
      enabled := true;
      button#misc#set_sensitive true;
      entry#set_text revision in

    let update = (branch, fail, success) in
      button#set_active true;
      (update, install) in
      
  let (update_stable, install_stable) = add_install_branch "stable" in
  let (update_test, install_test) = add_install_branch "test" in

  let button_box = GPack.button_box `VERTICAL
    ~packing:branches_box#add
    () in

  let install () =
    install_stable ();
    install_test () in

  let update_button = GButton.button
    ~label:"Update"
    ~packing:button_box#pack
    () in

  let install_button = GButton.button
    ~label:"Install"
    ~packing:button_box#pack
    () in

  let update () =
    let branches = [update_stable; update_test] in

    let complete () =
      update_button#misc#set_sensitive true;
      install_button#misc#set_sensitive true in

    let update_revisions () = Download.get_revisions branches complete in

    let () = update_button#misc#set_sensitive false in
    let () = install_button#misc#set_sensitive false in
    let _ = Thread.create update_revisions () in
      () in

  let _ = install_button#connect#clicked ~callback:install in
  let _ = update_button#connect#clicked ~callback:update in

  (* Specific *)

  let rows = branches_table#rows in

  let _ = GMisc.label
    ~text:"specific revision"
    ~packing:(branches_table#attach ~top:rows ~left:0)
    () in

  let entry = GEdit.entry
    ~packing:(branches_table#attach ~top:rows ~left:1)
    () in

  let button = GButton.button
    ~label:"Specific"
    ~packing:button_box#pack
    () in

  let install () = queue_add (entry#text) in
  let _ = button#connect#clicked ~callback:install in

    update ()
            
let main () =
  ca_installer ();
  GtkThread.main ()

let () = main ()
