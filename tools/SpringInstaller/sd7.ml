module Entry = struct
  let make zip entry = object (self)
    method name = entry.Sevenzip.filename
    method size = entry.Sevenzip.uncompressed_size
    method read = "STUB"
    method digest = Digest.string self#read
  end
end  

module In = struct

  let make zip path = object
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
