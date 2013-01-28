type 'a t = int
external to_int : _ t -> int = "%identity"

type +'a internal

(* copy pasted from 3.12, because typehash is dependent on this particular hash function *)
external hash_param : int -> int -> 'a -> int = "caml_hash_univ_param_3_12" "noalloc"
let hash x = hash_param 10 100 x

