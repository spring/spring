module Entry = struct
  let make zip entry = object (self)
    val zip = zip
    val entry = entry

    method name = entry.Sevenzip.filename
    method size = entry.Sevenzip.uncompressed_size
    (* method read = Sevenzip.read_entry zip entry *)
    method read = "STUB"
    method digest = Digest.string self#read
  end
end  

module In = struct

  let make zip path = object
    val zip = zip
    val path: string = path
      
    method unload = Sevenzip.close_in zip
      
    method entries =
      let combine entries entry =
        if entry.Sevenzip.is_directory then
          entries
        else
          (Entry.make zip entry) :: entries
      in
        Array.fold_left combine [] (Sevenzip.entries zip)
          
    method path = path
  end
    
  let load path =
    let zip = Sevenzip.open_in path in
      make zip path
end

(* module Out = struct *)

(*   let make zip = object *)
(*     val zip = zip *)
      
(*     method unload = Sevenzip.close_out zip *)
      
(*     method add_entry entry = *)
(*       let filename = entry#name in *)
(*       let data = entry#read in *)
(*         Sevenzip.add_entry data zip filename *)
(*   end *)
          
(*   let load path = *)
(*     let zip = Sevenzip.open_out path in *)
(*       make zip *)
      
(* end *)
