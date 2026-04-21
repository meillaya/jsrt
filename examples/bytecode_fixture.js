function bump(n) {
  return n + 1;
}

let value = 0;
for (let i = 0; i < 3; i = i + 1) {
  value = bump(value);
}
