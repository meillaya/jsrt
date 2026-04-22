let total = 0;
let i = 0;
while (i < 200) {
  if (i > 100) {
    total = total + i * 2;
  } else {
    total = total + i;
  }
  i = i + 1;
}
total;
