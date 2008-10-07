module Entry = struct
  let make zip entry = object (self)
    method name = entry.Zip.filename
    method size = entry.Zip.uncompressed_size
    method read = Zip.read_entry zip entry
    method digest = Digest.string self#read
  end
end  

module In = struct

  let make zip path = object
    method unload = Zip.close_in zip
      
    method entries =
      let combine entries entry =
        if entry.Zip.is_directory then
          entries
        else
          (Entry.make zip entry) :: entries
      in
        List.fold_left combine [] (Zip.entries zip)
          
    method path = path
  end
    
  let load path =
    let zip = Zip.open_in path in
      make zip path
end

module Out = struct

  let make zip = object
    method unload = Zip.close_out zip
      
    method add_entry entry =
      let filename = entry#name in
      let data = entry#read in
        Zip.add_entry data zip filename
  end
          
  let load path =
    let zip = Zip.open_out path in
      make zip
      
end
