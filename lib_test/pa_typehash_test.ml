let (==) i1 i2 =
  if i1 <> i2 then
    failwith (Printf.sprintf "%d <> %d" i1 i2)
let (!=) i1 i2 =
  if i1 = i2 then
    failwith (Printf.sprintf "%d <> %d" i1 i2)

module Int = struct
  type t = int with typehash
  TEST_UNIT = Typehash.to_int typehash == 792919698
end

module Alpha = struct
  type 'a t = 'a with typehash
  module Instance1 = struct
    type u = int t with typehash
    TEST_UNIT = Typehash.to_int typehash_of_u == 664011991
  end
  module Instance2 = struct
    type u = float t with typehash
    TEST_UNIT = Typehash.to_int typehash_of_u == 73794421
  end
  module Instance3 = struct
    type u = unit  list t t with typehash
    TEST_UNIT = Typehash.to_int typehash_of_u == 831367328
  end
end

module Record = struct
  type t = {
    a : float;
  } with typehash
  type u = {
    a : float;
  } with typehash
  TEST_UNIT = Typehash.to_int typehash == Typehash.to_int typehash_of_u
  type 'a param = {
    a : 'a;
  } with typehash
  type v = float param with typehash
  TEST_UNIT = Typehash.to_int typehash != Typehash.to_int typehash_of_v
end
