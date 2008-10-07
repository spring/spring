exception Error of string

external init: unit -> unit = "ml_sevenzip_init"
let () = init ()

type origin = SeekSet | SeekCur | SeekEnd

type readable = {
  read: int -> (string * int);
  seek: int -> origin -> unit
}

type db
type in_file = readable * db

type entry = {
  index: int;
  filename: string;
  uncompressed_size: int;
  is_directory: bool
}

external _open_readable: readable -> db = "ml_sevenzip_open_readable"
let open_readable readable =
  try
    (readable, _open_readable readable)
  with
      Failure s -> raise (Error s)

external _entries: db -> entry array = "ml_sevenzip_entries"
let entries (_, db) = _entries db

let open_in path =
  let file = open_in_bin path in
  let buffer = String.create 65536 in
    
  let read n =
    let length = String.length buffer in
    let n = if n > length then length else n in
    let n = input file buffer 0 n in
      (buffer, n) in
    
  let seek pos origin =
    let pos =
      match origin with
          SeekSet -> pos
        | SeekCur -> (pos_in file) + pos
        | SeekEnd -> (in_channel_length file) + pos in
      seek_in file pos in

    open_readable {read=read; seek=seek}


let close_in t = ()
