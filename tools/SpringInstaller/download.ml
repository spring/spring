let base_url = "http://files.caspring.org"

let install_revision revision fail progress status success =
  (* Initialize directory structure *)
  let data_dir = Datadir.detect () in
  let mods_dir = Filename.concat data_dir "mods" in
  let ca_dir = Filename.concat data_dir "ca" in
  let revs_dir = Filename.concat ca_dir "revs" in
  let pool_dir = Filename.concat ca_dir "pool" in
  let temp_dir = Filename.concat ca_dir "temp" in

  let () = FileSystem.make_dir data_dir in
  let () = FileSystem.make_dir mods_dir in
  let () = FileSystem.make_dir ca_dir in
  let () = FileSystem.make_dir revs_dir in
  let () = FileSystem.make_dir pool_dir in
  let () = FileSystem.make_dir temp_dir in

  (* Set up pipeline *)
  let pipeline = new Http_client.pipeline in

  let options = {pipeline#get_options with
                   Http_client.number_of_parallel_connections=8;
                   Http_client.synchronization=Http_client.Pipeline 8} in

  let () = pipeline#set_options options in

  let gz_file dir name = 
    let gz_name = Printf.sprintf "%s.gz" name in
      Filename.concat dir gz_name in
    
  let rev_file = gz_file revs_dir revision in
  let temp_rev_file = gz_file temp_dir revision in
    
  let rec get_index () =
    if Sys.file_exists rev_file then
      get_pool ()
    else
      let () = status "Downloading file index" in
      let url = Printf.sprintf "%s/store/revs/%s.gz" base_url revision in
      let storage = `File (fun () -> temp_rev_file) in
      let request = new Http_client.get url in
        request#set_response_body_storage storage;
        pipeline#add request;
        pipeline#run ();

        match request#status with
            `Successful ->
              FileSystem.move temp_rev_file rev_file;
              get_pool ()
          | _ ->
              fail "Error downloading file index"
            
  and get_pool () =
    let store = Store.In.load data_dir revision in
      
    (* Only download repeat digests once *)
    let uniq compare entries =
      match entries with
          [] -> []
        | first :: rest ->
            let combine (last, accu) element =
              if (compare last element) = 0 then
                (last, accu)
              else
                (element, last :: accu) in

            let array = Array.of_list rest in
            let () = Array.sort compare array in
            let (last, uniqs) = Array.fold_left combine (first, []) array in
              last :: uniqs in

    let need_file entry =
      let digest = entry#digest in
      let hex = Hex.encode (digest) in
      let pool_file = gz_file pool_dir hex in
        not (Sys.file_exists pool_file) in
      
    let compare_entry a b = String.compare a#digest b#digest in
    let entries = store#entries in
    let uniqs = uniq compare_entry entries in
    let needed = List.filter need_file uniqs in
      
    let combine size entry = size + (entry#compressed_size) in
    let total_size = List.fold_left combine 0 entries in

      match needed with
          [] -> zip store total_size
        | _ ->

            let needed_size = List.fold_left combine 0 needed in
            let have_size = total_size - needed_size in
            let have_ratio = (float have_size) /. (float total_size) in
            let current_size = ref have_size in

            let () = status "Downloading mod files" in
            let () = progress have_ratio in

            let rec get_pool_file entry =
              let hex = Hex.encode (entry#digest) in
              let temp_pool_file = gz_file temp_dir hex in
              let url = Printf.sprintf "%s/store/pool/%s.gz" base_url hex in
              let storage = `File (fun () -> temp_pool_file) in
              let request = new Http_client.get url in
                request#set_response_body_storage storage;
                pipeline#add_with_callback request (got_pool_file entry)
                    
            and got_pool_file entry result =
              match result#status with
                  `Successful ->
                    let hex = Hex.encode (entry#digest) in
                    let temp_pool_file = gz_file temp_dir hex in
                    let pool_file = gz_file pool_dir hex in
                      FileSystem.move temp_pool_file pool_file;
                      current_size := !current_size + entry#compressed_size;
                      progress ((float !current_size) /. (float total_size));
                | _ ->
                    () in
                      
              List.iter get_pool_file needed;
              pipeline#run ();

              if pipeline#cnt_failed_connections = 0 then
                zip store total_size
              else
                fail "Error downloading pool files" 
  
  and zip store total_size =
    let current_size = ref 0 in
    let () = progress 0.0 in
    let sdz_name = Printf.sprintf "ca-r%s.sdz" revision in
    let temp_mod_file = Filename.concat temp_dir sdz_name in
    let mod_file = Filename.concat mods_dir sdz_name in
      if Sys.file_exists mod_file then
        success ()
      else 
        let () = status "Zipping" in
        let sdz = Sdz.Out.load temp_mod_file in

        let f entry =
          (sdz#add_entry entry;
           current_size := !current_size + entry#compressed_size;
           progress ((float !current_size) /. (float total_size))) in

          List.iter f store#entries;
          sdz#unload;
          FileSystem.move temp_mod_file mod_file;
          success () in
    
    try
      get_index ()
    with
        Sys_error message -> fail message
      
let get_revisions branches complete =
  let pipeline = new Http_client.pipeline in

  let add (branch, fail, success) =
    let url = Printf.sprintf "%s/snapshots/latest_%s" base_url branch in  
    let request = new Http_client.get url in
      pipeline#add request;
      (request, fail, success) in

  let check (request, fail, success) =
    match request#status with
        `Successful -> success request#response_body#value
      | _ -> fail "Error" in

  let results = List.map add branches in
  let () = pipeline#run () in
    List.iter check results;
    complete ()

