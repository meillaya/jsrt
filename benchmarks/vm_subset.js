function twice(n) {
  return n * 2;
}

let total = 0;
for (let round = 0; round < 40; round = round + 1) {
  for (let i = 0; i < 4; i = i + 1) {
    total = total + twice(i);
  }
}

typeOf(total) + ":" + len("hello") + ":" + total;
