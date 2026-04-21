let makeAdder = function (base) {
  return function (n) {
    return base + n;
  };
};

let add3 = makeAdder(3);
add3(8);
