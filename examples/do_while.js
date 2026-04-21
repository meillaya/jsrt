let i = 0;
let total = 0;

do {
  i = i + 1;
  if (i == 2) {
    continue;
  }
  total = total + i;
} while (i < 4);

total;
